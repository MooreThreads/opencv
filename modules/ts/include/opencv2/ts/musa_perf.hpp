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

#ifndef OPENCV_MUSA_PERF_UTILITY_HPP
#define OPENCV_MUSA_PERF_UTILITY_HPP

#include "opencv2/ts.hpp"

#include "opencv2/ts/ts_perf.hpp"

namespace perf
{
    #define ALL_BORDER_MODES BorderMode::all()
    #define ALL_INTERPOLATIONS Interpolation::all()

    CV_ENUM(BorderMode, BORDER_REFLECT101, BORDER_REPLICATE, BORDER_CONSTANT, BORDER_REFLECT, BORDER_WRAP)
    CV_ENUM(Interpolation, INTER_NEAREST, INTER_LINEAR, INTER_CUBIC, INTER_AREA)
    CV_ENUM(NormType, NORM_INF, NORM_L1, NORM_L2, NORM_HAMMING, NORM_MINMAX)

    enum { Gray = 1, TwoChannel = 2, BGR = 3, BGRA = 4 };
    CV_ENUM(MatCn, Gray, TwoChannel, BGR, BGRA)

    #define MUSA_CHANNELS_1_3_4 testing::Values(MatCn(Gray), MatCn(BGR), MatCn(BGRA))
    #define MUSA_CHANNELS_1_3 testing::Values(MatCn(Gray), MatCn(BGR))

    #define GET_PARAM(k) testing::get< k >(GetParam())

    #define DEF_PARAM_TEST(name, ...) typedef ::perf::TestBaseWithParam< testing::tuple< __VA_ARGS__ > > name
    #define DEF_PARAM_TEST_1(name, param_type) typedef ::perf::TestBaseWithParam< param_type > name

    DEF_PARAM_TEST_1(Sz, cv::Size);
    typedef perf::Size_MatType Sz_Type;
    DEF_PARAM_TEST(Sz_Depth, cv::Size, perf::MatDepth);
    DEF_PARAM_TEST(Sz_Depth_Cn, cv::Size, perf::MatDepth, MatCn);

    #define MUSA_TYPICAL_MAT_SIZES testing::Values(perf::sz720p, perf::szSXGA, perf::sz1080p)

    #define FAIL_NO_CPU() FAIL() << "No such CPU implementation analogy"

    #define MUSA_SANITY_CHECK(mat, ...) \
        do{ \
            cv::Mat gpu_##mat(mat); \
            SANITY_CHECK(gpu_##mat, ## __VA_ARGS__); \
        } while(0)

    #define CPU_SANITY_CHECK(mat, ...) \
        do{ \
            cv::Mat cpu_##mat(mat); \
            SANITY_CHECK(cpu_##mat, ## __VA_ARGS__); \
        } while(0)

    cv::Mat readGpuImage(const std::string& fileName, int flags = cv::IMREAD_COLOR);

    struct CvtColorInfo
    {
        int scn;
        int dcn;
        int code;

        CvtColorInfo() {}
        explicit CvtColorInfo(int scn_, int dcn_, int code_) : scn(scn_), dcn(dcn_), code(code_) {}
    };
    void musaPrintTo(const CvtColorInfo& info, std::ostream* os);

    void printMusaInfo();

    void sortMusaKeyPoints(std::vector<cv::KeyPoint>& keypoints, cv::InputOutputArray _descriptors = cv::noArray());

#ifdef HAVE_MUSA
    #define CV_PERF_TEST_MUSA_MAIN(modulename) \
        int main(int argc, char **argv)\
        {\
            const char * impls[] = { "musa", "plain" };\
            CV_PERF_TEST_MAIN_INTERNALS(modulename, impls, perf::printMusaInfo())\
        }
#else
    #define CV_PERF_TEST_MUSA_MAIN(modulename) \
        int main(int argc, char **argv)\
        {\
            const char * plain_only[] = { "plain" };\
            CV_PERF_TEST_MAIN_INTERNALS(modulename, plain_only)\
        }
#endif
}

#endif // OPENCV_MUSA_PERF_UTILITY_HPP
