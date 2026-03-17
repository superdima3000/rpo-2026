
cmake_minimum_required(VERSION 3.22.1)

project("andro25")

add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp)

find_library(log-lib log)

#spdlog
add_library(spdlog STATIC IMPORTED)
set_target_properties(spdlog PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/../../../../../libs/spdlog/build/${ANDROID_ABI}/libspdlog.a)

#mbedcrypto
add_library(mbedcrypto SHARED IMPORTED)
set_target_properties(mbedcrypto PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/../nativeLibs/${ANDROID_ABI}/libmbedcrypto.so)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/../../../../../libs/spdlog/spdlog/include
    ${CMAKE_SOURCE_DIR}/../../../../../libs/mbedtls/mbedtls/include)



target_link_libraries(${CMAKE_PROJECT_NAME}
    android
    spdlog
    mbedcrypto
    ${log-lib})