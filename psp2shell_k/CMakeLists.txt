cmake_minimum_required(VERSION 3.0)

project(psp2shell_k)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99 -D__PSP2__")

link_directories(${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DRELEASE)
add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME}
        taihenForKernel_stub
        taihenModuleUtils_stub
        SceSysmemForDriver_stub
        SceIofilemgrForDriver_stub
        SceSysclibForDriver_stub
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES LINK_FLAGS "-nostdlib"
        COMPILE_FLAGS "-D__VITA_KERNEL__"
        )

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k.yml)
vita_create_stubs(${PROJECT_NAME}-stubs ${PROJECT_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_k.yml KERNEL)
