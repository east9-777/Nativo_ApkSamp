project(opencv)

# Caminho correto para o libopencv_java4.so
# Estrutura esperada:
# app/src/main/cpp/          ← aqui fica o CMakeLists.txt principal
# app/src/main/jniLibs/      ← aqui ficam as pastas armeabi-v7a e arm64-v8a

set(OPENCV_SO_PATH "\( {CMAKE_SOURCE_DIR}/../jniLibs/ \){ANDROID_ABI}/libopencv_java4.so")

# Mensagem de debug (aparece no log do Gradle)
message(STATUS "Looking for OpenCV at: ${OPENCV_SO_PATH}")

if(NOT EXISTS "${OPENCV_SO_PATH}")
    message(FATAL_ERROR "OpenCV shared library not found: ${OPENCV_SO_PATH}")
endif()

# Cria o target da biblioteca compartilhada
add_library(opencv_java4 SHARED IMPORTED)
set_target_properties(opencv_java4 PROPERTIES
    IMPORTED_LOCATION "${OPENCV_SO_PATH}"
)

# Target INTERFACE usado pelo resto do projeto
add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT_NAME} INTERFACE
    opencv_java4
)