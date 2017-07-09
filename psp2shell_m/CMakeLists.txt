cmake_minimum_required(VERSION 3.0)

project(psp2shellusb_m)
include("${VITASDK}/share/vita.cmake" REQUIRED)

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

set(SRC_FILES
        source/p2s_main.c
        source/p2s_utility.c
        source/p2s_thread.c
        source/p2s_cmd_parser.c
        ${CMAKE_SOURCE_DIR}/common/p2s_module.c
        ${CMAKE_SOURCE_DIR}/common/p2s_cmd.c
        ${CMAKE_SOURCE_DIR}/common/p2s_msg.c
        )

set(INC_FILES
        ${CMAKE_SOURCE_DIR}/psp2shell_k
        ${CMAKE_SOURCE_DIR}/common
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

# usb module
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_FILES} ../psp2shell_k/usbhostfs)
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS} -D__USB__)
target_link_libraries(${PROJECT_NAME} ${LDFLAGS}
        ${CMAKE_BINARY_DIR}/psp2shell_k/psp2shellusb_k-stubs/libpsp2shellusb_k_stub.a)
vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shellusb_m.yml
        UNSAFE)
