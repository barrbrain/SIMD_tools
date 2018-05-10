cmake_minimum_required(VERSION 3.4.1)

add_library( native-lib
             SHARED
             src/main/cpp/native-lib.cpp
	     ../cfl_neon.c
	     ../cfl.c
	     ../cfl_test.c )

target_link_libraries( native-lib )

set(ANDROID_NDK_ARCH "armv7")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=exynos-m1 -fno-slp-vectorize -mllvm -force-vector-width=1 -mllvm -force-vector-interleave=1 -O3")