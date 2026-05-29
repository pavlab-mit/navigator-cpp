cmake_minimum_required(VERSION 3.14)
project(navigator_cpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

option(BUILD_TESTS "Build hardware test binaries" OFF)

# ─── Static library ─────────────────────────────────────────────

add_library(navigator_cpp STATIC
    src/nav.cpp
    src/i2c.cpp
    src/spi.cpp
    src/gpio.cpp
    src/bmp390.cpp
    src/bmp280.cpp
    src/icm20689.cpp
    src/ak09915.cpp
    src/mmc5983.cpp
    src/ads1115.cpp
    src/pca9685.cpp
    src/leak.cpp
    src/led.cpp
    src/neopixel.cpp
    vendored/BMP3_SensorAPI/bmp3.c
)

target_include_directories(navigator_cpp
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendored/BMP3_SensorAPI
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(navigator_cpp PRIVATE gpiod)
endif()

# ─── Tests ───────────────────────────────────────────────────────

if(BUILD_TESTS)
    set(TEST_SOURCES
        test_version_detect
        test_bmp390
        test_bmp280
        test_icm20689
        test_ak09915
        test_mmc5983
        test_ads1115
        test_pca9685
        test_leak
        test_leds
        test_neopixel
        test_all
    )

    foreach(test_name ${TEST_SOURCES})
        add_executable(${test_name} tests/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE navigator_cpp)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_link_libraries(${test_name} PRIVATE gpiod)
        endif()
    endforeach()
endif()
