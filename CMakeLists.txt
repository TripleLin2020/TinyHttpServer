cmake_minimum_required(VERSION 3.16)
project(TinyHttpServer)
set(CMAKE_CXX_STANDARD 20)

include_directories(include)

set(CXX_FLAGS
#        -fno-omit-frame-pointer
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wold-style-cast
        -Woverloaded-virtual
        -Wpointer-arith
        -Wshadow
        -Wwrite-strings
        -march=native
#        -rdynamic
        -std=c++20)

find_package(Boost)
find_package(Threads)

add_executable(TinyHttpServer src/Server.cpp
        include/IoContextPool.h
        include/AsioCoroutineUtil.h
        include/Executor.h
        include/Lazy.h
        include/SyncAwait.h
        include/Common.h
        include/Try.h
        include/ViaCoroutine.h
        include/Traits.h
        include/Connection.h
        include/HttpRequest.h
        include/HttpResponse.h
        include/DetachedCoroutine.h
        include/Condition.h)
target_link_libraries(TinyHttpServer Threads::Threads)

add_executable(TinyHttpClient src/Client.cpp
        include/IoContextPool.h
        include/AsioCoroutineUtil.h
        include/Executor.h
        include/Lazy.h
        include/SyncAwait.h
        include/Common.h
        include/Try.h
        include/ViaCoroutine.h
        include/Traits.h
        include/Connection.h
        include/HttpRequest.h
        include/HttpResponse.h
        include/DetachedCoroutine.h
        include/Condition.h)
target_link_libraries(TinyHttpClient Threads::Threads)