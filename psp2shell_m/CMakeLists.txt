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
        source/hooks.c
        source/pool.c
        ../common/source/psp2cmd.c
        ../common/source/libmodule.c
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

# library
add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS})

# user module
add_executable(${PROJECT_NAME}.elf ${SRC_FILES})
target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INC_FILES})
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${CFLAGS} -DMODULE)
target_link_libraries(${PROJECT_NAME}.elf
        #${CMAKE_BINARY_DIR}/psp2shell_k/psp2shell_k-stubs/libpsp2shell_k_stub.a
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

# fix "make" command not finding VITASDK env variable
# on my system..
macro(_vita_create_stubs target-dir source config)
    set(VITA_ELF_EXPORT_FLAGS "${VITA_ELF_EXPORT_FLAGS}" CACHE STRING "vita-elf-export flags")
    set(VITA_LIBS_GEN_FLAGS "${VITA_LIBS_GEN_FLAGS}" CACHE STRING "vita-libs-gen flags")

    set(options KERNEL)
    cmake_parse_arguments(_vita_create_stubs "${options}" "" "" ${ARGN})

    if (_vita_create_stubs_KERNEL)
        set(kind kernel)
    else ()
        set(kind user)
    endif ()
    separate_arguments(VITA_ELF_EXPORT_FLAGS)
    get_filename_component(fconfig ${config} ABSOLUTE)
    MESSAGE(info OUTPUT ${target-dir}.yml
            COMMAND ${VITA_ELF_EXPORT} ${kind} ${VITA_ELF_EXPORT_FLAGS} ${source} ${fconfig} ${target-dir}.yml
            DEPENDS ${source} ${fconfig}
            COMMENT "Generating imports YAML for ${source}")
    add_custom_command(OUTPUT ${target-dir}.yml
            COMMAND ${VITA_ELF_EXPORT} ${kind} ${VITA_ELF_EXPORT_FLAGS} ${source} ${fconfig} ${target-dir}.yml
            DEPENDS ${source} ${fconfig}
            COMMENT "Generating imports YAML for ${source}"
            )
    separate_arguments(VITA_LIBS_GEN_FLAGS)
    add_custom_target(${target-dir} ALL
            COMMAND ${VITA_LIBS_GEN} ${VITA_LIBS_GEN_FLAGS} ${target-dir}.yml ${target-dir}
            # modified
            COMMAND VITASDK=${VITASDK} make -C ${target-dir}
            # modified
            DEPENDS ${target-dir}.yml
            COMMENT "Building stubs ${target-dir}"
            )
    add_dependencies(${target-dir} ${source})
endmacro(_vita_create_stubs)
_vita_create_stubs(psp2shell_m-stubs ${PROJECT_NAME}.elf
        ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_m.yml)

#add_dependencies(${PROJECT_NAME}.elf psp2shell_m-stubs)
#add_dependencies(${PROJECT_NAME}.suprx psp2shell_m-stubs)

# library
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES include/psp2shell.h DESTINATION include)
