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
        src/addr.cpp
        src/address_ip4.cpp
        src/address_ip6.cpp
        src/address_link.cpp
        src/dummy_worker.cpp
        src/if_worker.cpp
        src/interface.cpp
        src/main.cpp
        src/nmcommand_data.cpp
        src/nmconfig.cpp
        src/nmdaemon.cpp
        src/nmexception.cpp
        src/route_worker.cpp
        src/system_worker.cpp
        src/wpa_worker.cpp
        src/tool.cpp
        src/rcconf.cpp
        src/loguru/loguru.cpp
        src/inifile/inifile.cpp
        )

target_link_libraries(nmdaemon ${PTHREAD_LIBRARY} ${D_LIBRARY} ${KVM_LIBRARY} sockpp)
