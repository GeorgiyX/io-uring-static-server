cmake_minimum_required(VERSION 3.22)
project(io-uring-static-server)

set(CMAKE_CXX_STANDARD 20)

set(HEADERS include/server.h include/utils.h include/event_loop.h include/resources.h
        include/connection_state.h include/config.h include/connection_state.h include/config.h)
set(SOURCES src/main.cpp src/server.cpp src/utils.cpp src/connection_state.cpp
        src/connection_state.cpp src/event_loop.cpp src/resources.cpp
        src/config.cpp include/logger.h src/logger.cpp)

include_directories(include)
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} -luring -lpthread -lcds)
