cmake_minimum_required(VERSION 3.16)
project(TinyHttpServer)
set(CMAKE_CXX_STANDARD 20)

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

add_executable(TinyHttpServer Server.cpp
        IoContextPool.h
        AsioCoroutineUtil.h
        Executor.h
        Lazy.h
        SyncAwait.h
        Common.h
        Try.h
        ViaCoroutine.h
        Traits.h
        Connection.h
        HttpRequest.h
        HttpResponse.h
        DetachedCoroutine.h
        Condition.h)
target_link_libraries(TinyHttpServer Threads::Threads)

add_executable(TinyHttpClient Client.cpp
        IoContextPool.h
        AsioCoroutineUtil.h
        Executor.h
        Lazy.h
        SyncAwait.h
        Common.h
        Try.h
        ViaCoroutine.h
        Traits.h
        Connection.h
        HttpRequest.h
        HttpResponse.h
        DetachedCoroutine.h
        Condition.h)
target_link_libraries(TinyHttpClient Threads::Threads)