cmake_minimum_required(VERSION 2.8)
project( webcam )
# Find and include OpenCV
find_package(OpenCV REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )
# Find and include pthread
find_package(Threads REQUIRED)
# Find libconfig
find_package(libconfig++ REQUIRED)
include_directories( ${libconfig++_INCLUDE_DIRS} )
# find include files and source files
include_directories(../include)
file(GLOB SOURCES "../sources/*.cpp")
# Add executable to the makefile
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY executable)
add_executable( webcam_server ${SOURCES} )
target_link_libraries( webcam_server ${OpenCV_LIBS} Threads::Threads ${LIBCONFIG++_LIBRARIES})
