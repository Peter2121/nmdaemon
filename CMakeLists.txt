cmake_minimum_required(VERSION 3.18)

project(nmdaemon)

find_library(PTHREAD_LIBRARY pthread)
find_library(D_LIBRARY dl)
find_library(KVM_LIBRARY kvm)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(LOGURU_WITH_STREAMS=1)

include_directories("${CMAKE_CURRENT_LIST_DIR}/../sockpp/include")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../sockpp/build/generated")

add_library(sockpp STATIC IMPORTED)
set_property(TARGET sockpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_property(TARGET sockpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_property(TARGET sockpp PROPERTY IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_LIST_DIR}/../sockpp/build-debug/libsockpp.a")
set_property(TARGET sockpp PROPERTY IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/../sockpp/build/libsockpp.a")

add_executable(nmdaemon
        src/addressgroup.cpp
        src/addressip4.cpp
        src/addressip6.cpp
        src/addresslink.cpp
        src/nmworkerdummy.cpp
        src/nmworkerif.cpp
        src/interface.cpp
        src/main.cpp
        src/nmcommanddata.cpp
        src/nmconfig.cpp
        src/nmdaemon.cpp
        src/nmexception.cpp
        src/nmworkerroute.cpp
        src/nmworkersys.cpp
        src/wpa_worker.cpp
        src/tool.cpp
        src/rcconf.cpp
        src/loguru/loguru.cpp
        src/inifile/inifile.cpp
        )

target_link_libraries(nmdaemon ${PTHREAD_LIBRARY} ${D_LIBRARY} ${KVM_LIBRARY} sockpp)
