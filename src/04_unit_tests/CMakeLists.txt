set(TESTS
    test_ads1115
    test_ahrs
    test_ak09915
    test_all
    test_bmp280
    test_bmp390
    test_icm20689
    test_leak
    test_leds
    test_mmc5983
    test_neopixel
    test_pca9685
    test_version_detect
)

foreach(test_name ${TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE navigator_cpp)
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/sbus)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(${test_name} PRIVATE gpiod)
    endif()
endforeach()
