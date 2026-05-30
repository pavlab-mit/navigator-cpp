add_library(nav_i2c OBJECT i2c.cpp)
target_include_directories(nav_i2c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_i2c PRIVATE ${CMAKE_SOURCE_DIR}/include)
