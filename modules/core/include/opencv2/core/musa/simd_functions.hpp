/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef OPENCV_MUSA_SIMD_FUNCTIONS_HPP
#define OPENCV_MUSA_SIMD_FUNCTIONS_HPP

#include "common.hpp"

/** @file
 * @deprecated Use @ref mudev instead.
 */

//! @cond IGNORED

namespace cv { namespace musa { namespace device
{
    // 2

    static __device__ __forceinline__ unsigned int vadd2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;
        unsigned int s;
        s = a ^ b;          // sum bits
        r = a + b;          // actual sum
        s = s ^ r;          // determine carry-ins for each bit position
        s = s & 0x00010000; // carry-in to high word (= carry-out from low word)
        r = r - s;          // subtract out carry-out from low word

        return r;
    }

    static __device__ __forceinline__ unsigned int vsub2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;
        unsigned int s;
        s = a ^ b;          // sum bits
        r = a - b;          // actual sum
        s = s ^ r;          // determine carry-ins for each bit position
        s = s & 0x00010000; // borrow to high word
        r = r + s;          // compensate for borrow from low word

        return r;
    }

    static __device__ __forceinline__ unsigned int vabsdiff2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;
        unsigned int s, t, u, v;
        s = a & 0x0000ffff; // extract low halfword
        r = b & 0x0000ffff; // extract low halfword
        u = ::max(r, s);    // maximum of low halfwords
        v = ::min(r, s);    // minimum of low halfwords
        s = a & 0xffff0000; // extract high halfword
        r = b & 0xffff0000; // extract high halfword
        t = ::max(r, s);    // maximum of high halfwords
        s = ::min(r, s);    // minimum of high halfwords
        r = u | t;          // maximum of both halfwords
        s = v | s;          // minimum of both halfwords
        r = r - s;          // |a - b| = max(a,b) - min(a,b);

