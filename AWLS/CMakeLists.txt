cmake_minimum_required(VERSION 3.20)
project(FJSP)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)

# 设置 include 目录
include_directories(${CMAKE_SOURCE_DIR}/include)
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -O3 -static-libgcc -static-libstdc++")


# 查找所有源文件和头文件
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/include/*.h ${CMAKE_SOURCE_DIR}/include/*.hpp)

# 设置宏参数
# add_compile_definitions(PRINT_INFO)
# add_compile_definitions(MAP_MACHINE_DURATION)
add_compile_definitions(TABU_WITH_POS)
# add_compile_definitions(EVALUATE_1)
# add_compile_definitions(GREEDY_INIT)

# 设置可执行程序输出目录
set(publish_bin_debug ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)
set(publish_bin_release ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${publish_bin_debug})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${publish_bin_release})


# 生成可执行文件
add_executable(${PROJECT_NAME} main.cpp ${SOURCES} ${HEADERS})

set(USE_OPENMP ON)

message(STATUS "USE_OPENMP = ${USE_OPENMP}")

if (USE_OPENMP)
    find_package(OpenMP)
    if (OpenMP_FOUND)
        message(STATUS "OpenMP found. Enabling OpenMP support.")
        target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
        target_compile_definitions(${PROJECT_NAME} PUBLIC USE_OPENMP)
    else ()
        message(WARNING "USE_OPENMP is ON but OpenMP was not found. Proceeding without OpenMP.")
    endif ()
else ()
    message(STATUS "OpenMP support not enabled.")
endif ()