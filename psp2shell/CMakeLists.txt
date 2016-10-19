cmake_minimum_required(VERSION 3.0)

project(psp2shell)

set(SRC_FILES
        source/main.c
        source/utility.c
        source/file.c
        source/module.c
        source/thread.c
        ../common/source/binn.c)

set(INC_FILES
        ${VITASDK}/arm-vita-eabi/include
        ../common/include
        include)

set(CFLAGS ${CMAKE_C_FLAGS} -O2 -DVERSION=\"${BUILD_DATE}\")
#-DDEBUG=1

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS})

add_executable(${PROJECT_NAME}_mod.elf ${SRC_FILES})
target_include_directories(${PROJECT_NAME}_mod.elf PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME}_mod.elf PRIVATE ${CFLAGS} -DMODULE)
target_link_libraries(${PROJECT_NAME}_mod.elf
        SceSysmodule_stub
        SceKernel_stub
        SceNet_stub
        SceNetCtl_stub
        SceAppMgr_stub
        SceLibc_stub
        -Wl,-q -nostartfiles)
#-nostdlib

add_custom_target(${PROJECT_NAME}.suprx
        DEPENDS ${PROJECT_NAME}_mod.elf
        COMMAND ${VITASDK}/bin/vita-elf-create ${PROJECT_NAME}_mod.elf ${PROJECT_NAME}_mod.velf
        COMMAND ${VITASDK}/bin/vita-make-fself ${PROJECT_NAME}_mod.velf ${PROJECT_NAME}.suprx)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES include/psp2shell.h DESTINATION include)
