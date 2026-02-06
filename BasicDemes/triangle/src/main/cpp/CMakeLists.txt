# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("triangle")

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u \
    Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(triangle SHARED
        VulkanBase.cpp
        Triangle.cpp
        main.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_link_libraries(${PROJECT_NAME} PUBLIC
        vulkan
        game-activity::game-activity_static
        android
        log)

# ============================================================================
# Shader compilation
# ============================================================================

# Find glslc shader compiler
# First try to find it in Android NDK
if(ANDROID)
    # Get the path to glslc from the Android NDK
    if(DEFINED ENV{ANDROID_NDK})
        set(GLSLC_PATH "$ENV{ANDROID_NDK}/shader-tools/${ANDROID_HOST_TAG}/glslc")
    elseif(DEFINED ANDROID_NDK)
        set(GLSLC_PATH "${ANDROID_NDK}/shader-tools/${ANDROID_HOST_TAG}/glslc")
    endif()
    
    if(NOT EXISTS ${GLSLC_PATH})
        # Try common paths
        find_program(GLSLC_PATH glslc
            HINTS
                "$ENV{VULKAN_SDK}/Bin"
                "$ENV{VULKAN_SDK}/bin"
        )
    endif()
endif()

if(NOT GLSLC_PATH)
    find_program(GLSLC_PATH glslc)
endif()

if(GLSLC_PATH)
    message(STATUS "Found glslc: ${GLSLC_PATH}")
    
    # Define shader source and output directories
    set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shaders")
    set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../assets/shaders")
    
    # Create output directory
    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
    
    # List of shaders to compile
    set(SHADER_SOURCES
        "${SHADER_SOURCE_DIR}/triangle.vert"
        "${SHADER_SOURCE_DIR}/triangle.frag"
    )
    
    # Compile each shader
    foreach(SHADER_SOURCE ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME)
        set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
        
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSLC_PATH} -o ${SHADER_OUTPUT} ${SHADER_SOURCE}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling shader ${SHADER_NAME}"
        )
        
        list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endforeach()
    
    # Add a custom target for shaders
    add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})
    
    # Make sure shaders are compiled before the main library
    add_dependencies(${PROJECT_NAME} shaders)
else()
    message(WARNING "glslc not found! Shaders will not be compiled automatically.")
    message(WARNING "Please compile shaders manually using:")
    message(WARNING "  glslc triangle.vert -o triangle.vert.spv")
    message(WARNING "  glslc triangle.frag -o triangle.frag.spv")
endif()
