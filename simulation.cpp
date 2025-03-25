// g++ -o mon_projet.exe simulation.cpp glad.c -Iinclude -Llib -lglfw3 -lopengl32 -lgdi32
// ./simulation.cpp
// g++ -o mon_projet.exe simulation.cpp glad.c -Iinclude -IC:/msys64/mingw64/include/freetype2 -Llib -LC:/msys64/mingw64/lib -lglfw3 -lfreetype -lopengl32 -lgdi32
#include "glad/include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <xmmintrin.h>
#include <cmath>
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <sstream>

#include <map>
#include <string>

std::map<int, std::string> fastSqrtModes = {
    {0, "rien"},
    {1, "sqrt"},
    {2, "bit shift"},
    {3, "bit shift and correct"},
    {4, "Herron"},
    {5, "simple : 0.01"},
    {6, "simple : 0.1"}



};

// Shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec4 FragPosLightSpace; 

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix; 

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    FragPosLightSpace = lightSpaceMatrix * worldPos; 
    gl_Position = projection * view * worldPos;
}
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 FragPos;
    in vec3 Normal;
    in vec4 FragPosLightSpace;
    
    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform int fastSqrt;
    uniform vec3 objectColor;
    uniform sampler2D shadowMap;
    
    float simpleSqrt(float nbr,float epsilon) {
        float racine = 0;
        int iteration = 0;
        int i, j;
        for (j = 0; j < epsilon; j++) {
            i = 0;
            while ((racine + i * epsilon) * (racine + i * epsilon) < nbr) {
                iteration += 1;
                i += 1;
            }
            i -= 1;
            racine += i * epsilon;
        }
        return 1/racine;
    }

    float fastInvSqrtAndCorrect(float x) {
        int i = floatBitsToInt(x);
        i = 0x5F3759DF - (i >> 1);
        float y = intBitsToFloat(i);
        return y * (1.5 - 0.5 * x * y * y);
    }
    
    float fastInvSqrt(float x) {
        int i = floatBitsToInt(x);
        i = 0x5F3759DF - (i >> 1);
        float y = intBitsToFloat(i);
        return y;
    }
    
    float sqrtHerron(float number) {
        float x = number;
        
        for (int i = 0; i < 10; i++) {
            x = (x + number / x) / 2;
        }

        return 1 / x;
    }

    vec3 normaliser(vec3 v) {
        float lengthSq = dot(v, v);
        float invLength;
        if (fastSqrt == 0) {
            invLength = 1;
        }
        else if (fastSqrt == 1) {
            invLength = 1/sqrt(lengthSq);
        }
        else if (fastSqrt == 2) {
            invLength = fastInvSqrt(lengthSq);
        } 
        else if (fastSqrt == 3) {
            invLength = fastInvSqrtAndCorrect(lengthSq);
        } 
        else if (fastSqrt == 4) {
            invLength = sqrtHerron(lengthSq);
        }
        else if (fastSqrt == 5) {
            invLength = simpleSqrt(lengthSq,0.01);
        } 
        else if (fastSqrt == 6) {
            invLength = simpleSqrt(lengthSq,0.1);
        }
        return v * invLength;
    }

    // Fonction de calcul d'ombre simple (sans PCF)
    float ShadowCalculation(vec4 fragPosLightSpace)
    {
        // Perspective divide
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        float closestDepth = texture(shadowMap, projCoords.xy).r;
        float currentDepth = projCoords.z;
        float shadow = currentDepth - 0.005 > closestDepth ? 0.5 : 0.0; 
        return shadow;
    }

    void main()
    {
        vec3 norm = normaliser(Normal);
        vec3 lightDir = lightPos - FragPos;
        
        lightDir = normaliser(lightDir);

        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * (distance * distance));

        vec3 result = vec3(0.0);
        vec3 currentLightDir = lightDir;
        vec3 currentFragPos = FragPos;
        vec3 currentNorm = norm;
        float reflectionAttenuation = 1.0; 

        for (int i = 0; i < 3; i++) {
            float diff = max(dot(currentNorm, currentLightDir), 0.0);
            vec3 diffuse = attenuation * diff * objectColor * reflectionAttenuation;
            result += diffuse;

            // Reflect the light direction
            currentLightDir = reflect(currentLightDir, currentNorm);
           
            currentFragPos += currentLightDir * 0.1; 
            vec3 newNorm = normaliser(currentFragPos - FragPos); 
            currentNorm = normaliser(mix(currentNorm, newNorm, 0.5)); 

            // Atténuer la réflexion pour le prochain rebond
            reflectionAttenuation *= 0.5; 
        }

        // Calcul de l'ombre
        float shadow = ShadowCalculation(FragPosLightSpace);
        result = result * (1.0 - shadow);

        FragColor = vec4(result, 1.0);
    }
)";

