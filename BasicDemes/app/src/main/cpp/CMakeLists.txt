# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("androidvulkantutorials")

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u \
    Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(androidvulkantutorials SHARED
            VulkanMain.cpp
            AndroidMain.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


target_link_libraries(${PROJECT_NAME} PUBLIC
        vulkan
        game-activity::game-activity_static
        android
        log)