#include <iostream>

#include "opencv2/opencv_modules.hpp"

#if defined(HAVE_OPENCV_MUSACODEC)

#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

#include <opencv2/core.hpp>
#include <opencv2/core/opengl.hpp>
#include <opencv2/musacodec.hpp>
#include <opencv2/highgui.hpp>

int main(int argc, const char* argv[])
{
    if (argc != 2)
        return -1;

    const std::string fname(argv[1]);

    cv::namedWindow("CPU", cv::WINDOW_NORMAL);
    cv::namedWindow("GPU", cv::WINDOW_OPENGL);
    cv::musa::setGlDevice();

    cv::Mat frame;
    cv::VideoCapture reader(fname);

    cv::musa::GpuMat d_frame;
    cv::Ptr<cv::musacodec::VideoReader> d_reader = cv::musacodec::createVideoReader(fname);

    cv::TickMeter tm;
    std::vector<double> cpu_times;
    std::vector<double> gpu_times;

    int gpu_frame_count=0, cpu_frame_count=0;

    for (;;)
    {
        tm.reset(); tm.start();
        if (!reader.read(frame))
            break;
        tm.stop();
        cpu_times.push_back(tm.getTimeMilli());
        cpu_frame_count++;

        cv::imshow("CPU", frame);

        if (cv::waitKey(3) > 0)
            break;
    }

    for (;;)
    {
        tm.reset(); tm.start();
        if (!d_reader->nextFrame(d_frame))
            break;
        tm.stop();
        gpu_times.push_back(tm.getTimeMilli());
        gpu_frame_count++;

        cv::imshow("GPU", d_frame);

        if (cv::waitKey(3) > 0)
            break;
    }

    if (!cpu_times.empty() && !gpu_times.empty())
    {
        std::cout << std::endl << "Results:" << std::endl;

        std::sort(cpu_times.begin(), cpu_times.end());
        std::sort(gpu_times.begin(), gpu_times.end());

        double cpu_avg = std::accumulate(cpu_times.begin(), cpu_times.end(), 0.0) / cpu_times.size();
        double gpu_avg = std::accumulate(gpu_times.begin(), gpu_times.end(), 0.0) / gpu_times.size();

        std::cout << "CPU : Avg : " << cpu_avg << " ms FPS : " << 1000.0 / cpu_avg << " Frames " << cpu_frame_count << std::endl;
        std::cout << "GPU : Avg : " << gpu_avg << " ms FPS : " << 1000.0 / gpu_avg << " Frames " << gpu_frame_count << std::endl;
    }

    return 0;
}

#else

int main()
{
    std::cout << "OpenCV was built without MUSA Video decoding support\n" << std::endl;
    return 0;
}

#endif
