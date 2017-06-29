cmake_minimum_required(VERSION 3.0)

set(CMAKE_SYSTEM_NAME "Linux")

project(psp2shell_cli)

add_executable(${PROJECT_NAME}
        source/main.c
        source/cmd.c
        source/utility.c
        ../common/p2s_cmd.c
        ../common/p2s_msg.c
        )
target_include_directories(${PROJECT_NAME} PRIVATE
        include
        ../common
        )
target_link_libraries(${PROJECT_NAME}
        readline
        termcap
        pthread
        )

add_executable(${PROJECT_NAME}_usb
        source/main.c
        source/cmd.c
        source/utility.c
        ../common/p2s_cmd.c
        ../common/p2s_msg.c
        usbhostfs_pc/main.c
        usbhostfs_pc/hostfs.c
        )
target_compile_options(${PROJECT_NAME}_usb PRIVATE
        -D__USB__
        -DREADLINE_SHELL
        -DPC_SIDE
        -D_FILE_OFFSET_BITS=64
)
target_include_directories(${PROJECT_NAME}_usb PRIVATE
        include
        ../common
        ../psp2shell_k/usbhostfs
        usbhostfs_pc
        )
target_link_libraries(${PROJECT_NAME}_usb
        usb
        readline
        termcap
        pthread
        )
