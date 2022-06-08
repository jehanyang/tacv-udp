cmake_minimum_required(VERSION 3.1)
project(SensorProd)

include_directories(${PROJECT_SOURCE_DIR})
find_library (sensor_LIBRARY NAMES core device PATHS ${PROJECT_SOURCE_DIR})

add_executable(test ${PROJECT_SOURCE_DIR}/test.cpp)
target_link_libraries(test ${sensor_LIBRARY})

add_executable(testTwoSensors ${PROJECT_SOURCE_DIR}/testTwoSEnsors.cpp)
target_link_libraries(testTwoSensors ${sensor_LIBRARY})
