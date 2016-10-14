cmake_minimum_required(VERSION 3.0)
#set(CMAKE_VERBOSE_MAKEFILE ON)

# set vitasdk path variable
set(VITASDK "/usr/local/vitasdk")

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

add_subdirectory(psp2shell)
add_subdirectory(psp2shell_test)
add_subdirectory(psp2shell_loader) # no module support yet in vitasdk
#add_subdirectory(psp2shell_cli) # external project for now, else cross-compile problem
