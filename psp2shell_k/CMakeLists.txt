cmake_minimum_required(VERSION 3.0)

project(psp2shell_k)

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99 -D__PSP2__")

link_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(../common)
add_definitions(-DRELEASE -DVERSION=\"${BUILD_DATE}\")
add_executable(${PROJECT_NAME}
        main.c cmd.c file.c module.c net.c patch.c utility.c
        ../common/p2s_msg.c ../common/p2s_cmd.c)
target_link_libraries(${PROJECT_NAME}
        ScePowerForDriver_stub
        SceSysmemForDriver_stub
        SceThreadmgrForDriver_stub
        SceIofilemgrForDriver_stub
        SceNetPsForDriver_stub
        SceModulemgrForKernel_stub
        SceModulemgrForDriver_stub
        SceKernelUtilsForDriver_stub
        SceDebugForDriver_stub
        SceLibKernel_stub
        SceSysclibForDriver_stub
        taihenForKernel_stub
        taihenModuleUtils_stub
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES LINK_FLAGS "-nostdlib"
        COMPILE_FLAGS "-D__VITA_KERNEL__"
        )

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k.yml)
#vita_create_stubs(${PROJECT_NAME}-stubs ${PROJECT_NAME}
#        ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k.yml KERNEL)
