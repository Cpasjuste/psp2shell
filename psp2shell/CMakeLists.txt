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

set(SRC_FILES
        source/main.c
        source/utility.c
        source/file.c
        source/module.c
        source/thread.c
        source/hooks.c
        ../common/source/libmodule.c
        ../common/source/binn.c)

set(INC_FILES
        ${VITASDK}/arm-vita-eabi/include
        ../common/include
        include)

set(CFLAGS -Wall -O2 -std=c99
        -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format
        -Wno-strict-aliasing -Wno-attributes -ffreestanding
        -DVERSION=\"${BUILD_DATE}\")
#-DDEBUG=1

# library
add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS})

# user module
add_executable(${PROJECT_NAME}_user.elf ${SRC_FILES})
target_include_directories(${PROJECT_NAME}_user.elf PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME}_user.elf PRIVATE ${CFLAGS} -DMODULE)
target_link_libraries(${PROJECT_NAME}_user.elf
        k
        taihen_stub
        SceLibKernel_stub
        SceKernelThreadMgr_stub
        SceKernelModulemgr_stub
        SceSysmodule_stub
        SceSysmem_stub
        SceIofilemgr_stub
        SceNet_stub
        SceNetCtl_stub
        SceAppMgr_stub
        gcc
        -nostdlib)
vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}_user.elf UNSAFE)

# kernel module
add_executable(${PROJECT_NAME}_kernel.elf ${SRC_FILES})
target_include_directories(${PROJECT_NAME}_kernel.elf PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME}_kernel.elf PRIVATE ${CFLAGS} -DMODULE -D__VITA_KERNEL__ -DDEBUG)
target_link_libraries(${PROJECT_NAME}_kernel.elf
        k
        taihenForKernel_stub
        SceSysclibForDriver_stub
        SceSysmemForDriver_stub
        SceThreadmgrForDriver_stub
        SceIofilemgrForDriver_stub
        SceNetPsForDriver_stub
        gcc
        -nostdlib)
vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}_kernel.elf UNSAFE)

# library
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES include/psp2shell.h DESTINATION include)
