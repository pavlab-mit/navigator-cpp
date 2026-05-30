add_library(nav_gpio OBJECT gpio.cpp)
target_include_directories(nav_gpio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_gpio PRIVATE ${CMAKE_SOURCE_DIR}/include)
