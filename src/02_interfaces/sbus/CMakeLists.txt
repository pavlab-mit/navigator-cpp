add_library(nav_sbus OBJECT sbus_handler.cpp)
target_include_directories(nav_sbus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
