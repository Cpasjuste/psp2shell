cmake_minimum_required(VERSION 3.0)

project(psp2shell_m)
include("${VITASDK}/share/vita.cmake" REQUIRED)

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

set(SRC_FILES
        source/main.c
        source/utility.c
        source/file.c
        source/module.c
        source/thread.c
        source/pool.c
        source/libmodule.c
        ../common/source/psp2cmd.c
        )

set(INC_FILES
        ../common/include
        include
        )

set(CFLAGS -Wall -O2 -std=c99 -DPSP2 -DMODULE
        -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format
        -Wno-strict-aliasing -Wno-attributes -ffreestanding
        -DVERSION=\"${BUILD_DATE}\"
        )
#-DDEBUG=1

# user module
add_executable(${PROJECT_NAME}.elf ${SRC_FILES})
target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${CFLAGS} -DMODULE)
target_link_libraries(${PROJECT_NAME}.elf
        ${CMAKE_BINARY_DIR}/psp2shell_k/psp2shell_k-stubs/libpsp2shell_k_stub.a
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
        -nostdlib)

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}.elf
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_m.yml
        UNSAFE)
