cmake_minimum_required(VERSION 3.0)

project(psp2shell_m)
include("${VITASDK}/share/vita.cmake" REQUIRED)

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

set(SRC_FILES
        source/p2s_main.c
        source/p2s_cmd_parser.c
        source/p2s_utility.c
        source/file.c
        source/p2s_io.c
        source/p2s_module.c
        source/p2s_thread.c
        ../common/p2s_cmd.c
        ../common/p2s_msg.c
        )

set(INC_FILES
        ../common
        include
        )

set(CFLAGS -Wall -O2 -std=c99 -D__PSP2__ -D__USB__
        -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format
        -Wno-strict-aliasing -Wno-attributes -ffreestanding
        -DVERSION=\"${BUILD_DATE}\"
        )

set(LDFLAGS
        taihen_stub
        SceLibKernel_stub
        SceKernelThreadMgr_stub
        SceKernelModulemgr_stub
        SceSysmodule_stub
        SceSysmem_stub
        SceIofilemgr_stub
        SceNet_stub_weak
        SceNetCtl_stub_weak
        SceAppMgr_stub
        ScePower_stub
        k
        gcc
        -nostdlib
        )

# wifi module
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS})
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(${PROJECT_NAME} ${LDFLAGS}
            ${CMAKE_BINARY_DIR}/psp2shell_k/psp2shell_k-stubs/libpsp2shell_k_stub.a)
else ()
    target_link_libraries(${PROJECT_NAME} ${LDFLAGS})
endif ()

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_m.yml
        UNSAFE)

# usb module
add_executable(${PROJECT_NAME}_usb ${SRC_FILES})
target_include_directories(${PROJECT_NAME}_usb PRIVATE ${INC_FILES} ../psp2shell_k/usbhostfs)
target_compile_options(${PROJECT_NAME}_usb PRIVATE ${CFLAGS} -D__USB__)
target_link_libraries(${PROJECT_NAME}_usb ${LDFLAGS}
        ${CMAKE_BINARY_DIR}/psp2shell_k/psp2shell_k__usb-stubs/libpsp2shell_k_usb_stub.a)
vita_create_self(${PROJECT_NAME}_usb.suprx ${PROJECT_NAME}_usb
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_m.yml
        UNSAFE)