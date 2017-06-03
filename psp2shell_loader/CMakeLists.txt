cmake_minimum_required(VERSION 3.0)

project(psp2shell_loader)
set(TITLE_ID "PSP2SLOAD")

set(INC_FILES
        ../common/include
        include)

set(CFLAGS -O3 -DVERSION=\"${BUILD_DATE}\")

add_executable(${PROJECT_NAME}.elf source/main.c)
target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${CFLAGS})

target_link_libraries(${PROJECT_NAME}.elf
        taihen_stub
        SceLibKernel_stub
        SceSysmodule_stub
        SceDisplay_stub
        SceCtrl_stub
        )

add_custom_target(${PROJECT_NAME}.vpk ALL
        DEPENDS ${PROJECT_NAME}.elf
        COMMAND ${VITASDK}/bin/vita-elf-create ${PROJECT_NAME}.elf ${PROJECT_NAME}.velf
        COMMAND ${VITASDK}/bin/vita-make-fself ${PROJECT_NAME}.velf eboot.bin
        COMMAND ${VITASDK}/bin/vita-mksfoex -s TITLE_ID=${TITLE_ID} "${PROJECT_NAME}" param.sfo
        COMMAND ${VITASDK}/bin/vita-pack-vpk -s param.sfo -b eboot.bin ${PROJECT_NAME}.vpk
        )
