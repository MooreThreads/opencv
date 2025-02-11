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

using namespace cv;
using namespace cv::musa;

int cv::musa::getMusaEnabledDeviceCount() {
#ifndef HAVE_MUSA
  return 0;
#else
  int count;
  musaError_t error = musaGetDeviceCount(&count);

  if (error == musaErrorInsufficientDriver) return -1;

  if (error == musaErrorNoDevice) return 0;

  musaSafeCall(error);
  return count;
#endif
}

void cv::musa::setDevice(int device) {
#ifndef HAVE_MUSA
  CV_UNUSED(device);
  throw_no_musa();
#else
  musaSafeCall(musaSetDevice(device));
  musaSafeCall(musaFree(0));
#endif
}

int cv::musa::getDevice() {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  int device;
  musaSafeCall(musaGetDevice(&device));
  return device;
#endif
}

void cv::musa::resetDevice() {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  musaSafeCall(musaDeviceReset());
#endif
}

bool cv::musa::deviceSupports(FeatureSet feature_set) {
#ifndef HAVE_MUSA
  CV_UNUSED(feature_set);
  throw_no_musa();
#else
  static int versions[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  static const int cache_size =
      static_cast<int>(sizeof(versions) / sizeof(versions[0]));

  const int devId = getDevice();

  int version;

  if (devId < cache_size && versions[devId] >= 0) {
    version = versions[devId];
  } else {
    DeviceInfo dev(devId);
    version = dev.majorVersion() * 10 + dev.minorVersion();
    if (devId < cache_size) versions[devId] = version;
  }

  return TargetArchs::builtWith(feature_set) && (version >= feature_set);
#endif
}

////////////////////////////////////////////////////////////////////////
// TargetArchs

#ifdef HAVE_MUSA

namespace {
class MusaArch {
 public:
  MusaArch();

  bool builtWith(FeatureSet feature_set) const;
  bool hasPtx(int major, int minor) const;
  bool hasBin(int major, int minor) const;
  bool hasEqualOrLessPtx(int major, int minor) const;
  bool hasEqualOrGreaterPtx(int major, int minor) const;
  bool hasEqualOrGreaterBin(int major, int minor) const;

 private:
  static void fromStr(const char* set_as_str, std::vector<int>& arr);

  std::vector<int> bin;
  std::vector<int> ptx;
  std::vector<int> features;
};

const MusaArch MusaArch;

MusaArch::MusaArch() {
  fromStr(MUSA_ARCH_BIN, bin);
  fromStr(MUSA_ARCH_PTX, ptx);
  fromStr(MUSA_ARCH_FEATURES, features);
}

bool MusaArch::builtWith(FeatureSet feature_set) const {
  return !features.empty() && (features.back() >= feature_set);
}

bool MusaArch::hasPtx(int major, int minor) const {
  return std::find(ptx.begin(), ptx.end(), major * 10 + minor) != ptx.end();
}

bool MusaArch::hasBin(int major, int minor) const {
  return std::find(bin.begin(), bin.end(), major * 10 + minor) != bin.end();
}

bool MusaArch::hasEqualOrLessPtx(int major, int minor) const {
  return !ptx.empty() && (ptx.front() <= major * 10 + minor);
}

bool MusaArch::hasEqualOrGreaterPtx(int major, int minor) const {
  return !ptx.empty() && (ptx.back() >= major * 10 + minor);
}

bool MusaArch::hasEqualOrGreaterBin(int major, int minor) const {
  return !bin.empty() && (bin.back() >= major * 10 + minor);
}

void MusaArch::fromStr(const char* set_as_str, std::vector<int>& arr) {
  arr.clear();

  const size_t len = strlen(set_as_str);

  size_t pos = 0;
  while (pos < len) {
    if (isspace(set_as_str[pos])) {
      ++pos;
    } else {
      int cur_value;
      int chars_read;
      int args_read = sscanf(set_as_str + pos, "%d%n", &cur_value, &chars_read);
      CV_Assert(args_read == 1);

      arr.push_back(cur_value);
      pos += chars_read;
    }
  }

  std::sort(arr.begin(), arr.end());
}
}  // namespace

#endif

bool cv::musa::TargetArchs::builtWith(cv::musa::FeatureSet feature_set) {
#ifndef HAVE_MUSA
  CV_UNUSED(feature_set);
  throw_no_musa();
#else
  return MusaArch.builtWith(feature_set);
#endif
}

bool cv::musa::TargetArchs::hasPtx(int major, int minor) {
#ifndef HAVE_MUSA
  CV_UNUSED(major);
  CV_UNUSED(minor);
  throw_no_musa();
#else
  return MusaArch.hasPtx(major, minor);
#endif
}

bool cv::musa::TargetArchs::hasBin(int major, int minor) {
#ifndef HAVE_MUSA
  CV_UNUSED(major);
  CV_UNUSED(minor);
  throw_no_musa();
#else
  return MusaArch.hasBin(major, minor);
#endif
}

bool cv::musa::TargetArchs::hasEqualOrLessPtx(int major, int minor) {
#ifndef HAVE_MUSA
  CV_UNUSED(major);
  CV_UNUSED(minor);
  throw_no_musa();
#else
  return MusaArch.hasEqualOrLessPtx(major, minor);
#endif
}

bool cv::musa::TargetArchs::hasEqualOrGreaterPtx(int major, int minor) {
#ifndef HAVE_MUSA
  CV_UNUSED(major);
  CV_UNUSED(minor);
  throw_no_musa();
#else
  return MusaArch.hasEqualOrGreaterPtx(major, minor);
#endif
}

bool cv::musa::TargetArchs::hasEqualOrGreaterBin(int major, int minor) {
#ifndef HAVE_MUSA
  CV_UNUSED(major);
  CV_UNUSED(minor);
  throw_no_musa();
#else
  return MusaArch.hasEqualOrGreaterBin(major, minor);
#endif
}

////////////////////////////////////////////////////////////////////////
// DeviceInfo

#ifdef HAVE_MUSA

namespace {
class DeviceProps {
 public:
  DeviceProps();

  const musaDeviceProp* get(int devID) const;

 private:
  std::vector<musaDeviceProp> props_;
};

DeviceProps::DeviceProps() {
  int count = getMusaEnabledDeviceCount();

  if (count > 0) {
    props_.resize(count);

    for (int devID = 0; devID < count; ++devID) {
      musaSafeCall(musaGetDeviceProperties(&props_[devID], devID));
    }
  }
}

const musaDeviceProp* DeviceProps::get(int devID) const {
  CV_Assert(static_cast<size_t>(devID) < props_.size());

  return &props_[devID];
}

DeviceProps& deviceProps() {
  static DeviceProps props;
  return props;
}
}  // namespace

#endif

const char* cv::musa::DeviceInfo::name() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->name;
#endif
}

