cmake_minimum_required(VERSION 3.0)

set(CMAKE_SYSTEM_NAME "Linux")

project(psp2shell_cli)

add_executable(${PROJECT_NAME}
        source/main.c
        source/cmd.c
        source/utility.c
        ../common/source/psp2cmd.c)

target_include_directories(${PROJECT_NAME} PRIVATE
        include
        ../common/include
        )

target_link_libraries(${PROJECT_NAME}
        readline
        termcap
        pthread
        )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