const char* depthVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;
out vec4 FragPosLightSpace;
void main(){
    vec4 fragPos = model * vec4(aPos, 1.0);
    FragPosLightSpace = lightSpaceMatrix * fragPos;
    gl_Position = FragPosLightSpace;
}
)";

const char* depthFragmentShaderSource = R"(
#version 330 core
void main(){
    // Pas de couleur, seule la profondeur est écrite
}
)";

// Ajoutez la déclaration suivante avant de l'utiliser :
GLuint createShader(GLenum type, const char* source);

// Fonction pour créer le programme shader de profondeur
GLuint createDepthShaderProgram() {
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, depthVertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, depthFragmentShaderSource);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Erreur de linkage du programme shader:\n"
                  << infoLog << "\n";
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

GLuint createShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Erreur de compilation du shader:\n"
                  << infoLog << "\n";
    }

    return shader;
}

// Fonction de callback pour ajuster la taille de la fenêtre
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Initialisation OpenGL et création de la fenêtre
GLFWwindow* initOpenGL() {
    if (!glfwInit()) {
        std::cerr << "Échec de l'initialisation de GLFW\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Reflexions avec racine carrée inverse", nullptr, nullptr);
    if (!window) {
        std::cerr << "Échec de la création de la fenêtre\n";
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Échec de l'initialisation de GLAD\n";
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    return window;
}



const char* sphereFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    void main()
    {
        FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Couleur jaune
    }
    )";
    
GLuint createSphereShaderProgram() {
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
    )";

    GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, sphereFragmentShaderSource);
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Erreur de linkage du programme shader:\n"
                    << infoLog << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint createShaderProgram() {
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Erreur de linkage du programme shader:\n"
                  << infoLog << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}


glm::vec3 cameraPos = glm::vec3(2.0f, 0.0f, -2.0f);

glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 0.0f);

float PlaceSize = 4.0f;
float wallWidth = 0.1;
bool mouvLight = false;

glm::vec3 centerSphere1 = glm::vec3(-2.0f, 0.0f, 0.0f);
glm::vec3 centerSphere2 = glm::vec3(0.0f, 0.0f, 2.0f);
glm::vec3 centerSquare1 = glm::vec3(-2.0f, 1.0f, 2.0f);

glm::vec3 sizeSphere1 = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 sizeSphere2 = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 sizeSquare1 = glm::vec3(1.0f, 1.0f, 1.0f);





int fastSqrt = 0;


glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.005f; 

