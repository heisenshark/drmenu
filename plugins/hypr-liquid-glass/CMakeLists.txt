cmake_minimum_required(VERSION 3.25)
project(hypr-liquid-glass LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-fPIC -O3 -Wall -Wno-unused-variable -Wno-unused-parameter --no-gnu-unique -fvisibility=default")

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED hyprland pixman-1 cairo pangocairo libdrm)

add_library(hypr-liquid-glass SHARED
    main.cpp
)

target_include_directories(hypr-liquid-glass PRIVATE
    ${DEPS_INCLUDE_DIRS}
)

target_link_libraries(hypr-liquid-glass PRIVATE
    ${DEPS_LIBRARIES}
)

set_target_properties(hypr-liquid-glass PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hypr-liquid-glass"
)

install(TARGETS hypr-liquid-glass DESTINATION "lib")
