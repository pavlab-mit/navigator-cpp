add_library(navigator_cpp SHARED nav.cpp)

target_include_directories(navigator_cpp
    PUBLIC  ${CMAKE_SOURCE_DIR}/include

    # Sensors
    PRIVATE ${CMAKE_SOURCE_DIR}/src/01_sensors/ads1115
    PRIVATE ${CMAKE_SOURCE_DIR}/src/01_sensors/ak09915
    PRIVATE ${CMAKE_SOURCE_DIR}/src/01_sensors/bmp280
    PRIVATE ${CMAKE_SOURCE_DIR}/src/01_sensors/bmp390
    PRIVATE ${CMAKE_SOURCE_DIR}/src/01_sensors/icm20689
    PRIVATE ${CMAKE_SOURCE_DIR}/src/01_sensors/mmc5983

    # Interfaces
    PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/gpio
    PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/i2c
    PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/leak
    PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/led
    PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/neopixel
    PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/pca9685
    PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/sbus
    PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/spi

    # Vendored
    PRIVATE ${CMAKE_SOURCE_DIR}/src/03_vendored/BMP3_SensorAPI
    PRIVATE ${CMAKE_SOURCE_DIR}/src/03_vendored/attitude_estimator
)

target_link_libraries(navigator_cpp PRIVATE
    # Sensors
    nav_ads1115
    nav_ak09915
    nav_bmp280
    nav_bmp390
    nav_icm20689
    nav_mmc5983

    # Interfaces
    nav_gpio
    nav_i2c
    nav_leak
    nav_led
    nav_neopixel
    nav_pca9685
    nav_sbus
    nav_spi

    # Vendored
    nav_attitude_estimator
    nav_bmp3_api
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(navigator_cpp PRIVATE gpiod)
endif()
