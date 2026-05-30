add_library(nav_bmp280 OBJECT bmp280.cpp)
target_include_directories(nav_bmp280 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_bmp280 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(nav_bmp280 PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/i2c)
