cmake_minimum_required(VERSION 3.0)

project(psp2shell)
set(TITLE_ID "PSP2SHELL")

set(INC_FILES
        ${VITASDK}/arm-vita-eabi/include
        ../common/include
        include)

set(CFLAGS ${CMAKE_C_FLAGS} -O3 -DVERSION=\"${BUILD_DATE}\")

add_executable(${PROJECT_NAME}.elf source/main.c)
target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${CFLAGS})

target_link_libraries(${PROJECT_NAME}.elf
        psp2shell
        SceDisplay_stub
        SceCtrl_stub
        SceSysmodule_stub
        SceNet_stub
        SceNetCtl_stub
        SceAppMgr_stub)

add_custom_target(${PROJECT_NAME}.vpk
        DEPENDS ${PROJECT_NAME}.elf
        COMMAND ${VITASDK}/bin/vita-elf-create ${PROJECT_NAME}.elf ${PROJECT_NAME}.velf
        COMMAND ${VITASDK}/bin/vita-make-fself ${PROJECT_NAME}.velf eboot.bin
        COMMAND ${VITASDK}/bin/vita-mksfoex -s TITLE_ID=${TITLE_ID} "${PROJECT_NAME}" param.sfo
        COMMAND ${VITASDK}/bin/vita-pack-vpk -s param.sfo -b eboot.bin ${PROJECT_NAME}.vpk)
