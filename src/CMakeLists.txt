add_library(NeoPixel Neopixel.cpp)
target_include_directories(NeoPixel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)
target_link_libraries(NeoPixel GPIO)
