#version 450

// Uniform buffer for matrices
layout (binding = 0) uniform UBO {
    mat4 projectionMatrix;
    mat4 modelMatrix;
    mat4 viewMatrix;
} ubo;

// Vertex attributes
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

// Output to fragment shader
layout (location = 0) out vec3 outColor;

void main() {
    outColor = inColor;
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos, 1.0);
}
