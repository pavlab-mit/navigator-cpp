add_library(nav_icm20689 OBJECT icm20689.cpp)
target_include_directories(nav_icm20689 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_icm20689 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(nav_icm20689 PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/spi)
