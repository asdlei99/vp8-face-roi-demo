cmake_minimum_required(VERSION 3.5)
project(faceDetectionNTracking)

set(CMAKE_CXX_STANDARD 14)

file(COPY media/haarcascade_frontalface_default.xml DESTINATION ${PROJECT_BINARY_DIR})
file(COPY media/face.mp4 DESTINATION ${PROJECT_BINARY_DIR})

#VPX
INCLUDE(FindPkgConfig)
pkg_check_modules(vpx REQUIRED)
include_directories(${VPX_INCLUDE_DIRS})

# OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(src)
set(SOURCE_FILES src/main.cpp src/VideoFaceDetector.cpp src/VideoFaceDetector.h src/mp4recorder.cpp src/vp8encoder.cpp)
add_executable(vp8faceroi ${SOURCE_FILES})

target_link_libraries(vp8faceroi ${OpenCV_LIBS} libvpx.a libmp4v2.a pthread)
