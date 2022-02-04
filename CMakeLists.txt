cmake_minimum_required(VERSION 3.0)
#set(CMAKE_VERBOSE_MAKEFILE ON)

# set vitasdk path variable
if (NOT DEFINED ENV{VITASDK})
    set(ENV{VITASDK} "/usr/local/vitasdk")
endif ()
set(VITASDK $ENV{VITASDK})
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
include("$ENV{VITASDK}/share/vita.toolchain.cmake" REQUIRED)
message("${CMAKE_TOOLCHAIN_FILE}")

project(psp2shell)

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

# kernel module
add_subdirectory(psp2shell_k)

# main module
#add_subdirectory(psp2shell_m)
#add_dependencies(psp2shell_m psp2shell_k-stubs)

#add_subdirectory(psp2shell_loader)
#add_subdirectory(psp2shell_cli) # external project for now, else cross-compile problem

add_custom_target(release
        DEPENDS psp2shell_k psp2shell_m
        COMMAND mkdir -p ${CMAKE_BINARY_DIR}/release
        COMMAND cp ${CMAKE_BINARY_DIR}/psp2shell_k/psp2shell_k.skprx ${CMAKE_BINARY_DIR}/release/
        COMMAND cp ${CMAKE_BINARY_DIR}/psp2shell_m/psp2shell_m.suprx ${CMAKE_BINARY_DIR}/release/
        COMMAND cp ${CMAKE_SOURCE_DIR}/README.MD ${CMAKE_BINARY_DIR}/release/
        COMMAND cd ${CMAKE_BINARY_DIR}/release && zip -r ../psp2shell-${BUILD_DATE}.zip . && cd ..
        )