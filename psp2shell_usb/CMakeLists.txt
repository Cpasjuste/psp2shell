cmake_minimum_required(VERSION 2.8)

project(psp2shell_usb)
include("${VITASDK}/share/vita.cmake" REQUIRED)

# set build date variable
execute_process(COMMAND "date" "+%Y.%m.%d" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99 -D__PSP2__ -D__KERNEL__ -D__USB__ -DVERSION=\"${BUILD_DATE}\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions -D__PSP2__ -D__KERNEL__ -D__USB__ -DVERSION=\"${BUILD_DATE}\"")

link_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME}
        main.c
        usbhostfs/usbhostfs.c
        usbhostfs/host_driver.c
        ../psp2shell_m/source/cmd.c
        ../psp2shell_m/source/file.c
        ../psp2shell_m/source/main.c
        ../psp2shell_m/source/module.c
        ../psp2shell_m/source/thread.c
        ../psp2shell_m/source/utility.c
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        ../psp2shell_m/include
        ../common
        )

target_link_libraries(${PROJECT_NAME}
        taihenForKernel_stub
        SceSysmemForDriver_stub
        SceThreadmgrForDriver_stub
        SceIofilemgrForDriver_stub
        SceModulemgrForKernel_stub
        SceUdcdForDriver_stub
        SceCpuForDriver_stub
        SceDebugForDriver_stub
        k
        gcc
        -nostdlib
        )

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
        CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/psp2shell_usb.yml)
