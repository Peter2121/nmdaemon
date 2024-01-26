cmake_minimum_required(VERSION 3.20)

project(nmdaemon)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(WORKERS_NUM 6)
cmake_policy(VERSION 3.20)

include_directories(SYSTEM /usr/local/include/)

string(TIMESTAMP TODAY "%Y%m%d%H%M")
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH ${TODAY})
set(SOFTWARE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

configure_file("src/config.h.in" ${CMAKE_CURRENT_SOURCE_DIR}/src/generated/config.h @ONLY)

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

set(WORKERS_INCLUDE_FILE src/generated/nmworkers.h)
file(REMOVE ${WORKERS_INCLUDE_FILE})

foreach(WORKER_HEADER ${WORKERS_HEADERS})
    file(APPEND ${WORKERS_INCLUDE_FILE} "#include \"../${WORKER_HEADER}\"\n")
endforeach()

set(WORKERS_CLASSES_FILE src/generated/nmworkers.cpp)
file(REMOVE ${WORKERS_CLASSES_FILE})

foreach(WORKER_CLASS ${WORKERS_CLASSES})
    file(APPEND ${WORKERS_CLASSES_FILE} "workers.push_back(new ${WORKER_CLASS}());\n")
endforeach()

# System libs (shared)
find_library(PTHREAD_LIBRARY pthread)
find_library(D_LIBRARY dl)
find_library(KVM_LIBRARY kvm)
find_library(80211_LIBRARY 80211)
find_library(JAIL_LIBRARY jail)

# Libs from ports (static)
#find_library(ASYNC_LIBRARY libasync++.a)
#find_library(SOCKPP_LIBRARY libsockpp.a)
find_package(Async++ REQUIRED)
find_package(sockpp REQUIRED)

add_compile_definitions(LOGURU_WITH_STREAMS=1)

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
    src/wpasocket.cpp
    src/jailparam.cpp
    src/resolvconf.cpp
    src/loguru/loguru.cpp
    src/inifile/inifile.cpp
    )

set(HEADERS
    src/addressbase.h
    src/addressgroup.h
    src/addressip4.h
    src/addressip6.h
    src/addresslink.h
    src/interface.h
    src/mediadesc.h
    src/mediastatus.h
    src/nmcommand.h
    src/nmcommanddata.h
    src/nmconfig.h
    src/nmdaemon.h
    src/nmexception.h
    src/nmjsonconst.h
    src/nmworkerbase.h
    src/nmworkerdummy.h
    src/nmworkerieee80211.h
    src/nmworkerif.h
    src/nmworkerroute.h
    src/nmworkersys.h
    src/nmworkerwpa.h
    src/rcconf.h
    src/tool.h
    src/wpasocket.h
    src/jailparam.h
    src/resolvconf.h
    src/generated/nmworkers.h
    src/generated/config.h
    )

set(SOURCES_ALL ${SOURCES} ${WORKERS_SOURCES})
add_executable(nmdaemon ${SOURCES_ALL} ${HEADERS})

target_link_libraries(nmdaemon ${PTHREAD_LIBRARY} ${D_LIBRARY} ${KVM_LIBRARY} ${80211_LIBRARY} ${JAIL_LIBRARY} async++-static Sockpp::sockpp-static)

include(GNUInstallDirs)
install(TARGETS nmdaemon DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS "rc.d/nmdaemon" DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rc.d)
install(FILES "doc/nmdaemon.conf" DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
install(FILES "doc/commands.md" "doc/commands.txt" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/nmdaemon)
