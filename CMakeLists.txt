cmake_minimum_required(VERSION 3.18.4)

# dvmtranscode
file(GLOB dvmtranscode_SRC
    # DMR module
    "dmr/*.h"
    "dmr/*.cpp"
    "dmr/data/*.h"
    "dmr/data/*.cpp"
    "dmr/lc/*.h"
    "dmr/lc/*.cpp"

    # P25 module
    "p25/*.h"
    "p25/*.cpp"
    "p25/data/*.h"
    "p25/data/*.cpp"
    "p25/lc/*.h"
    "p25/lc/*.cpp"

    # Core
    "edac/*.h"
    "edac/*.cpp"
    "host/*.h"
    "host/*.cpp"
    "network/*.h"
    "network/*.cpp"
    "network/json/*.h"
    "vocoder/*.cpp"
    "vocoder/*.c"
    "vocoder/*.h"
    "vocoder/imbe/*.cpp"
    "vocoder/imbe/*.h"
    "yaml/*.h"
    "yaml/*.cpp"
    "*.h"
    "*.cpp"
)

# cross-compile options
option(CROSS_COMPILE_ARM "Cross-compile for 32-bit ARM" off)
option(CROSS_COMPILE_AARCH64 "Cross-compile for 64-bit ARM" off)
option(CROSS_COMPILE_RPI_ARM "Cross-compile for (old RPi) 32-bit ARM" off)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

message(CHECK_START "Cross compiling for 32-bit ARM")
if (CROSS_COMPILE_ARM)
    set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
    message(CHECK_PASS "yes")
else ()
    message(CHECK_PASS "no")
endif (CROSS_COMPILE_ARM)
message(CHECK_START "Cross compiling for 64-bit ARM")
if (CROSS_COMPILE_AARCH64)
    set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
    message(CHECK_PASS "yes")
else ()
    message(CHECK_PASS "no")
endif (CROSS_COMPILE_AARCH64)
message(CHECK_START "Cross compiling for (old RPi) 32-bit ARM")
if (CROSS_COMPILE_RPI_ARM)
    set(CMAKE_C_COMPILER /opt/tools/arm-bcm2708/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER /opt/tools/arm-bcm2708/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++)
    message(CHECK_PASS "yes")
else ()
    message(CHECK_PASS "no")
endif (CROSS_COMPILE_RPI_ARM)

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY .)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall -std=c++11")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g -O3 -Wall -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O3 -Wall -std=c++11 -s")

set(GIT_VER "")
set(GIT_VER_HASH "")
execute_process(COMMAND git describe --abbrev=8 --dirty --always --tags WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE GIT_VER OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git describe --abbrev=8 --always --tags WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE GIT_VER_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)

add_definitions(-D__GIT_VER__="${GIT_VER}")
add_definitions(-D__GIT_VER_HASH__="${GIT_VER_HASH}")

project(dvmtranscode)
find_package(Threads REQUIRED)
add_executable(dvmtranscode ${dvmtranscode_SRC})
target_include_directories(dvmtranscode PRIVATE .)
target_link_libraries(dvmtranscode PRIVATE Threads::Threads)