float lastX = 400, lastY = 300;
float yaw = -90.0f, pitch = 0.0f;
bool firstMouse = true;
float sensitivity = 0.1f;

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        if(mouvLight){
            lightPos += cameraSpeed * glm::vec3(1.0f, 0.0f, 0.0f);
        }else{
            cameraPos += cameraSpeed * cameraFront;
        }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        if(mouvLight){
            lightPos += cameraSpeed * glm::vec3(-1.0f, 0.0f, 0.0f);
        }else{
            cameraPos -= cameraSpeed * cameraFront;
        }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        if(mouvLight){
            lightPos += cameraSpeed * glm::vec3(0.0f, 0.0f, 1.0f);
        }else{
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        }
        
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        if(mouvLight){
            lightPos += cameraSpeed * glm::vec3(0.0f, 0.0f, -1.0f);
        }else{
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        if(mouvLight){
            lightPos += cameraSpeed  *  glm::vec3(0.0f, 1.0f, 0.0f);
        }else{
            cameraPos += cameraSpeed * cameraUp;
        }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        if(mouvLight){
            lightPos += cameraSpeed *  glm::vec3(0.0f, -1.0f, 0.0f);
        }else{
            cameraPos -= cameraSpeed * cameraUp;
        }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        fastSqrt = 0;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        fastSqrt = 1;
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
        fastSqrt = 2;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        fastSqrt = 3;
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        fastSqrt = 4;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        fastSqrt = 5;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        fastSqrt = 6;
    if (glfwGetKey(window, GLFW_KEY_Q)== GLFW_PRESS)
        mouvLight = true;
    if (glfwGetKey(window, GLFW_KEY_E)== GLFW_PRESS)
        mouvLight = false;

}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        cameraSpeed += yoffset * 0.0001f;
        if (cameraSpeed < 0.0001f) cameraSpeed = 0.0001f; 
    } else {
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void displayFPS(GLFWwindow* window) {
    static double previousSeconds = 0.0;
    static int frameCount = 0;
    double currentSeconds = glfwGetTime();
    double elapsedSeconds = currentSeconds - previousSeconds;

    if (elapsedSeconds > 0.25) {
        previousSeconds = currentSeconds;
        double fps = (double)frameCount / elapsedSeconds;
        std::ostringstream ss;
        ss.precision(3);    
        ss << std::fixed << fps << " FPS"<<" :: mode : "<<fastSqrtModes[fastSqrt];
        glfwSetWindowTitle(window, ss.str().c_str());
        frameCount = 0;
    }
    frameCount++;
}

int main() {
    GLFWwindow* window = initOpenGL();

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int screenWidth = mode->width;
    int screenHeight = mode->height;

    int windowWidth = 2880 - 100;
    int windowHeight = 1920 - 200;
    glfwSetWindowSize(window, windowWidth, windowHeight);

    int windowPosX = (screenWidth - windowWidth) / 2;
    int windowPosY = (screenHeight - windowHeight) / 2;
    glfwSetWindowPos(window, windowPosX, windowPosY);

    if (!window) return -1;

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLuint shaderProgram = createShaderProgram();

    GLuint sphereShaderProgram = createSphereShaderProgram();

    // Création de la shadow map
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint depthShaderProgram = createDepthShaderProgram();

    float smallCubeVertices[] = {
        // Face arrière
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    
        // Face avant
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    
        // Face gauche
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    
        // Face droite
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
    
        // Face inférieure
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
    
        // Face supérieure
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    const float PI = 3.14159265359f;

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            sphereVertices.push_back(xPos);
            sphereVertices.push_back(yPos);
            sphereVertices.push_back(zPos);
            sphereVertices.push_back(xPos);
            sphereVertices.push_back(yPos);
            sphereVertices.push_back(zPos);
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        if (!oddRow) {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
                sphereIndices.push_back(y * (X_SEGMENTS + 1) + x);
                sphereIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        } else {
            for (int x = X_SEGMENTS; x >= 0; --x) {
                sphereIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                sphereIndices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
    GLuint sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), &sphereIndices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
// fin shpère


    GLuint smallVBO, smallVAO;
    glGenVertexArrays(1, &smallVAO);
    glGenBuffers(1, &smallVBO);

    glBindVertexArray(smallVAO);

    glBindBuffer(GL_ARRAY_BUFFER, smallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(smallCubeVertices), smallCubeVertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint largeVBO, largeVAO;
    glGenVertexArrays(1, &largeVAO);
    glGenBuffers(1, &largeVBO);

    glBindVertexArray(largeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, largeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(smallCubeVertices), smallCubeVertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Définir les positions de la lumière et de la caméra

    glm::vec3 viewPos(0.0f, 0.0f, 3.0f);
    


    
    
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        
        // Calcul de la matrice de la lumière
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 20.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // 1ère passe : rendu de la profondeur depuis la vue de la lumière
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(depthShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(depthShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        // Dessiner chaque objet avec le shader de profondeur
        {
            // ...existing code pour dessiner le cube (par exemple)...
            glm::mat4 modelDepth = glm::mat4(1.0f);
            modelDepth = glm::translate(modelDepth, centerSquare1);
            modelDepth = glm::scale(modelDepth, sizeSquare1);
            glUniformMatrix4fv(glGetUniformLocation(depthShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelDepth));
            glBindVertexArray(smallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        // ... répéter pour les murs, sphères ... (utilisez des placeholders pour le reste)
        // ...existing rendering code for depth pass...
        // Ajout de la profondeur pour la première sphère
        {
            glm::mat4 modelDepthSphere1 = glm::mat4(1.0f);
            modelDepthSphere1 = glm::translate(modelDepthSphere1, centerSphere1);
            modelDepthSphere1 = glm::scale(modelDepthSphere1, sizeSphere1);
            glUniformMatrix4fv(glGetUniformLocation(depthShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelDepthSphere1));
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);
        }

        // Ajout de la profondeur pour la deuxième sphère
        {
            glm::mat4 modelDepthSphere2 = glm::mat4(1.0f);
            modelDepthSphere2 = glm::translate(modelDepthSphere2, centerSphere2);
            modelDepthSphere2 = glm::scale(modelDepthSphere2, sizeSphere2);
            glUniformMatrix4fv(glGetUniformLocation(depthShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelDepthSphere2));
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2ème passe : rendu de la scène en couleur avec ombres
        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        // Passer la matrice lightSpaceMatrix au shader principal
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        // Lier la shadow map sur l'unité de texture 1 et configurer la uniform
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(shaderProgram, "shadowMap"), 1);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        unsigned int fastSqrtLoc = glGetUniformLocation(shaderProgram, "fastSqrt");
        unsigned int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        
        unsigned int centerSphere1Loc = glGetUniformLocation(shaderProgram, "centerSphere1");
        unsigned int centerSphere2Loc = glGetUniformLocation(shaderProgram, "centerSphere2");
        unsigned int centerSquare1Loc = glGetUniformLocation(shaderProgram, "centerSquare1");

        unsigned int sizeSphere1Loc = glGetUniformLocation(shaderProgram, "sizeSphere1");
        unsigned int sizeSphere2Loc = glGetUniformLocation(shaderProgram, "sizeSphere2");
        unsigned int sizeSquare1Loc = glGetUniformLocation(shaderProgram, "sizeSquare1");


        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, &viewPos[0]);
        glUniform1i(fastSqrtLoc, fastSqrt);

        glm::vec3 blueColor(0.0f, 0.0f, 1.0f);
        glm::vec3 redColor(1.0f, 0.0f, 0.0f);
        glm::vec3 greenColor(0.0f, 1.0f, 0.0f);
        glm::vec3 whiteColor(1.0f, 1.0f, 1.0f);
        
        
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
        
        glBindVertexArray(smallVAO);
        glm::mat4 smallCubeModel = glm::mat4(1.0f);
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(redColor));

        smallCubeModel = glm::translate(smallCubeModel, centerSquare1); // Positionner le cube
        smallCubeModel = glm::scale(smallCubeModel, sizeSquare1);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(smallCubeModel));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Mur arrière
        glm::mat4 wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(0.0f, 0.0f, -PlaceSize));
        wallModel = glm::scale(wallModel, glm::vec3(10.0f, 10.0f, wallWidth));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Mur avant
        wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(0.0f, 0.0f, PlaceSize));
        wallModel = glm::scale(wallModel, glm::vec3(10.0f, 10.0f, wallWidth));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Mur gauche
        wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(-PlaceSize, 0.0f, 0.0f));
        wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        wallModel = glm::scale(wallModel, glm::vec3(10.0f, 10.0f, wallWidth));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Mur droit
        wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(PlaceSize, 0.0f, 0.0f));
        wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        wallModel = glm::scale(wallModel, glm::vec3(10.0f, 10.0f, wallWidth));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Plafond
        wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(0.0f, PlaceSize, 0.0f));
        wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        wallModel = glm::scale(wallModel, glm::vec3(10.0f, 10.0f, wallWidth));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Sol
        wallModel = glm::mat4(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(0.0f, -PlaceSize, 0.0f));
        wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        wallModel = glm::scale(wallModel, glm::vec3(10.0f, 10.0f, wallWidth));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
        glDrawArrays(GL_TRIANGLES, 0, 36);


        glBindVertexArray(sphereVAO);

        glUniform3fv(objectColorLoc, 1, glm::value_ptr(greenColor));
        glm::mat4 sphereModel1 = glm::mat4(1.0f);
        sphereModel1 = glm::translate(sphereModel1, centerSphere1); // Positionner la sphère
        sphereModel1 = glm::scale(sphereModel1, sizeSphere1);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel1));
        glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        glUniform3fv(objectColorLoc, 1, glm::value_ptr(blueColor));
        glm::mat4 sphereModel2 = glm::mat4(1.0f);
        sphereModel2 = glm::translate(sphereModel2, centerSphere2); // Positionner la sphère
        sphereModel2 = glm::scale(sphereModel2,sizeSphere2);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel2));
        glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        


        // sphère
        glUseProgram(sphereShaderProgram);
        // Récupérer les emplacements des uniforms requis
        modelLoc = glGetUniformLocation(sphereShaderProgram, "model");
        viewLoc = glGetUniformLocation(sphereShaderProgram, "view");
        projectionLoc = glGetUniformLocation(sphereShaderProgram, "projection");
        unsigned int lightSpaceLoc = glGetUniformLocation(sphereShaderProgram, "lightSpaceMatrix");
        unsigned int sphereColorLoc = glGetUniformLocation(sphereShaderProgram, "sphereColor");

        // Passer view, projection et lightSpaceMatrix
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        // Lier la shadow map sur l'unité de texture 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(sphereShaderProgram, "shadowMap"), 1);

        // Dessiner la sphère verte
        {
            glm::mat4 sphereModel1 = glm::mat4(1.0f);
            sphereModel1 = glm::translate(sphereModel1, centerSphere1);
            sphereModel1 = glm::scale(sphereModel1, sizeSphere1);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel1));
            glUniform3fv(sphereColorLoc, 1, glm::value_ptr(greenColor));
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);
        }

        // Dessiner la sphère bleue
        {
            glm::mat4 sphereModel2 = glm::mat4(1.0f);
            sphereModel2 = glm::translate(sphereModel2, centerSphere2);
            sphereModel2 = glm::scale(sphereModel2, sizeSphere2);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel2));
            glUniform3fv(sphereColorLoc, 1, glm::value_ptr(blueColor));
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);
        }

        // sphère
        glUseProgram(sphereShaderProgram);

        modelLoc = glGetUniformLocation(sphereShaderProgram, "model");
        viewLoc = glGetUniformLocation(sphereShaderProgram, "view");
        projectionLoc = glGetUniformLocation(sphereShaderProgram, "projection");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
        glBindVertexArray(sphereVAO);
        glm::mat4 sphereModel = glm::mat4(1.0f);
        sphereModel = glm::translate(sphereModel, glm::vec3(lightPos)); // Positionner la sphère
        sphereModel = glm::scale(sphereModel, glm::vec3(0.1f, 0.1f, 0.1f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel));
        glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // Display FPS
        displayFPS(window);
    
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &smallVAO);
    glDeleteBuffers(1, &smallVBO);
    glDeleteVertexArrays(1, &largeVAO);
    glDeleteBuffers(1, &largeVBO);

    glfwTerminate();
    return 0;
}