size_t cv::musa::DeviceInfo::totalGlobalMem() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->totalGlobalMem;
#endif
}

size_t cv::musa::DeviceInfo::sharedMemPerBlock() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->sharedMemPerBlock;
#endif
}

int cv::musa::DeviceInfo::regsPerBlock() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->regsPerBlock;
#endif
}

int cv::musa::DeviceInfo::warpSize() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->warpSize;
#endif
}

size_t cv::musa::DeviceInfo::memPitch() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->memPitch;
#endif
}

int cv::musa::DeviceInfo::maxThreadsPerBlock() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->maxThreadsPerBlock;
#endif
}

Vec3i cv::musa::DeviceInfo::maxThreadsDim() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec3i(deviceProps().get(device_id_)->maxThreadsDim);
#endif
}

Vec3i cv::musa::DeviceInfo::maxGridSize() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec3i(deviceProps().get(device_id_)->maxGridSize);
#endif
}

int cv::musa::DeviceInfo::clockRate() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->clockRate;
#endif
}

size_t cv::musa::DeviceInfo::totalConstMem() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->totalConstMem;
#endif
}

int cv::musa::DeviceInfo::majorVersion() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->major;
#endif
}

int cv::musa::DeviceInfo::minorVersion() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->minor;
#endif
}

