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

#include "precomp.hpp"

#include "opencv2/photo/musa.hpp"
#include "opencv2/core/private.musa.hpp"

#include "opencv2/opencv_modules.hpp"

#ifdef HAVE_OPENCV_MUSAARITHM
#  include "opencv2/musaarithm.hpp"
#endif

#ifdef HAVE_OPENCV_MUSAIMGPROC
#  include "opencv2/musaimgproc.hpp"
#endif

using namespace cv;
using namespace cv::musa;

#if !defined (HAVE_MUSA) || !defined(HAVE_OPENCV_MUSAARITHM) || !defined(HAVE_OPENCV_MUSAIMGPROC)

void cv::musa::nonLocalMeans(InputArray, OutputArray, float, int, int, int, Stream&) { throw_no_musa(); }
void cv::musa::fastNlMeansDenoising(InputArray, OutputArray, float, int, int, Stream&) { throw_no_musa(); }
void cv::musa::fastNlMeansDenoisingColored(InputArray, OutputArray, float, float, int, int, Stream&) { throw_no_musa(); }

#else

//////////////////////////////////////////////////////////////////////////////////
//// Non Local Means Denosing (brute force)

namespace cv { namespace musa { namespace device
{
    namespace imgproc
    {
        template<typename T>
        void nlm_bruteforce_gpu(const PtrStepSzb& src, PtrStepSzb dst, int search_radius, int block_radius, float h, int borderMode, musaStream_t stream);
    }
}}}

void cv::musa::nonLocalMeans(InputArray _src, OutputArray _dst, float h, int search_window, int block_window, int borderMode, Stream& stream)
{
    using cv::musa::device::imgproc::nlm_bruteforce_gpu;
    typedef void (*func_t)(const PtrStepSzb& src, PtrStepSzb dst, int search_radius, int block_radius, float h, int borderMode, musaStream_t stream);

    static const func_t funcs[4] = { nlm_bruteforce_gpu<uchar>, nlm_bruteforce_gpu<uchar2>, nlm_bruteforce_gpu<uchar3>, 0/*nlm_bruteforce_gpu<uchar4>,*/ };

    const GpuMat src = _src.getMUSAGpuMat();

    CV_Assert(src.type() == CV_8U || src.type() == CV_8UC2 || src.type() == CV_8UC3);

    const func_t func = funcs[src.channels() - 1];
    CV_Assert(func != 0);

    int b = borderMode;
    CV_Assert(b == BORDER_REFLECT101 || b == BORDER_REPLICATE || b == BORDER_CONSTANT || b == BORDER_REFLECT || b == BORDER_WRAP);

    _dst.create(src.size(), src.type());
    GpuMat dst = _dst.getMUSAGpuMat();

    func(src, dst, search_window/2, block_window/2, h, borderMode, StreamAccessor::getStream(stream));
}

namespace cv { namespace musa { namespace device
{
    namespace imgproc
    {
        void nln_fast_get_buffer_size(const PtrStepSzb& src, int search_window, int block_window, int& buffer_cols, int& buffer_rows);

        template<typename T>
        void nlm_fast_gpu(const PtrStepSzb& src, PtrStepSzb dst, PtrStepi buffer,
                          int search_window, int block_window, float h, musaStream_t stream);

        void fnlm_split_channels(const PtrStepSz<uchar3>& lab, PtrStepb l, PtrStep<uchar2> ab, musaStream_t stream);
        void fnlm_merge_channels(const PtrStepb& l, const PtrStep<uchar2>& ab, PtrStepSz<uchar3> lab, musaStream_t stream);
     }
}}}

void cv::musa::fastNlMeansDenoising(InputArray _src, OutputArray _dst, float h, int search_window, int block_window, Stream& stream)
{
    const GpuMat src = _src.getMUSAGpuMat();

    CV_Assert(src.depth() == CV_8U && src.channels() < 4);

    int border_size = search_window/2 + block_window/2;
    Size esize = src.size() + Size(border_size, border_size) * 2;

    BufferPool pool(stream);

    GpuMat extended_src = pool.getBuffer(esize, src.type());
    cv::musa::copyMakeBorder(src, extended_src, border_size, border_size, border_size, border_size, cv::BORDER_DEFAULT, Scalar(), stream);
    GpuMat src_hdr = extended_src(Rect(Point2i(border_size, border_size), src.size()));

    int bcols, brows;
    device::imgproc::nln_fast_get_buffer_size(src_hdr, search_window, block_window, bcols, brows);
    GpuMat buffer = pool.getBuffer(brows, bcols, CV_32S);

    using namespace cv::musa::device::imgproc;
    typedef void (*nlm_fast_t)(const PtrStepSzb&, PtrStepSzb, PtrStepi, int, int, float, musaStream_t);
    static const nlm_fast_t funcs[] = { nlm_fast_gpu<uchar>, nlm_fast_gpu<uchar2>, nlm_fast_gpu<uchar3>, 0};

    _dst.create(src.size(), src.type());
    GpuMat dst = _dst.getMUSAGpuMat();

    funcs[src.channels()-1](src_hdr, dst, buffer, search_window, block_window, h, StreamAccessor::getStream(stream));
}

void cv::musa::fastNlMeansDenoisingColored(InputArray _src, OutputArray _dst, float h_luminance, float h_color, int search_window, int block_window, Stream& stream)
{
    const GpuMat src = _src.getMUSAGpuMat();

    CV_Assert(src.type() == CV_8UC3);

    BufferPool pool(stream);

    GpuMat lab = pool.getBuffer(src.size(), src.type());
    cv::musa::cvtColor(src, lab, cv::COLOR_BGR2Lab, 0, stream);

    GpuMat l = pool.getBuffer(src.size(), CV_8U);
    GpuMat ab = pool.getBuffer(src.size(), CV_8UC2);
    device::imgproc::fnlm_split_channels(lab, l, ab, StreamAccessor::getStream(stream));

    fastNlMeansDenoising(l, l, h_luminance, search_window, block_window, stream);
    fastNlMeansDenoising(ab, ab, h_color, search_window, block_window, stream);

    device::imgproc::fnlm_merge_channels(l, ab, lab, StreamAccessor::getStream(stream));
    cv::musa::cvtColor(lab, _dst, cv::COLOR_Lab2BGR, 0, stream);
}

#endif
