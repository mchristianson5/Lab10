cmake_minimum_required(VERSION 3.5)

project(lab10 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(shared_sources
        ./shared/info.c
        ./shared/info.h
        )
find_package(Threads REQUIRED)

add_executable(server.out
        ${shared_sources}
        server/server.c
        server/server.h
        )
add_executable(client.out
        ${shared_sources}
        client/client.c
        client/main.c
        client/client.h
        )

target_link_libraries(server.out PRIVATE Threads::Threads)
set_target_properties(server.out PROPERTIES COMPILE_OPTIONS "-m32" LINK_FLAGS "-m32")
target_compile_options(server.out PRIVATE -Wall -W -pedantic -pipe -Wno-unused-parameter -ggdb)

set_target_properties(client.out  PROPERTIES COMPILE_OPTIONS "-m32" LINK_FLAGS "-m32")
target_compile_options(client.out PRIVATE -Wall -W -pedantic -pipe -Wno-unused-parameter -ggdb)
