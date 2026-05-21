cmake_minimum_required(VERSION 3.20)
project(DaVinciCode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dvcore
    src/core/Card.cpp
    src/core/Deck.cpp
    src/core/Hand.cpp
    src/core/GameManager.cpp
)
target_include_directories(dvcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(dv_console src/main_console.cpp)
target_link_libraries(dv_console PRIVATE dvcore)

if(MSVC)
    target_compile_options(dvcore PRIVATE /W4 /utf-8)
    target_compile_options(dv_console PRIVATE /W4 /utf-8)
endif()
