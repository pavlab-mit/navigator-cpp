add_library(nav_neopixel OBJECT neopixel.cpp)
target_include_directories(nav_neopixel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_neopixel PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(nav_neopixel PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/spi)
