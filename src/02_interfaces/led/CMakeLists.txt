add_library(nav_led OBJECT led.cpp)
target_include_directories(nav_led PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_led PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(nav_led PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/gpio)
