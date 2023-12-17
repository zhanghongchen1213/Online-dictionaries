cmake_minimum_required(VERSION 3.26)
project(Database C)

set(CMAKE_C_STANDARD 11)



add_compile_options(-l sqlite3)
add_executable(Database sqlite3.c
        Client.c
        Server.c
        Tcp.c
        Tcp.h)

#find_package(SQLite3 REQUIRED)
#(${SqLite3_INCLUDE_DIRS})
#target_link_libraries(${PROJECT_NAME} ${SqLite3_LIBRARIES})