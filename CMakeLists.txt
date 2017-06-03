cmake_minimum_required(VERSION 3.0)
#set(CMAKE_VERBOSE_MAKEFILE ON)

# set vitasdk path variable
set(VITASDK $ENV{VITASDK})

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else ()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif ()
endif ()

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

# kernel module
add_subdirectory(psp2shell_k)

# main module
add_subdirectory(psp2shell_m)

# user module
# add_subdirectory(psp2shell_u)

add_subdirectory(psp2shell_loader) # no module support yet in vitasdk
#add_subdirectory(psp2shell_cli) # external project for now, else cross-compile problem
