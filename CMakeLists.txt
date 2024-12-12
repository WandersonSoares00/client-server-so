cmake_minimum_required(VERSION 3.25)

project(server)

include_directories(include)

add_subdirectory(src)

add_executable(${PROJECT_NAME} ${SRC_FILES})

add_executable(analyst src/analyst.c)

add_executable(client src/client.c)
