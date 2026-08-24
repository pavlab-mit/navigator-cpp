add_library(nav_spi OBJECT spi.cpp)
target_include_directories(nav_spi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_spi PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(nav_spi PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/gpio)