size_t cv::musa::DeviceInfo::textureAlignment() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->textureAlignment;
#endif
}

size_t cv::musa::DeviceInfo::texturePitchAlignment() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->texturePitchAlignment;
#endif
}

int cv::musa::DeviceInfo::multiProcessorCount() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->multiProcessorCount;
#endif
}

bool cv::musa::DeviceInfo::kernelExecTimeoutEnabled() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->kernelExecTimeoutEnabled != 0;
#endif
}

bool cv::musa::DeviceInfo::integrated() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->integrated != 0;
#endif
}

bool cv::musa::DeviceInfo::canMapHostMemory() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->canMapHostMemory != 0;
#endif
}

DeviceInfo::ComputeMode cv::musa::DeviceInfo::computeMode() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  static const ComputeMode tbl[] = {ComputeModeDefault, ComputeModeExclusive,
                                    ComputeModeProhibited,
                                    ComputeModeExclusiveProcess};

  return tbl[deviceProps().get(device_id_)->computeMode];
#endif
}

int cv::musa::DeviceInfo::maxTexture1D() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->maxTexture1D;
#endif
}

int cv::musa::DeviceInfo::maxTexture1DMipmap() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->maxTexture1DMipmap;
#endif
}

int cv::musa::DeviceInfo::maxTexture1DLinear() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->maxTexture1DLinear;
#endif
}

Vec2i cv::musa::DeviceInfo::maxTexture2D() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec2i(deviceProps().get(device_id_)->maxTexture2D);
#endif
}

Vec2i cv::musa::DeviceInfo::maxTexture2DMipmap() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec2i(deviceProps().get(device_id_)->maxTexture2DMipmap);
#endif
}

Vec3i cv::musa::DeviceInfo::maxTexture2DLinear() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec3i(deviceProps().get(device_id_)->maxTexture2DLinear);
#endif
}

Vec2i cv::musa::DeviceInfo::maxTexture2DGather() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec2i(deviceProps().get(device_id_)->maxTexture2DGather);
#endif
}

Vec3i cv::musa::DeviceInfo::maxTexture3D() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec3i(deviceProps().get(device_id_)->maxTexture3D);
#endif
}

int cv::musa::DeviceInfo::maxTextureCubemap() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->maxTextureCubemap;
#endif
}

Vec2i cv::musa::DeviceInfo::maxTexture1DLayered() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec2i(deviceProps().get(device_id_)->maxTexture1DLayered);
#endif
}

Vec3i cv::musa::DeviceInfo::maxTexture2DLayered() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec3i(deviceProps().get(device_id_)->maxTexture2DLayered);
#endif
}

Vec2i cv::musa::DeviceInfo::maxTextureCubemapLayered() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec2i(deviceProps().get(device_id_)->maxTextureCubemapLayered);
#endif
}

int cv::musa::DeviceInfo::maxSurface1D() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->maxSurface1D;
#endif
}

Vec2i cv::musa::DeviceInfo::maxSurface2D() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec2i(deviceProps().get(device_id_)->maxSurface2D);
#endif
}

Vec3i cv::musa::DeviceInfo::maxSurface3D() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec3i(deviceProps().get(device_id_)->maxSurface3D);
#endif
}

Vec2i cv::musa::DeviceInfo::maxSurface1DLayered() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec2i(deviceProps().get(device_id_)->maxSurface1DLayered);
#endif
}

Vec3i cv::musa::DeviceInfo::maxSurface2DLayered() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec3i(deviceProps().get(device_id_)->maxSurface2DLayered);
#endif
}

int cv::musa::DeviceInfo::maxSurfaceCubemap() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->maxSurfaceCubemap;
#endif
}

Vec2i cv::musa::DeviceInfo::maxSurfaceCubemapLayered() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return Vec2i(deviceProps().get(device_id_)->maxSurfaceCubemapLayered);
#endif
}

size_t cv::musa::DeviceInfo::surfaceAlignment() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->surfaceAlignment;
#endif
}

bool cv::musa::DeviceInfo::concurrentKernels() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->concurrentKernels != 0;
#endif
}

