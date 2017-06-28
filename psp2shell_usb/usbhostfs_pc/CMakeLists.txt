cmake_minimum_required(VERSION 3.0)

set(CMAKE_SYSTEM_NAME "Linux")

project(usbhostfs_pc)

add_executable(${PROJECT_NAME}
        main.c
        hostfs.c
        #source/main.c
        #source/cmd.c
        #source/utility.c
        #../common/p2s_cmd.c
        )

target_compile_options(${PROJECT_NAME} PRIVATE
        #-DREADLINE_SHELL
        -DPC_SIDE
        -D_FILE_OFFSET_BITS=64
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        include
        ../common
        ../usbhostfs
        )

target_link_libraries(${PROJECT_NAME}
        usb
        readline
        pthread
        )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)