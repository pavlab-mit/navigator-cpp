add_library(nav_mmc5983 OBJECT mmc5983.cpp)
target_include_directories(nav_mmc5983 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_mmc5983 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(nav_mmc5983 PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/spi)