bool cv::musa::DeviceInfo::ECCEnabled() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->ECCEnabled != 0;
#endif
}

int cv::musa::DeviceInfo::pciBusID() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->pciBusID;
#endif
}

int cv::musa::DeviceInfo::pciDeviceID() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->pciDeviceID;
#endif
}

int cv::musa::DeviceInfo::pciDomainID() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->pciDomainID;
#endif
}

bool cv::musa::DeviceInfo::tccDriver() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->tccDriver != 0;
#endif
}

int cv::musa::DeviceInfo::asyncEngineCount() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->asyncEngineCount;
#endif
}

bool cv::musa::DeviceInfo::unifiedAddressing() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->unifiedAddressing != 0;
#endif
}

int cv::musa::DeviceInfo::memoryClockRate() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->memoryClockRate;
#endif
}

int cv::musa::DeviceInfo::memoryBusWidth() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->memoryBusWidth;
#endif
}

int cv::musa::DeviceInfo::l2CacheSize() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->l2CacheSize;
#endif
}

int cv::musa::DeviceInfo::maxThreadsPerMultiProcessor() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  return deviceProps().get(device_id_)->maxThreadsPerMultiProcessor;
#endif
}

void cv::musa::DeviceInfo::queryMemory(size_t& _totalMemory,
                                       size_t& _freeMemory) const {
#ifndef HAVE_MUSA
  CV_UNUSED(_totalMemory);
  CV_UNUSED(_freeMemory);
  throw_no_musa();
#else
  int prevDeviceID = getDevice();
  if (prevDeviceID != device_id_) setDevice(device_id_);

  musaSafeCall(musaMemGetInfo(&_freeMemory, &_totalMemory));

  if (prevDeviceID != device_id_) setDevice(prevDeviceID);
#endif
}

bool cv::musa::DeviceInfo::isCompatible() const {
#ifndef HAVE_MUSA
  throw_no_musa();
#else
  // Check PTX compatibility
  // TODO: add back soon!
  /*
  if (TargetArchs::hasEqualOrLessPtx(majorVersion(), minorVersion()))
    return true;

  // Check BIN compatibility
  for (int i = minorVersion(); i >= 0; --i)
    if (TargetArchs::hasBin(majorVersion(), i)) return true;

  return false;
  */
  return true;
#endif
}

////////////////////////////////////////////////////////////////////////
// print info

#ifdef HAVE_MUSA

namespace {
int convertSMVer2Cores(int major, int minor) {
  // Defines for GPU Architecture types (using the SM version to determine the #
  // of cores per SM
  typedef struct {
    int SM;  // 0xMm (hexadecimal notation), M = SM Major version, and m = SM
             // minor version
    int Cores;
  } SMtoCores;

  SMtoCores gpuArchCoresPerSM[] = {{0x10, 8},   {0x11, 8},   {0x12, 8},
                                   {0x13, 8},   {0x20, 32},  {0x21, 48},
                                   {0x30, 192}, {0x35, 192}, {-1, -1}};

  int index = 0;
  while (gpuArchCoresPerSM[index].SM != -1) {
    if (gpuArchCoresPerSM[index].SM == ((major << 4) + minor))
      return gpuArchCoresPerSM[index].Cores;
    index++;
  }

  return -1;
}
}  // namespace

#endif

