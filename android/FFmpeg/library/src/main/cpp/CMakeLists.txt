cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -DANDROID_NDK -DDISABLE_IMPORTGL")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -UNDEBUG")

#SDL2 build options
set(EXTRA_LIBS android log dl GLESv2)
set(SDL_STATIC ON)
set(STDC_HEADERS 1)
set(SDL_SHARED ON)
set(CMAKE_SYSTEM_NAME Linux)
include(CMakeForceCompiler)
#set(HAVE_SDL_TIMERS TRUE)
option(OPT_DEF_LIBC "" ON)
set(ANDROID TRUE)
#add SDL2 library
add_subdirectory(SDL2)

#add libyuv library
add_subdirectory(libyuv)

#add ffmpeg header
set(FFMPEG_HEADER ${CMAKE_CURRENT_LIST_DIR}/../../../../../../tools/ffmpeg-build/${ANDROID_ABI}/output/include)
include_directories(${FFMPEG_HEADER})
#add SDL2 header
include_directories(${PROJECT_SOURCE_DIR}/SDL2/include)
#add libyuv header
include_directories(${PROJECT_SOURCE_DIR}/libyuv/include)

set(wlanjie_SRCS ffmpeg.c
                    filter.c
                    openfile.c
                    utils.c
                    player.c
                    SDL_sample.c
                    wlanjie_jni.c)
add_library(wlanjie SHARED ${wlanjie_SRCS})

message(STATUS "cmake build dir : ${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "ffmpeg lib: ${FFMPEG_LIB_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

#link SDL2 library
set(SDL2_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/../../obj)
add_library(SDL2_LIB SHARED IMPORTED)
set_target_properties(SDL2_LIB PROPERTIES IMPORTED_LOCATION ${SDL2_LIB_DIR}/${ANDROID_ABI}/libSDL2.so)

#link libyuv library
set(YUV_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/libyuv)
add_library(YUV_LIB STATIC IMPORTED)
set_target_properties(YUV_LIB PROPERTIES IMPORTED_LOCATION ${YUV_LIB_DIR}/libyuv.a)

#link ffmpeg library
set(FFMPEG_LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/../jniLibs)
add_library(ffmpeg SHARED IMPORTED)
set_target_properties(ffmpeg PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIB_DIR}/${ANDROID_ABI}/libffmpeg.so)

target_link_libraries(wlanjie SDL2_LIB YUV_LIB android log dl m GLESv1_CM ffmpeg)