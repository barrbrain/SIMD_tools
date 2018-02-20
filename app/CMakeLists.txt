cmake_minimum_required(VERSION 3.4.1)

add_library( native-lib
             SHARED
             src/main/cpp/native-lib.cpp
	     ../subavg_neon.c )

target_link_libraries( native-lib )