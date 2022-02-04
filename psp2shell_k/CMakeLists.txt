cmake_minimum_required(VERSION 3.0)

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
include("$ENV{VITASDK}/share/vita.toolchain.cmake" REQUIRED)

project(psp2shell_k)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99 -D__PSP2__ -D__KERNEL__")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions -D__PSP2__ -D__KERNEL__")

add_executable(${PROJECT_NAME}
        source/main.c
        source/file.c
        source/utility.c
        source/cmd.c
        source/module.c
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        include
        ${CMAKE_SOURCE_DIR}/common
        )

target_link_libraries(${PROJECT_NAME}
        taihenForKernel_stub
        SceLibKernel_stub
        SceSysmemForDriver_stub
        SceThreadmgrForDriver_stub
        SceIofilemgrForDriver_stub
        SceNetPsForDriver_stub
        SceModulemgrForKernel_stub
        SceDebugForDriver_stub
        k
        gcc
        -nostdlib
        )

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k.yml)
vita_create_stubs(${PROJECT_NAME}-stubs ${PROJECT_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k.yml KERNEL)
