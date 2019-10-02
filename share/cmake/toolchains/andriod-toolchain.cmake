# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(NDK /Applications/android-ndk-r5b)
set(NDK_LEVEL 9)
set(NDK_ARCH arm)
set(NDK_CPU_ABI armeabi-v7a) # armeabi, armeabi-v7a, x86

set(NDK_SYSROOT ${NDK}/platforms/android-${NDK_LEVEL}/arch-${NDK_ARCH})
set(NDK_TOOLCHAIN_PREFIX arm-linux-androideabi)
set(NDK_TOOLCHAIN_NAME ${NDK_TOOLCHAIN_PREFIX}-4.4.3)
set(NDK_TOOLCHAIN_SYSTEM darwin-x86)
set(NDK_TOOLCHAIN ${NDK}/toolchains/${NDK_TOOLCHAIN_NAME}/prebuilt/${NDK_TOOLCHAIN_SYSTEM})

set(CMAKE_FIND_ROOT_PATH "${NDK_TOOLCHAIN}" "${NDK_SYSROOT}" "${NDK}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_SYSTEM_NAME GNU)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR armv7-a)
set(CMAKE_OSX_ARCHITECTURES armv7-a)

set(CMAKE_C_COMPILER ${NDK_TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${NDK_TOOLCHAIN_PREFIX}-g++)

include_directories("${NDK}/sources/cxx-stl/gnu-libstdc++/libs/${NDK_CPU_ABI}/include")
include_directories("${NDK}/sources/cxx-stl/gnu-libstdc++/include")
include_directories("${NDK_SYSROOT}/usr/include")

# TODO: work out if we need any of these compile flags
# -DNDEBUG -MMD -MP -MF -fpic -ffunction-sections -funwind-tables
# -fstack-protector -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__
# -D__ARM_ARCH_5TE__ -mtune=xscale -msoft-float -mthumb
# -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -Wno-psabi
# -Wa,--noexecstack -Os -O2 -g

set(NDK_COMPILE_FLAGS "-DANDROID -march=${CMAKE_SYSTEM_PROCESSOR} -mfloat-abi=softfp")

# TODO: work out if we need any of these link flags
# -Wl,--gc-sections -Wl,-z,nocopyreloc -Wl,--no-undefined -Wl,-z,noexecstack
# -Wl,-rpath-link=${NDK_PLATFORM_ROOT}/usr/lib")

set(NDK_LINK_FLAGS "-Wl,--fix-cortex-a8 --sysroot=${NDK_SYSROOT} -L${NDK_SYSROOT}/usr/lib -lsupc++ -lstdc++ -lm -lc -ldl")

add_definitions(${NDK_COMPILE_FLAGS})
set(CMAKE_C_LINK_FLAGS ${NDK_LINK_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${NDK_LINK_FLAGS})

set(CMAKE_CROSSCOMPILING TRUE)
set(ANDROID TRUE)

set(OCIO_BUILD_SHARED TRUE)
set(OCIO_BUILD_STATIC FALSE)
set(OCIO_BUILD_UNITTESTS FALSE)
set(OCIO_BUILD_TESTBED FALSE)
set(OCIO_BUILD_APPS FALSE)
set(OCIO_BUILD_NUKE FALSE)
set(OCIO_BUILD_PYGLUE FALSE)
set(OCIO_BUILD_JNIGLUE TRUE)
set(OCIO_BUILD_SSE FALSE)
