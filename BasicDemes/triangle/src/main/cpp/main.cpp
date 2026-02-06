/*
 * Android Native Activity Entry Point
 * Using GameActivity with VulkanExampleBase architecture
 */

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include "Triangle.hpp"

// Global instance
Triangle* vulkanExample = nullptr;

/**
 * @brief Android main entry point for native activity
 */
void android_main(android_app* state) {
    LOGI("android_main: Starting Vulkan Example");

    vulkanExample = new Triangle();
    
    // Set up android app reference and callbacks
    state->userData = vulkanExample;
    state->onAppCmd = Triangle::handleAppCommand;
    
    // Note: GameActivity uses a different input handling mechanism
    // Input events are processed through android_app_swap_input_buffers()
    // instead of the traditional onInputEvent callback

    // Enter the main render loop
    vulkanExample->renderLoop();

    delete vulkanExample;
    vulkanExample = nullptr;

    LOGI("android_main: Exiting");
}
