set(EXAMPLES
    nav_ahrs_example
    nav_configure_example
    nav_error_handling_example
    nav_led_example
    nav_pwm_example
    nav_usage_example
)

foreach(example_name ${EXAMPLES})
    add_executable(${example_name} ${example_name}.cpp)
    target_link_libraries(${example_name} PRIVATE navigator_cpp)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(${example_name} PRIVATE gpiod)
    endif()
endforeach()
