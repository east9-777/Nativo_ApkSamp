project(opencv)

# Usamos a biblioteca compartilhada (libopencv_java4.so)
# Ela deve estar em: app/src/main/jniLibs/${ANDROID_ABI}/libopencv_java4.so

add_library(${PROJECT_NAME} INTERFACE)

# Inclui os headers
target_include_directories(${PROJECT_NAME} INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Linka com a lib compartilhada
# O Android Gradle Plugin já coloca as libs de jniLibs no caminho de link
target_link_libraries(${PROJECT_NAME} INTERFACE
    opencv_java4
)