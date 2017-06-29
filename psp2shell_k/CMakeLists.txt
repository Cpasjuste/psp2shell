cmake_minimum_required(VERSION 2.8)

project(psp2shell_k)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99 -D__PSP2__ -D__VITA_KERNEL__")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions -D__PSP2__ -D__VITA_KERNEL__")

link_directories(${CMAKE_CURRENT_BINARY_DIR})

# fix "make" command not finding VITASDK env variable
# on my system (CLion)..
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

########
# WIFI #
########
add_executable(${PROJECT_NAME}
        main.c
        hooks.c
        )
target_link_libraries(${PROJECT_NAME}
        taihenForKernel_stub
        SceLibKernel_stub
        SceSysmemForDriver_stub
        SceThreadmgrForDriver_stub
        SceIofilemgrForDriver_stub
        SceNetPsForDriver_stub
        SceModulemgrForKernel_stub
        k
        gcc
        -nostdlib
        )
vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k.yml)
_vita_create_stubs(${PROJECT_NAME}-stubs ${PROJECT_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k.yml KERNEL)

#######
# USB #
#######
add_executable(${PROJECT_NAME}_usb
        main.c
        hooks.c
        usbhostfs/usbhostfs.c
        usbhostfs/host_driver.c
        )
target_include_directories(${PROJECT_NAME}_usb PRIVATE
        usbhostfs
        ../common
        )
target_link_libraries(${PROJECT_NAME}_usb
        taihenForKernel_stub
        SceSysmemForDriver_stub
        SceThreadmgrForDriver_stub
        SceIofilemgrForDriver_stub
        SceModulemgrForDriver_stub
        SceModulemgrForKernel_stub
        SceUdcdForDriver_stub
        SceCpuForDriver_stub
        SceDebugForDriver_stub
        k
        gcc
        -nostdlib
        )
set_target_properties(${PROJECT_NAME}_usb
        PROPERTIES COMPILE_FLAGS "-D__USB__"
        )
vita_create_self(${PROJECT_NAME}_usb.skprx ${PROJECT_NAME}_usb
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k_usb.yml)
_vita_create_stubs(${PROJECT_NAME}_usb-stubs ${PROJECT_NAME}_usb
        ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k_usb.yml KERNEL)