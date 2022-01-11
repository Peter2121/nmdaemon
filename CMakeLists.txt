cmake_minimum_required(VERSION 3.18)

project(nmdaemon)

string(TIMESTAMP TODAY "%Y%m%d%H%M")
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH ${TODAY})
set(SOFTWARE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(WORKERS_NUM 6)

configure_file("src/config.h.in" ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h @ONLY)

set(WORKERS_CLASSES
    NmWorkerDummy
    NmWorkerSys
    NmWorkerIf
    NmWorkerRoute
    NmWorkerWpa
    NmWorkerIeee80211
    )

set(WORKERS_HEADERS
    nmworkerdummy.h
    nmworkersys.h
    nmworkerif.h
    nmworkerroute.h
    nmworkerwpa.h
    nmworkerieee80211.h
    )

set(WORKERS_SOURCES
    src/nmworkerdummy.cpp
    src/nmworkersys.cpp
    src/nmworkerif.cpp
    src/nmworkerroute.cpp
    src/nmworkerwpa.cpp
    src/nmworkerieee80211.cpp
    )

find_library(PTHREAD_LIBRARY pthread)
find_library(D_LIBRARY dl)
find_library(KVM_LIBRARY kvm)
find_library(80211_LIBRARY 80211)

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

set(SOURCES
    src/main.cpp
    src/addressgroup.cpp
    src/addressip4.cpp
    src/addressip6.cpp
    src/addresslink.cpp
    src/interface.cpp
    src/nmcommanddata.cpp
    src/nmconfig.cpp
    src/nmdaemon.cpp
    src/nmexception.cpp
    src/tool.cpp
    src/rcconf.cpp
    src/loguru/loguru.cpp
    src/inifile/inifile.cpp
    )

set(SOURCES_ALL ${SOURCES} ${WORKERS_SOURCES})
add_executable(nmdaemon ${SOURCES_ALL})

target_link_libraries(nmdaemon ${PTHREAD_LIBRARY} ${D_LIBRARY} ${KVM_LIBRARY} ${80211_LIBRARY} sockpp)
