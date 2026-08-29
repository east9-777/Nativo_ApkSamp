project(opencv)

set(OPENCV_SO_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../jniLibs/${ANDROID_ABI}/libopencv_java4.so"
)

message(STATUS "Looking for OpenCV at: ${OPENCV_SO_PATH}")

if(NOT EXISTS "${OPENCV_SO_PATH}")
    message(FATAL_ERROR "OpenCV shared library not found: ${OPENCV_SO_PATH}")
endif()

add_library(opencv_java4 SHARED IMPORTED)

set_target_properties(opencv_java4 PROPERTIES
    IMPORTED_LOCATION "${OPENCV_SO_PATH}"
)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT_NAME} INTERFACE
    opencv_java4
)