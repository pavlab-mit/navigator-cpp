add_library(nav_ads1115 OBJECT ads1115.cpp)
target_include_directories(nav_ads1115 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_ads1115 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(nav_ads1115 PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/i2c)
