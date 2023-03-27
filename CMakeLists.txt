cmake_minimum_required(VERSION 3.16)
project(TinyHttpServer)
set(CMAKE_CXX_STANDARD 20)

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
        Uint.h
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
        Uint.h
        ViaCoroutine.h
        Traits.h
        Connection.h
        HttpRequest.h
        HttpResponse.h
        DetachedCoroutine.h
        Condition.h)
target_link_libraries(TinyHttpClient Threads::Threads)