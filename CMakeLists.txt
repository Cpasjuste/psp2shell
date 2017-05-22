cmake_minimum_required(VERSION 3.0)
#set(CMAKE_VERBOSE_MAKEFILE ON)

# set vitasdk path variable
set(VITASDK $ENV{VITASDK})

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
endif()

project(psp2shell)
include("${VITASDK}/share/vita.cmake" REQUIRED)

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

set(SRC_FILES
        psp2shell/source/main.c
        psp2shell/source/utility.c
        psp2shell/source/file.c
        psp2shell/source/module.c
        psp2shell/source/thread.c
        psp2shell/source/hooks.c
        psp2shell/source/pool.c
        common/source/psp2cmd.c
        common/source/libmodule.c
        )

set(INC_FILES
        common/include
        psp2shell/include
        )

set(CFLAGS -Wall -O2 -std=c99 -DPSP2
        -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format
        -Wno-strict-aliasing -Wno-attributes -ffreestanding
        -DVERSION=\"${BUILD_DATE}\"
        )
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
        SceNet_stub_weak
        SceNetCtl_stub_weak
        SceAppMgr_stub
        ScePower_stub
        gcc
        -nostdlib)
vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}_user.elf
        CONFIG ${CMAKE_SOURCE_DIR}/psp2shell/psp2shell.yml
        UNSAFE)

# kernel module
#add_executable(${PROJECT_NAME}_kernel.elf ${SRC_FILES})
#target_include_directories(${PROJECT_NAME}_kernel.elf PRIVATE ${INC_FILES})
#target_compile_options(${PROJECT_NAME}_kernel.elf PRIVATE ${CFLAGS} -DMODULE -D__VITA_KERNEL__)
#target_link_libraries(${PROJECT_NAME}_kernel.elf
#        k
#        taihenForKernel_stub
#        SceSysclibForDriver_stub
#        SceSysmemForDriver_stub
#        SceThreadmgrForDriver_stub
#        SceIofilemgrForDriver_stub
#        SceNetPsForDriver_stub
#        gcc
#        -nostdlib)
#vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}_kernel.elf UNSAFE)

# library
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES psp2shell/include/psp2shell.h DESTINATION include)


#add_subdirectory(psp2shell)
add_subdirectory(psp2shell_test)
add_subdirectory(psp2shell_loader) # no module support yet in vitasdk
#add_subdirectory(psp2shell_cli) # external project for now, else cross-compile problem
