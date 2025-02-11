if((NOT UNIX AND NOT CV_CLANG))
  message(STATUS "Currently MUSA application can only be compiled with clang.")
  return()
endif()

if(EXISTS /usr/local/musa)
  set(HAVE_MUSA 1)
  set(MUSA_INCLUDE_DIRS /usr/local/musa/include)

  macro(ocv_musa_compile VAR)
    set(musa_src)
    foreach(file ${ARGN})
      if(NOT ${file} MATCHES "cl$|hpp$|inc$|h$")
        # ocv_debug_message("${file}")
        list(APPEND musa_src ${file})
      endif()
    endforeach()

    # list(FILTER ARGN EXCLUDE REGEX "hpp$")
    # ocv_debug_message("ocv musa compile var: ${VAR} src: ${musa_src}")
    set_source_files_properties(${musa_src}
      PROPERTIES
      LANGUAGE CXX)
    add_library(${VAR} OBJECT ${musa_src})
    target_compile_options(${VAR} PRIVATE "--offload-arch=mp_21")
  endmacro()

  set(MUSA_LIBS_PATH "/usr/local/musa/lib")
  link_directories(${MUSA_LIBS_PATH})

  set(MUSA_mufft_LIBRARY "")
  if(WITH_MUFFT)
    set(HAVE_MUFFT 1)
    list(APPEND MUSA_mufft_LIBRARY "mufft;")
  endif()

  set(MUSA_mublas_LIBRARY "")
  if(WITH_MUBLAS)
    set(HAVE_MUBLAS 1)
    list(APPEND MUSA_mublas_LIBRARY "mublas;")
  endif()

  set(MUSA_LIBRARIES "musart;musa")
  # ocv_debug_message("MUSA_LIBRARIES: ${MUSA_LIBRARIES}")
  set(MUSA_mupp_LIBRARY "muppc;muppial;muppicc;muppidei;muppif;muppil;muppig;muppim;muppist;muppitc")
  # ocv_debug_message("MUSA_mupp_LIBRARY: ${MUSA_mupp_LIBRARY}")
  set(OPENCV_LINKER_LIBS ${OPENCV_LINKER_LIBS} ${MUSA_LIBRARIES} ${MUSA_mupp_LIBRARY} ${MUSA_mublas_LIBRARY} ${MUSA_mufft_LIBRARY})
  foreach(p ${MUSA_LIBS_PATH})
    if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja|JOM")
      set(OPENCV_LINKER_LIBS ${OPENCV_LINKER_LIBS} ${CMAKE_LIBRARY_PATH_FLAG}"${p}")
    else()
      set(OPENCV_LINKER_LIBS ${OPENCV_LINKER_LIBS} ${CMAKE_LIBRARY_PATH_FLAG}${p})
    endif()
  endforeach()

  ocv_debug_message("OPENCV_LINKER_LIBS after: ${OPENCV_LINKER_LIBS}")

  set(OPENCV_MUSA_ARCH_BIN "")
  set(OPENCV_MUSA_ARCH_PTX "")
  set(OPENCV_MUSA_ARCH_FEATURES "")
endif()
