set(VITASDK $ENV{VITASDK})

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "armv7-a")
set(CMAKE_C_COMPILER "${VITASDK}/bin/arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "${VITASDK}/bin/arm-vita-eabi-g++")
set(CMAKE_AR "${VITASDK}/bin/arm-vita-eabi-ar" CACHE STRING "")
set(CMAKE_RANLIB "${VITASDK}/bin/arm-vita-eabi-ranlib" CACHE STRING "")

set(ARCH "-march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard")
set(CMAKE_C_FLAGS "-Wl,-q -fno-builtin -fno-common -ffast-math -fomit-frame-pointer -fexpensive-optimizations ${ARCH}" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "C++ flags")

SET(CMAKE_FIND_ROOT_PATH "${VITASDK} ${VITASDK}/arm-vita-eabi")
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(PSP2 TRUE)
