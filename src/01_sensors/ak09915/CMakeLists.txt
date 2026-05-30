add_library(nav_ak09915 OBJECT ak09915.cpp)
target_include_directories(nav_ak09915 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_ak09915 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(nav_ak09915 PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/i2c)
