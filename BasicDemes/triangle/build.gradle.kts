plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.ahao.triangle"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.ahao.triangle"
        minSdk = 29
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        
        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64", "x86")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    buildFeatures {
        prefab = true
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

// Shader compilation task
tasks.register("compileShaders") {
    description = "Compile GLSL shaders to SPIR-V"
    group = "build"
    
    val shaderDir = file("src/main/shaders")
    val outputDir = file("src/main/assets/shaders")
    
    inputs.dir(shaderDir)
    outputs.dir(outputDir)
    
    doLast {
        // Create output directory
        outputDir.mkdirs()
        
        // Find glslc
        val glslc = findGlslc()
        if (glslc == null) {
            logger.warn("glslc not found! Please compile shaders manually.")
            logger.warn("You can install Vulkan SDK or use Android NDK's glslc.")
            return@doLast
        }
        
        logger.lifecycle("Using glslc: $glslc")
        
        // Compile each shader
        shaderDir.listFiles()?.filter { it.extension in listOf("vert", "frag", "comp", "geom", "tesc", "tese") }?.forEach { shader ->
            val outputFile = File(outputDir, "${shader.name}.spv")
            logger.lifecycle("Compiling ${shader.name} -> ${outputFile.name}")
            
            exec {
                commandLine(glslc, "-o", outputFile.absolutePath, shader.absolutePath)
            }
        }
    }
}

fun findGlslc(): String? {
    // Try Android NDK first
    val ndkDir = System.getenv("ANDROID_NDK_HOME") 
        ?: System.getenv("ANDROID_NDK")
        ?: android.ndkDirectory.absolutePath
    
    if (ndkDir.isNotEmpty()) {
        val hostTag = when {
            org.gradle.internal.os.OperatingSystem.current().isWindows -> "windows-x86_64"
            org.gradle.internal.os.OperatingSystem.current().isMacOsX -> "darwin-x86_64"
            else -> "linux-x86_64"
        }
        val ndkGlslc = File(ndkDir, "shader-tools/$hostTag/glslc${if (org.gradle.internal.os.OperatingSystem.current().isWindows) ".exe" else ""}")
        if (ndkGlslc.exists()) {
            return ndkGlslc.absolutePath
        }
    }
    
    // Try Vulkan SDK
    val vulkanSdk = System.getenv("VULKAN_SDK")
    if (!vulkanSdk.isNullOrEmpty()) {
        val sdkGlslc = File(vulkanSdk, "Bin/glslc${if (org.gradle.internal.os.OperatingSystem.current().isWindows) ".exe" else ""}")
        if (sdkGlslc.exists()) {
            return sdkGlslc.absolutePath
        }
    }
    
    // Try system PATH
    return try {
        val process = ProcessBuilder(if (org.gradle.internal.os.OperatingSystem.current().isWindows) "where" else "which", "glslc")
            .redirectErrorStream(true)
            .start()
        val result = process.inputStream.bufferedReader().readLine()
        process.waitFor()
        if (process.exitValue() == 0 && !result.isNullOrEmpty()) result else null
    } catch (e: Exception) {
        null
    }
}

// Make preBuild depend on compileShaders
tasks.named("preBuild") {
    dependsOn("compileShaders")
}

dependencies {
    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.games.activity)
    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)
}
