cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
endif()

project(psp2shell)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(TITLE_ID "PSP2SHELL")

set(INC_FILES
        ${VITASDK}/arm-vita-eabi/include
        ../common/include
        include)

set(CFLAGS -O3 -DVERSION=\"${BUILD_DATE}\")

add_executable(${PROJECT_NAME}.elf source/main.c)
target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${CFLAGS})

target_link_libraries(${PROJECT_NAME}.elf
        psp2shell
        taihen_stub
        SceDisplay_stub
        SceCtrl_stub
        SceSysmodule_stub
        SceNet_stub
        SceNetCtl_stub
        SceAppMgr_stub
        ScePower_stub
        )

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME}.elf UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${TITLE_ID} ${PROJECT_NAME}.self
        VERSION ${VITA_VERSION}
        NAME ${PROJECT_NAME}
        )