        return r;
    }

    static __device__ __forceinline__ unsigned int vavg2(unsigned int a, unsigned int b)
    {
        unsigned int r, s;

        // HAKMEM #23: a + b = 2 * (a & b) + (a ^ b) ==>
        // (a + b) / 2 = (a & b) + ((a ^ b) >> 1)
        s = a ^ b;
        r = a & b;
        s = s & 0xfffefffe; // ensure shift doesn't cross halfword boundaries
        s = s >> 1;
        s = r + s;

        return s;
    }

    static __device__ __forceinline__ unsigned int vavrg2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        // HAKMEM #23: a + b = 2 * (a | b) - (a ^ b) ==>
        // (a + b + 1) / 2 = (a | b) - ((a ^ b) >> 1)
        unsigned int s;
        s = a ^ b;
        r = a | b;
        s = s & 0xfffefffe; // ensure shift doesn't cross half-word boundaries
        s = s >> 1;
        r = r - s;

        return r;
    }

    static __device__ __forceinline__ unsigned int vseteq2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        // inspired by Alan Mycroft's null-byte detection algorithm:
        // null_byte(x) = ((x - 0x01010101) & (~x & 0x80808080))
        unsigned int c;
        r = a ^ b;          // 0x0000 if a == b
        c = r | 0x80008000; // set msbs, to catch carry out
        r = r ^ c;          // extract msbs, msb = 1 if r < 0x8000
        c = c - 0x00010001; // msb = 0, if r was 0x0000 or 0x8000
        c = r & ~c;         // msb = 1, if r was 0x0000
        r = c >> 15;        // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmpeq2(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // inspired by Alan Mycroft's null-byte detection algorithm:
        // null_byte(x) = ((x - 0x01010101) & (~x & 0x80808080))
        r = a ^ b;          // 0x0000 if a == b
        c = r | 0x80008000; // set msbs, to catch carry out
        r = r ^ c;          // extract msbs, msb = 1 if r < 0x8000
        c = c - 0x00010001; // msb = 0, if r was 0x0000 or 0x8000
        c = r & ~c;         // msb = 1, if r was 0x0000
        r = c >> 15;        // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetge2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int c;
        // asm("not.b32 %0, %0;" : "+r"(b));
        b = ~b;
        c = vavrg2(a, b);   // (a + ~b + 1) / 2 = (a - b) / 2
        c = c & 0x80008000; // msb = carry-outs
        r = c >> 15;        // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmpge2(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // asm("not.b32 %0, %0;" : "+r"(b));
        b = ~b;
        c = vavrg2(a, b);   // (a + ~b + 1) / 2 = (a - b) / 2
        c = c & 0x80008000; // msb = carry-outs
        r = c >> 15;        // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetgt2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int c;
        // asm("not.b32 %0, %0;" : "+r"(b));
        b = ~b;
        c = vavg2(a, b);    // (a + ~b) / 2 = (a - b) / 2 [rounded down]
        c = c & 0x80008000; // msbs = carry-outs
        r = c >> 15;        // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmpgt2(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // asm("not.b32 %0, %0;" : "+r"(b));
        b = ~b;
        c = vavg2(a, b);    // (a + ~b) / 2 = (a - b) / 2 [rounded down]
        c = c & 0x80008000; // msbs = carry-outs
        r = c >> 15;        // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetle2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int c;
        // asm("not.b32 %0, %0;" : "+r"(a));
        a = ~a;
        c = vavrg2(a, b);   // (b + ~a + 1) / 2 = (b - a) / 2
        c = c & 0x80008000; // msb = carry-outs
        r = c >> 15;        // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmple2(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // asm("not.b32 %0, %0;" : "+r"(a));
        a = ~a;
        c = vavrg2(a, b);   // (b + ~a + 1) / 2 = (b - a) / 2
        c = c & 0x80008000; // msb = carry-outs
        r = c >> 15;        // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetlt2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int c;
        // asm("not.b32 %0, %0;" : "+r"(a));
        a = ~a;
        c = vavg2(a, b);    // (b + ~a) / 2 = (b - a) / 2 [rounded down]
        c = c & 0x80008000; // msb = carry-outs
        r = c >> 15;        // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmplt2(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // asm("not.b32 %0, %0;" : "+r"(a));
        a = ~a;
        c = vavg2(a, b);    // (b + ~a) / 2 = (b - a) / 2 [rounded down]
        c = c & 0x80008000; // msb = carry-outs
        r = c >> 15;        // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetne2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        // inspired by Alan Mycroft's null-byte detection algorithm:
        // null_byte(x) = ((x - 0x01010101) & (~x & 0x80808080))
        unsigned int c;
        r = a ^ b;          // 0x0000 if a == b
        c = r | 0x80008000; // set msbs, to catch carry out
        c = c - 0x00010001; // msb = 0, if r was 0x0000 or 0x8000
        c = r | c;          // msb = 1, if r was not 0x0000
        c = c & 0x80008000; // extract msbs
        r = c >> 15;        // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmpne2(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // inspired by Alan Mycroft's null-byte detection algorithm:
        // null_byte(x) = ((x - 0x01010101) & (~x & 0x80808080))
        r = a ^ b;          // 0x0000 if a == b
        c = r | 0x80008000; // set msbs, to catch carry out
        c = c - 0x00010001; // msb = 0, if r was 0x0000 or 0x8000
        c = r | c;          // msb = 1, if r was not 0x0000
        c = c & 0x80008000; // extract msbs
        r = c >> 15;        // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vmax2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int s, t, u;
        r = a & 0x0000ffff; // extract low halfword
        s = b & 0x0000ffff; // extract low halfword
        t = ::max(r, s);    // maximum of low halfwords
        r = a & 0xffff0000; // extract high halfword
        s = b & 0xffff0000; // extract high halfword
        u = ::max(r, s);    // maximum of high halfwords
        r = t | u;          // combine halfword maximums

        return r;
    }

    static __device__ __forceinline__ unsigned int vmin2(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int s, t, u;
        r = a & 0x0000ffff; // extract low halfword
        s = b & 0x0000ffff; // extract low halfword
        t = ::min(r, s);    // minimum of low halfwords
        r = a & 0xffff0000; // extract high halfword
        s = b & 0xffff0000; // extract high halfword
        u = ::min(r, s);    // minimum of high halfwords
        r = t | u;          // combine halfword minimums

        return r;
    }

    // 4

    static __device__ __forceinline__ unsigned int vadd4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int s, t;
        s = a ^ b;          // sum bits
        r = a & 0x7f7f7f7f; // clear msbs
        t = b & 0x7f7f7f7f; // clear msbs
        s = s & 0x80808080; // msb sum bits
        r = r + t;          // add without msbs, record carry-out in msbs
        r = r ^ s;          // sum of msb sum and carry-in bits, w/o carry-out

        return r;
    }

    static __device__ __forceinline__ unsigned int vsub4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int s, t;
        s = a ^ ~b;         // inverted sum bits
        r = a | 0x80808080; // set msbs
        t = b & 0x7f7f7f7f; // clear msbs
        s = s & 0x80808080; // inverted msb sum bits
        r = r - t;          // subtract w/o msbs, record inverted borrows in msb
        r = r ^ s;          // combine inverted msb sum bits and borrows

        return r;
    }

    static __device__ __forceinline__ unsigned int vavg4(unsigned int a, unsigned int b)
    {
        unsigned int r, s;

        // HAKMEM #23: a + b = 2 * (a & b) + (a ^ b) ==>
        // (a + b) / 2 = (a & b) + ((a ^ b) >> 1)
        s = a ^ b;
        r = a & b;
        s = s & 0xfefefefe; // ensure following shift doesn't cross byte boundaries
        s = s >> 1;
        s = r + s;

        return s;
    }

    static __device__ __forceinline__ unsigned int vavrg4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        // HAKMEM #23: a + b = 2 * (a | b) - (a ^ b) ==>
        // (a + b + 1) / 2 = (a | b) - ((a ^ b) >> 1)
        unsigned int c;
        c = a ^ b;
        r = a | b;
        c = c & 0xfefefefe; // ensure following shift doesn't cross byte boundaries
        c = c >> 1;
        r = r - c;

        return r;
    }

    static __device__ __forceinline__ unsigned int vseteq4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        // inspired by Alan Mycroft's null-byte detection algorithm:
        // null_byte(x) = ((x - 0x01010101) & (~x & 0x80808080))
        unsigned int c;
        r = a ^ b;          // 0x00 if a == b
        c = r | 0x80808080; // set msbs, to catch carry out
        r = r ^ c;          // extract msbs, msb = 1 if r < 0x80
        c = c - 0x01010101; // msb = 0, if r was 0x00 or 0x80
        c = r & ~c;         // msb = 1, if r was 0x00
        r = c >> 7;         // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmpeq4(unsigned int a, unsigned int b)
    {
        unsigned int r, t;

        // inspired by Alan Mycroft's null-byte detection algorithm:
        // null_byte(x) = ((x - 0x01010101) & (~x & 0x80808080))
        t = a ^ b;          // 0x00 if a == b
        r = t | 0x80808080; // set msbs, to catch carry out
        t = t ^ r;          // extract msbs, msb = 1 if t < 0x80
        r = r - 0x01010101; // msb = 0, if t was 0x00 or 0x80
        r = t & ~r;         // msb = 1, if t was 0x00
        t = r >> 7;         // build mask
        t = r - t;          //  from
        r = t | r;          //   msbs

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetle4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int c;
        // asm("not.b32 %0, %0;" : "+r"(a));
        a = ~a;
        c = vavrg4(a, b);   // (b + ~a + 1) / 2 = (b - a) / 2
        c = c & 0x80808080; // msb = carry-outs
        r = c >> 7;         // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmple4(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // asm("not.b32 %0, %0;" : "+r"(a));
        a = ~a;
        c = vavrg4(a, b);   // (b + ~a + 1) / 2 = (b - a) / 2
        c = c & 0x80808080; // msbs = carry-outs
        r = c >> 7;         // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetlt4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int c;
        // asm("not.b32 %0, %0;" : "+r"(a));
        a = ~a;
        c = vavg4(a, b);    // (b + ~a) / 2 = (b - a) / 2 [rounded down]
        c = c & 0x80808080; // msb = carry-outs
        r = c >> 7;         // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmplt4(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // asm("not.b32 %0, %0;" : "+r"(a));
        a = ~a;
        c = vavg4(a, b);    // (b + ~a) / 2 = (b - a) / 2 [rounded down]
        c = c & 0x80808080; // msbs = carry-outs
        r = c >> 7;         // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetge4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int c;
        // asm("not.b32 %0, %0;" : "+r"(b));
        b = ~b;
        c = vavrg4(a, b);   // (a + ~b + 1) / 2 = (a - b) / 2
        c = c & 0x80808080; // msb = carry-outs
        r = c >> 7;         // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmpge4(unsigned int a, unsigned int b)
    {
        unsigned int r, s;

        // asm ("not.b32 %0,%0;" : "+r"(b));
        b = ~b;
        r = vavrg4 (a, b);  // (a + ~b + 1) / 2 = (a - b) / 2
        r = r & 0x80808080; // msb = carry-outs
        s = r >> 7;         // build mask
        s = r - s;          //  from
        r = s | r;          //   msbs

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetgt4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int c;
        // asm("not.b32 %0, %0;" : "+r"(b));
        b = ~b;
        c = vavg4(a, b);    // (a + ~b) / 2 = (a - b) / 2 [rounded down]
        c = c & 0x80808080; // msb = carry-outs
        r = c >> 7;         // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmpgt4(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // asm("not.b32 %0, %0;" : "+r"(b));
        b = ~b;
        c = vavg4(a, b);    // (a + ~b) / 2 = (a - b) / 2 [rounded down]
        c = c & 0x80808080; // msb = carry-outs
        r = c >> 7;         // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vsetne4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        // inspired by Alan Mycroft's null-byte detection algorithm:
        // null_byte(x) = ((x - 0x01010101) & (~x & 0x80808080))
        unsigned int c;
        r = a ^ b;          // 0x00 if a == b
        c = r | 0x80808080; // set msbs, to catch carry out
        c = c - 0x01010101; // msb = 0, if r was 0x00 or 0x80
        c = r | c;          // msb = 1, if r was not 0x00
        c = c & 0x80808080; // extract msbs
        r = c >> 7;         // convert to bool

        return r;
    }

    static __device__ __forceinline__ unsigned int vcmpne4(unsigned int a, unsigned int b)
    {
        unsigned int r, c;

        // inspired by Alan Mycroft's null-byte detection algorithm:
        // null_byte(x) = ((x - 0x01010101) & (~x & 0x80808080))
        r = a ^ b;          // 0x00 if a == b
        c = r | 0x80808080; // set msbs, to catch carry out
        c = c - 0x01010101; // msb = 0, if r was 0x00 or 0x80
        c = r | c;          // msb = 1, if r was not 0x00
        c = c & 0x80808080; // extract msbs
        r = c >> 7;         // convert
        r = c - r;          //  msbs to
        r = c | r;          //   mask

        return r;
    }

    static __device__ __forceinline__ unsigned int vabsdiff4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int s;
        s = vcmpge4(a, b);  // mask = 0xff if a >= b
        r = a ^ b;          //
        s = (r &  s) ^ b;   // select a when a >= b, else select b => max(a,b)
        r = s ^ r;          // select a when b >= a, else select b => min(a,b)
        r = s - r;          // |a - b| = max(a,b) - min(a,b);

        return r;
    }

    static __device__ __forceinline__ unsigned int vmax4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int s;
        s = vcmpge4(a, b);  // mask = 0xff if a >= b
        r = a & s;          // select a when b >= a
        s = b & ~s;         // select b when b < a
        r = r | s;          // combine byte selections

        return r;           // byte-wise unsigned maximum
    }

    static __device__ __forceinline__ unsigned int vmin4(unsigned int a, unsigned int b)
    {
        unsigned int r = 0;

        unsigned int s;
        s = vcmpge4(b, a);  // mask = 0xff if a >= b
        r = a & s;          // select a when b >= a
        s = b & ~s;         // select b when b < a
        r = r | s;          // combine byte selections

        return r;
    }
}}}

//! @endcond

#endif // OPENCV_MUSA_SIMD_FUNCTIONS_HPP
