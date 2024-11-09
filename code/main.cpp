#include <GL/glew.h>
#include <GL/glut.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <cmath>

constexpr int screenWidth = 800;
constexpr int screenHeight = 600;
int oldTimeSinceStart = 0;

struct InstanceData {
    glm::mat4 model;
    glm::vec3 color;
};

// Shader source code
const char* vertexShaderSource = R"(
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in mat4 instanceModel;
layout(location = 6) in vec3 instanceColor;

uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

void main() {
    mat4 model = instanceModel;
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = instanceColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";


const char* fragmentShaderSource = R"(
#version 450 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

out vec4 FragColor;

void main() {
    FragColor = vec4(Color, 1.0);
}
)";

void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, unsigned int latitudeBands = 30, unsigned int longitudeBands = 30) {
    const float radius = 1.0f;
    vertices.clear();
    indices.clear();

    // Generate vertices and normals
    for (unsigned int lat = 0; lat <= latitudeBands; ++lat) {
        float theta = lat * M_PI / latitudeBands;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (unsigned int lon = 0; lon <= longitudeBands; ++lon) {
            float phi = lon * 2.0f * M_PI / longitudeBands;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            vertices.push_back(radius * x);
            vertices.push_back(radius * y);
            vertices.push_back(radius * z);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // Generate indices
    for (unsigned int lat = 0; lat < latitudeBands; ++lat) {
        for (unsigned int lon = 0; lon < longitudeBands; ++lon) {
            unsigned int first = (lat * (longitudeBands + 1)) + lon;
            unsigned int second = first + longitudeBands + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Error compiling shader: " << infoLog << std::endl;
    }
    return shader;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Instanced Spheres", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    // Sphere data
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateSphere(vertices, indices, 4, 4);

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    const int numObj_x = 30;
    const int numObj_y = 30;
    const int numObj_z = 30;

    constexpr int instanceCount = numObj_x * numObj_y * numObj_z;
    std::vector<InstanceData> instanceData(instanceCount);


    constexpr float spread = 1.15f;

    constexpr float cameraDist = spread * numObj_x * 1.5f;
    const float camSpead2 = 0.5f;

    for (int i = 0; i < numObj_x; i++)
    {
        for (int j = 0; j < numObj_y; j++)
        {
            for (int k = 0; k < numObj_z; k++)
            {
                float x = (-numObj_x / 2.0f) * spread + spread * i;
                float y = spread * j + spread;
                float z = (-numObj_z / 2.0f) * spread + spread * k;

                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
                model = glm::scale(model, glm::vec3(0.33f));
                glm::vec3 color = glm::vec3(0.1f * (i+1), 0.1f * (j+1), 0.1f * (k+1));

                int index = i * (numObj_y * numObj_z) + j * numObj_z + k;
                instanceData[index] = {model, color};
            }
        }
    }

    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceCount * sizeof(InstanceData), instanceData.data(), GL_STATIC_DRAW);

    // Set up the instance data for the model and color
    for (int i = 0; i < 4; ++i)
    {
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)(sizeof(glm::vec4) * i));
        glEnableVertexAttribArray(2 + i);
        glVertexAttribDivisor(2 + i, 1); // Tell OpenGL to use instanced data
    }

    // Color is a 3-element vector, which is handled separately
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)(sizeof(glm::mat4)));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1); // Tell OpenGL to use instanced data for color

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)screenWidth / screenHeight, 0.1f, 1000.0f);
    
    glm::vec3 cameraPos = glm::vec3(camSpead2 * cameraDist, cameraDist, camSpead2 * cameraDist);
    glm::vec3 targetPos = glm::vec3(0.0f, numObj_y * spread * 0.5, 0.0f);

    glm::vec3 upDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::mat4 view = glm::lookAt(cameraPos, targetPos, upDirection);

    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float timeSinceStart = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        //int deltaTime = timeSinceStart - oldTimeSinceStart;
        //oldTimeSinceStart = timeSinceStart;

        constexpr float camera_speed = 0.1f;
        constexpr float camera_radius = cameraDist;

        cameraPos.x = sin(timeSinceStart * camera_speed) * -camera_radius;
        cameraPos.z = cos(timeSinceStart * camera_speed) * camera_radius; 

        view = glm::lookAt(cameraPos, targetPos, upDirection);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

        glBindVertexArray(VAO);
        glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instanceCount);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
