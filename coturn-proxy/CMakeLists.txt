cmake_minimum_required(VERSION 2.8)

project(coturn_proxy)

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -std=c++17 -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -std=c++17")

ADD_DEFINITIONS(-DLINUX)
ADD_DEFINITIONS(-DHAVE_STRDUP)
ADD_DEFINITIONS(-DNOLINUXIF)
ADD_DEFINITIONS(-DUSE_EPOLL)
ADD_DEFINITIONS(-fexceptions)


INCLUDE_DIRECTORIES("/usr/include/glib-2.0/")
INCLUDE_DIRECTORIES("/usr/lib/x86_64-linux-gnu/glib-2.0/include/")
INCLUDE_DIRECTORIES("./third/AMQP-CPP/include")
INCLUDE_DIRECTORIES("./third/coturn/include")
INCLUDE_DIRECTORIES("./third/coturn/include/turn")
INCLUDE_DIRECTORIES("./")

LINK_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/third/build/lib")

LINK_LIBRARIES(libamqpcpp.so)
LINK_LIBRARIES(libglib-2.0.so)
LINK_LIBRARIES(liblog4cxx.so)
LINK_LIBRARIES(libturnclient.a)
LINK_LIBRARIES(libboost_thread.a)
LINK_LIBRARIES(libboost_timer.a)
LINK_LIBRARIES(libboost_system.a)
LINK_LIBRARIES(libboost_chrono.a)
LINK_LIBRARIES(libboost_coroutine.a)
LINK_LIBRARIES(libboost_context.a)
LINK_LIBRARIES(libboost_regex.a)
LINK_LIBRARIES(libboost_filesystem.a)
LINK_LIBRARIES(libsocket.a)
LINK_LIBRARIES(crypto)
LINK_LIBRARIES(ssl)
LINK_LIBRARIES(pthread)
LINK_LIBRARIES(gnutls)
LINK_LIBRARIES(dl)
LINK_LIBRARIES(m)
LINK_LIBRARIES(z)

aux_source_directory(./src SRC_LIST)
aux_source_directory(./systhread SRC_LIST)
aux_source_directory(./lib SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})