void cv::musa::printMusaDeviceInfo(int device) {
#ifndef HAVE_MUSA
  CV_UNUSED(device);
  throw_no_musa();
#else
  int count = getMusaEnabledDeviceCount();
  bool valid = (device >= 0) && (device < count);

  int beg = valid ? device : 0;
  int end = valid ? device + 1 : count;

  printf(
      "*** MUSA Device Query (Runtime API) version (MUSART static linking) *** "
      "\n\n");
  printf("Device count: %d\n", count);

  int driverVersion = 0, runtimeVersion = 0;
  musaSafeCall(musaDriverGetVersion(&driverVersion));
  musaSafeCall(musaRuntimeGetVersion(&runtimeVersion));

  const char* computeMode[] = {
      "Default (multiple host threads can use ::musaSetDevice() with device "
      "simultaneously)",
      "Exclusive (only one host thread in one process is able to use "
      "::musaSetDevice() with this device)",
      "Prohibited (no host thread can use ::musaSetDevice() with this device)",
      "Exclusive Process (many threads in one process is able to use "
      "::musaSetDevice() with this device)",
      "Unknown",
      NULL};

  for (int dev = beg; dev < end; ++dev) {
    musaDeviceProp prop;
    musaSafeCall(musaGetDeviceProperties(&prop, dev));

    printf("\nDevice %d: \"%s\"\n", dev, prop.name);
    printf("  MUSA Driver Version / Runtime Version          %d.%d / %d.%d\n",
           driverVersion / 1000, driverVersion % 100, runtimeVersion / 1000,
           runtimeVersion % 100);
    printf("  MUSA Capability Major/Minor version number:    %d.%d\n",
           prop.major, prop.minor);
    printf(
        "  Total amount of global memory:                 %.0f MBytes (%llu "
        "bytes)\n",
        (float)prop.totalGlobalMem / 1048576.0f,
        (unsigned long long)prop.totalGlobalMem);

    int cores = convertSMVer2Cores(prop.major, prop.minor);
    if (cores > 0)
      printf(
          "  (%2d) Multiprocessors x (%2d) MUSA Cores/MP:     %d MUSA Cores\n",
          prop.multiProcessorCount, cores, cores * prop.multiProcessorCount);

    printf("  GPU Clock Speed:                               %.2f GHz\n",
           prop.clockRate * 1e-6f);

    printf(
        "  Max Texture Dimension Size (x,y,z)             1D=(%d), 2D=(%d,%d), "
        "3D=(%d,%d,%d)\n",
        prop.maxTexture1D, prop.maxTexture2D[0], prop.maxTexture2D[1],
        prop.maxTexture3D[0], prop.maxTexture3D[1], prop.maxTexture3D[2]);
    printf(
        "  Max Layered Texture Size (dim) x layers        1D=(%d) x %d, "
        "2D=(%d,%d) x %d\n",
        prop.maxTexture1DLayered[0], prop.maxTexture1DLayered[1],
        prop.maxTexture2DLayered[0], prop.maxTexture2DLayered[1],
        prop.maxTexture2DLayered[2]);

    printf("  Total amount of constant memory:               %u bytes\n",
           (int)prop.totalConstMem);
    printf("  Total amount of shared memory per block:       %u bytes\n",
           (int)prop.sharedMemPerBlock);
    printf("  Total number of registers available per block: %d\n",
           prop.regsPerBlock);
    printf("  Warp size:                                     %d\n",
           prop.warpSize);
    printf("  Maximum number of threads per block:           %d\n",
           prop.maxThreadsPerBlock);
    printf("  Maximum sizes of each dimension of a block:    %d x %d x %d\n",
           prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
    printf("  Maximum sizes of each dimension of a grid:     %d x %d x %d\n",
           prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
    printf("  Maximum memory pitch:                          %u bytes\n",
           (int)prop.memPitch);
    printf("  Texture alignment:                             %u bytes\n",
           (int)prop.textureAlignment);

    printf(
        "  Concurrent copy and execution:                 %s with %d copy "
        "engine(s)\n",
        (prop.deviceOverlap ? "Yes" : "No"), prop.asyncEngineCount);
    printf("  Run time limit on kernels:                     %s\n",
           prop.kernelExecTimeoutEnabled ? "Yes" : "No");
    printf("  Integrated GPU sharing Host Memory:            %s\n",
           prop.integrated ? "Yes" : "No");
    printf("  Support host page-locked memory mapping:       %s\n",
           prop.canMapHostMemory ? "Yes" : "No");

    printf("  Concurrent kernel execution:                   %s\n",
           prop.concurrentKernels ? "Yes" : "No");
    printf("  Alignment requirement for Surfaces:            %s\n",
           prop.surfaceAlignment ? "Yes" : "No");
    printf("  Device has ECC support enabled:                %s\n",
           prop.ECCEnabled ? "Yes" : "No");
    printf("  Device is using TCC driver mode:               %s\n",
           prop.tccDriver ? "Yes" : "No");
    printf("  Device supports Unified Addressing (UVA):      %s\n",
           prop.unifiedAddressing ? "Yes" : "No");
    printf("  Device PCI Bus ID / PCI location ID:           %d / %d\n",
           prop.pciBusID, prop.pciDeviceID);
    printf("  Compute Mode:\n");
    printf("      %s \n", computeMode[prop.computeMode]);
  }

  printf("\n");
  printf("deviceQuery, MUSA Driver = MUSART");
  printf(", MUSA Driver Version  = %d.%d", driverVersion / 1000,
         driverVersion % 100);
  printf(", MUSA Runtime Version = %d.%d", runtimeVersion / 1000,
         runtimeVersion % 100);
  printf(", NumDevs = %d\n\n", count);

  fflush(stdout);
#endif
}

void cv::musa::printShortMusaDeviceInfo(int device) {
#ifndef HAVE_MUSA
  CV_UNUSED(device);
  throw_no_musa();
#else
  int count = getMusaEnabledDeviceCount();
  bool valid = (device >= 0) && (device < count);

  int beg = valid ? device : 0;
  int end = valid ? device + 1 : count;

  int driverVersion = 0, runtimeVersion = 0;
  musaSafeCall(musaDriverGetVersion(&driverVersion));
  musaSafeCall(musaRuntimeGetVersion(&runtimeVersion));

  for (int dev = beg; dev < end; ++dev) {
    musaDeviceProp prop;
    musaSafeCall(musaGetDeviceProperties(&prop, dev));

    const char* arch_str = prop.major < 2 ? " (not Fermi)" : "";
    printf("Device %d:  \"%s\"  %.0fMb", dev, prop.name,
           (float)prop.totalGlobalMem / 1048576.0f);
    printf(", sm_%d%d%s", prop.major, prop.minor, arch_str);

    int cores = convertSMVer2Cores(prop.major, prop.minor);
    if (cores > 0) printf(", %d cores", cores * prop.multiProcessorCount);

    printf(", Driver/Runtime ver.%d.%d/%d.%d\n", driverVersion / 1000,
           driverVersion % 100, runtimeVersion / 1000, runtimeVersion % 100);
  }

  fflush(stdout);
#endif
}

////////////////////////////////////////////////////////////////////////
// Error handling

#ifdef HAVE_MUSA

namespace {
#define error_entry(entry) \
  { entry, #entry }

struct ErrorEntry {
  int code;
  const char* str;
};

struct ErrorEntryComparer {
  int code;
  ErrorEntryComparer(int code_) : code(code_) {}
  bool operator()(const ErrorEntry& e) const { return e.code == code; }
};

const ErrorEntry mupp_errors[] = {
#if defined(_MSC_VER)
    error_entry(NPP_NOT_SUFFICIENT_COMPUTE_CAPABILITY),
#endif

    error_entry(MUPP_INVALID_HOST_POINTER_ERROR),
    error_entry(MUPP_INVALID_DEVICE_POINTER_ERROR),
    error_entry(MUPP_LUT_PALETTE_BITSIZE_ERROR),
    error_entry(MUPP_ZC_MODE_NOT_SUPPORTED_ERROR),
    error_entry(MUPP_MEMFREE_ERROR),
    error_entry(MUPP_MEMSET_ERROR),
    error_entry(MUPP_QUALITY_INDEX_ERROR),
    error_entry(MUPP_HISTOGRAM_NUMBER_OF_LEVELS_ERROR),
    error_entry(MUPP_CHANNEL_ORDER_ERROR),
    error_entry(MUPP_ZERO_MASK_VALUE_ERROR),
    error_entry(MUPP_QUADRANGLE_ERROR),
    error_entry(MUPP_RECTANGLE_ERROR),
    error_entry(MUPP_COEFFICIENT_ERROR),
    error_entry(MUPP_NUMBER_OF_CHANNELS_ERROR),
    error_entry(MUPP_COI_ERROR),
    error_entry(MUPP_DIVISOR_ERROR),
    error_entry(MUPP_CHANNEL_ERROR),
    error_entry(MUPP_STRIDE_ERROR),
    error_entry(MUPP_ANCHOR_ERROR),
    error_entry(MUPP_MASK_SIZE_ERROR),
    error_entry(MUPP_MIRROR_FLIP_ERROR),
    error_entry(MUPP_MOMENT_00_ZERO_ERROR),
    error_entry(MUPP_THRESHOLD_NEGATIVE_LEVEL_ERROR),
    error_entry(MUPP_THRESHOLD_ERROR),
    error_entry(MUPP_CONTEXT_MATCH_ERROR),
    error_entry(MUPP_FFT_FLAG_ERROR),
    error_entry(MUPP_FFT_ORDER_ERROR),
    error_entry(MUPP_SCALE_RANGE_ERROR),
    error_entry(MUPP_DATA_TYPE_ERROR),
    error_entry(MUPP_OUT_OFF_RANGE_ERROR),
    error_entry(MUPP_DIVIDE_BY_ZERO_ERROR),
    error_entry(MUPP_MEMORY_ALLOCATION_ERR),
    error_entry(MUPP_RANGE_ERROR),
    error_entry(MUPP_BAD_ARGUMENT_ERROR),
    error_entry(MUPP_NO_MEMORY_ERROR),
    error_entry(MUPP_ERROR_RESERVED),
    error_entry(MUPP_NO_OPERATION_WARNING),
    error_entry(MUPP_DIVIDE_BY_ZERO_WARNING),
    error_entry(MUPP_WRONG_INTERSECTION_ROI_WARNING),
    error_entry(MUPP_NOT_SUPPORTED_MODE_ERROR),
    error_entry(MUPP_ROUND_MODE_NOT_SUPPORTED_ERROR),
    error_entry(MUPP_RESIZE_NO_OPERATION_ERROR),
    error_entry(MUPP_LUT_NUMBER_OF_LEVELS_ERROR),
    error_entry(MUPP_TEXTURE_BIND_ERROR),
    error_entry(MUPP_WRONG_INTERSECTION_ROI_ERROR),
    error_entry(MUPP_NOT_EVEN_STEP_ERROR),
    error_entry(MUPP_INTERPOLATION_ERROR),
    error_entry(MUPP_RESIZE_FACTOR_ERROR),
    error_entry(MUPP_HAAR_CLASSIFIER_PIXEL_MATCH_ERROR),
    error_entry(MUPP_MEMCPY_ERROR),
    error_entry(MUPP_ALIGNMENT_ERROR),
    error_entry(MUPP_STEP_ERROR),
    error_entry(MUPP_SIZE_ERROR),
    error_entry(MUPP_NULL_POINTER_ERROR),
    error_entry(MUPP_MUSA_KERNEL_EXECUTION_ERROR),
    error_entry(MUPP_NOT_IMPLEMENTED_ERROR),
    error_entry(MUPP_ERROR),
    error_entry(MUPP_NO_ERROR),
    error_entry(MUPP_SUCCESS),
    error_entry(MUPP_WRONG_INTERSECTION_QUAD_WARNING),
    error_entry(MUPP_MISALIGNED_DST_ROI_WARNING),
    error_entry(MUPP_AFFINE_QUAD_INCORRECT_WARNING),
    error_entry(MUPP_DOUBLE_SIZE_WARNING)};

const size_t mupp_error_num = sizeof(mupp_errors) / sizeof(mupp_errors[0]);

const ErrorEntry mu_errors[] = {
    error_entry(MUSA_SUCCESS),
    error_entry(MUSA_ERROR_INVALID_VALUE),
    error_entry(MUSA_ERROR_OUT_OF_MEMORY),
    error_entry(MUSA_ERROR_NOT_INITIALIZED),
    error_entry(MUSA_ERROR_DEINITIALIZED),
    error_entry(MUSA_ERROR_PROFILER_DISABLED),
    error_entry(MUSA_ERROR_PROFILER_NOT_INITIALIZED),
    error_entry(MUSA_ERROR_PROFILER_ALREADY_STARTED),
    error_entry(MUSA_ERROR_PROFILER_ALREADY_STOPPED),
    error_entry(MUSA_ERROR_NO_DEVICE),
    error_entry(MUSA_ERROR_INVALID_DEVICE),
    error_entry(MUSA_ERROR_INVALID_IMAGE),
    error_entry(MUSA_ERROR_INVALID_CONTEXT),
    error_entry(MUSA_ERROR_CONTEXT_ALREADY_CURRENT),
    error_entry(MUSA_ERROR_MAP_FAILED),
    error_entry(MUSA_ERROR_UNMAP_FAILED),
    error_entry(MUSA_ERROR_ARRAY_IS_MAPPED),
    error_entry(MUSA_ERROR_ALREADY_MAPPED),
    error_entry(MUSA_ERROR_NO_BINARY_FOR_GPU),
    error_entry(MUSA_ERROR_ALREADY_ACQUIRED),
    error_entry(MUSA_ERROR_NOT_MAPPED),
    error_entry(MUSA_ERROR_NOT_MAPPED_AS_ARRAY),
    error_entry(MUSA_ERROR_NOT_MAPPED_AS_POINTER),
    error_entry(MUSA_ERROR_ECC_UNCORRECTABLE),
    error_entry(MUSA_ERROR_UNSUPPORTED_LIMIT),
    error_entry(MUSA_ERROR_CONTEXT_ALREADY_IN_USE),
    error_entry(MUSA_ERROR_INVALID_SOURCE),
    error_entry(MUSA_ERROR_FILE_NOT_FOUND),
    error_entry(MUSA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND),
    error_entry(MUSA_ERROR_SHARED_OBJECT_INIT_FAILED),
    error_entry(MUSA_ERROR_OPERATING_SYSTEM),
    error_entry(MUSA_ERROR_INVALID_HANDLE),
    error_entry(MUSA_ERROR_NOT_FOUND),
    error_entry(MUSA_ERROR_NOT_READY),
    error_entry(MUSA_ERROR_LAUNCH_FAILED),
    error_entry(MUSA_ERROR_LAUNCH_OUT_OF_RESOURCES),
    error_entry(MUSA_ERROR_LAUNCH_TIMEOUT),
    error_entry(MUSA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING),
    error_entry(MUSA_ERROR_PEER_ACCESS_ALREADY_ENABLED),
    error_entry(MUSA_ERROR_PEER_ACCESS_NOT_ENABLED),
    error_entry(MUSA_ERROR_PRIMARY_CONTEXT_ACTIVE),
    error_entry(MUSA_ERROR_CONTEXT_IS_DESTROYED),
    error_entry(MUSA_ERROR_ASSERT),
    error_entry(MUSA_ERROR_TOO_MANY_PEERS),
    error_entry(MUSA_ERROR_HOST_MEMORY_ALREADY_REGISTERED),
    error_entry(MUSA_ERROR_HOST_MEMORY_NOT_REGISTERED),
    error_entry(MUSA_ERROR_UNKNOWN)};

const size_t mu_errors_num = sizeof(mu_errors) / sizeof(mu_errors[0]);

cv::String getErrorString(int code, const ErrorEntry* errors, size_t n) {
  size_t idx =
      std::find_if(errors, errors + n, ErrorEntryComparer(code)) - errors;

  const char* msg = (idx != n) ? errors[idx].str : "Unknown error code";
  cv::String str = cv::format("%s [Code = %d]", msg, code);

  return str;
}
}  // namespace

#endif

String cv::musa::getMUppErrorMessage(int code) {
#ifndef HAVE_MUSA
  CV_UNUSED(code);
  return String();
#else
  return getErrorString(code, mupp_errors, mupp_error_num);
#endif
}

String cv::musa::getMusaDriverApiErrorMessage(int code) {
#ifndef HAVE_MUSA
  CV_UNUSED(code);
  return String();
#else
  return getErrorString(code, mu_errors, mu_errors_num);
#endif
}
