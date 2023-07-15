cmake_minimum_required(VERSION 2.8)
project(rtspserver)

#Finding pkg-config is a helper tool
find_package(PkgConfig) 

#Using pkg-config to getting Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-rtsp-server-1.0)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
)

link_directories(
        ${GLIB_LIBRARY_DIRS}
        ${GSTREAMER_LIBRARY_DIRS}
)

add_executable(rtspserver Gstreamer.cpp)
#add_executable(rtspserver test-auth.c)

target_link_libraries(rtspserver ${GSTREAMER_LIBRARIES})
