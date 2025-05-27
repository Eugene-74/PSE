#include "simulation.hpp"

std::map<int, std::string> SQRT_MODES = {
    {0, "rien"},
    {1, "sqrt"},
    {2, "bit shift"},
    {3, "bit shift and correct"},
    {4, "Herron"}
};

// Déclaration de la fonction pour le shader de profondeur
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

// Déclaration de la fonction pour le shader de réflexion
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 FragPos;
    in vec3 Normal;
    in vec4 FragPosLightSpace;
    
    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform int sqrtMode;
    uniform vec3 objectColor;
    uniform sampler2D shadowMap;
    
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
        if (sqrtMode == 0) {
            invLength = 1;
        }
        else if (sqrtMode == 1) {
            invLength = 1/sqrt(lengthSq);
        }
        else if (sqrtMode == 2) {
            invLength = fastInvSqrt(lengthSq);
        } 
        else if (sqrtMode == 3) {
            invLength = fastInvSqrtAndCorrect(lengthSq);
        } 
        else if (sqrtMode == 4) {
            invLength = sqrtHerron(lengthSq);
        }
        return v * invLength;
    }

    float ShadowCalculation(vec4 fragPosLightSpace, vec3 lightDir){
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;

        if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0 || projCoords.z > 1.0)
            return 0.0;

        float currentDepth = projCoords.z;
        float shadow = 0.0;
        float bias = max(0.05 * (1.0 - dot(Normal, lightDir)), 0.005);

        int samples = 2; // 5x5 kernel
        float offset = 1.0 / 4096.0; // Taille de la texture d'ombre

        for (int x = -samples; x <= samples; ++x) {
            for (int y = -samples; y <= samples; ++y) {
                float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * offset).r;
                shadow += currentDepth - bias  > closestDepth ? 1.0 : 0.0;
            }
        }
        shadow /= (samples * 2 + 1) * (samples * 2 + 1);

        return shadow;
    }

    void main()
    {
        vec3 norm = normaliser(Normal);


        vec3 lightDir = lightPos - FragPos;
        
        lightDir = normaliser(lightDir);

        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (1.0 + 0.05 * distance + 0.005 * (distance * distance));

        vec3 result = vec3(0.0);
        vec3 currentLightDir = lightDir;
        vec3 currentFragPos = FragPos;
        vec3 currentNorm = norm;
        float reflectionAttenuation = 1.0; 

        for (int i = 0; i < 3; i++) {
            float diff = max(dot(currentNorm, currentLightDir), 0.0);
            vec3 diffuse = attenuation * diff * objectColor * reflectionAttenuation;
            result += diffuse;

            currentLightDir = reflect(currentLightDir, currentNorm);
           
            currentFragPos += currentLightDir * 0.1; 
            vec3 newNorm = normaliser(currentFragPos - FragPos); 
            currentNorm = normaliser(mix(currentNorm, newNorm, 0.5)); 

            reflectionAttenuation *= 0.5; 
        }

        float shadow = ShadowCalculation(FragPosLightSpace,lightDir);
        result = result * (1.0 - shadow);

        FragColor = vec4(result, 1.0);
    }
)";

// Déclaration de la fonction pour le shader de profondeur
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

// Déclaration de la fonction pour le shader de profondeur
const char* depthFragmentShaderSource = R"(
#version 330 core
void main(){
    // Pas de couleur, seule la profondeur est écrite
}
)";


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

// Fonction pour créer un shader
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


// Fragment shader pour la sphère
const char* sphereFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    void main()
    {
        FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Couleur jaune
    }
    )";
    
// Fonction pour créer le programme shader de la sphère
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

// Fonction pour créer le programme shader principal
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


glm::vec3 color(0.0f, 0.0f, 1.0f);

glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.05f;

float lastX = 400, lastY = 300;
float yaw = -90.0f, pitch = 0.0f;
bool firstMouse = true;
float sensitivity = 0.1f;

// Fonction pour déplacer un objet dans l'espace 3D
glm::vec3 Simulation::move(glm::vec3 toMove, glm::vec3 direction) {
    float border =  1.0f;
    glm::vec3 newToMove = toMove + direction;
    float PlaceSizeWithBorder = PlaceSize - 0.2f;
    if (newToMove.x > PlaceSizeWithBorder) newToMove.x = PlaceSizeWithBorder;
    if (newToMove.x < -PlaceSizeWithBorder) newToMove.x = -PlaceSizeWithBorder;
    if (newToMove.y > PlaceSizeWithBorder) newToMove.y = PlaceSizeWithBorder;
    if (newToMove.y < -PlaceSizeWithBorder) newToMove.y = -PlaceSizeWithBorder;
    if (newToMove.z > PlaceSizeWithBorder) newToMove.z = PlaceSizeWithBorder;
    if (newToMove.z < -PlaceSizeWithBorder) newToMove.z = -PlaceSizeWithBorder;

    if (square == 0) {
        glm::vec3 diff = newToMove - center;
        float distance = glm::length(diff);
        if (distance < size.x + border) { // Ajout d'une bordure de 0.2 unités autour de la sphère
            newToMove = center + glm::normalize(diff) * (size.x + border);
        }
        
    } else{
        if (abs(newToMove.x - center.x) < size.x + border && abs(newToMove.y - center.y) < size.y + border && abs(newToMove.z - center.z) < size.z + border) {
            newToMove = toMove;
        }
    }

    return newToMove;
}

// Fonction pour traiter les entrées de l'utilisateur
void Simulation::processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        if(mouvLight){
            lightPos = move(lightPos, cameraSpeed * glm::vec3(1.0f, 0.0f, 0.0f));
        }else{
            cameraPos = move(cameraPos, cameraSpeed * cameraFront);
        }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        if(mouvLight){
            lightPos = move(lightPos, cameraSpeed * glm::vec3(-1.0f, 0.0f, 0.0f));
        }else{
            cameraPos = move(cameraPos, cameraSpeed * -cameraFront);
        }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        if(mouvLight){
            lightPos = move(lightPos, cameraSpeed * glm::vec3(0.0f, 0.0f, 1.0f));
        }else{
            cameraPos = move(cameraPos, cameraSpeed * -glm::normalize(glm::cross(cameraFront, cameraUp)));
        }
        
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        if(mouvLight){
            lightPos = move(lightPos, cameraSpeed * glm::vec3(0.0f, 0.0f, -1.0f));
        }else{
            cameraPos = move(cameraPos, cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp)));
        }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        if(mouvLight){
            lightPos = move(lightPos, cameraSpeed * glm::vec3(0.0f, 1.0f, 0.0f));
        }else{
            cameraPos = move(cameraPos, cameraSpeed * cameraUp);
        }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        if(mouvLight){
            lightPos = move(lightPos, cameraSpeed * glm::vec3(0.0f, -1.0f, 0.0f));
        }else{
            cameraPos = move(cameraPos, cameraSpeed * -cameraUp);
        }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        sqrtMode = 0;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        sqrtMode = 1;
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
        sqrtMode = 2;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        sqrtMode = 3;
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        sqrtMode = 4;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS){
        square = 0;
        lightPos = initialLightPos; 
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS){
        square = 1;
        lightPos = initialLightPos; 
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS){
        square = 2;
        lightPos = initialLightPos; 
    }
    if (glfwGetKey(window, GLFW_KEY_Q)== GLFW_PRESS)
        mouvLight = true;
    if (glfwGetKey(window, GLFW_KEY_E)== GLFW_PRESS)
        mouvLight = false;
}

// Fonction de callback pour le défilement de la souris (molette)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        cameraSpeed += yoffset * 0.0001f;
        if (cameraSpeed < 0.0001f) cameraSpeed = 0.0001f; 
    } else {
    }
}

// Fonction de callback pour la souris
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


// Fonction pour afficher le nombre d'images par seconde (FPS) dans le titre de la fenêtre et le mode actuellement utilisé
void Simulation::displayFPS(GLFWwindow* window) {
    static double previousSeconds = 0.0;
    static int frameCount = 0;
    double currentSeconds = glfwGetTime();
    double elapsedSeconds = currentSeconds - previousSeconds;

    if (elapsedSeconds > 0.25) {
        previousSeconds = currentSeconds;
        double fps = (double)frameCount / elapsedSeconds;
        std::ostringstream ss;
        ss.precision(3);    
        ss << std::fixed << fps << " FPS"<<" :: mode : "<<SQRT_MODES[sqrtMode];
        glfwSetWindowTitle(window, ss.str().c_str());
        frameCount = 0;
    }
    frameCount++;
}

void Simulation::createDepthMap(){
    glGenFramebuffers(1, &depthMapFBO);
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
}

void Simulation::createCubeVAOandVBO(){
    float cubeSize = 1.0f;
    float cubeVertices[] = {
        // Face arrière
        -cubeSize, -cubeSize, -cubeSize, 0.0f, 0.0f, -2*cubeSize,
        cubeSize, -cubeSize, -cubeSize, 0.0f, 0.0f, -2*cubeSize,
        cubeSize, cubeSize, -cubeSize, 0.0f, 0.0f, -2*cubeSize,
        cubeSize, cubeSize, -cubeSize, 0.0f, 0.0f, -2*cubeSize,
        -cubeSize, cubeSize, -cubeSize, 0.0f, 0.0f, -2*cubeSize,
        -cubeSize, -cubeSize, -cubeSize, 0.0f, 0.0f, -2*cubeSize,

        // Face avant
        -cubeSize, -cubeSize, cubeSize, 0.0f, 0.0f, 2*cubeSize,
        cubeSize, -cubeSize, cubeSize, 0.0f, 0.0f, 2*cubeSize,
        cubeSize, cubeSize, cubeSize, 0.0f, 0.0f, 2*cubeSize,
        cubeSize, cubeSize, cubeSize, 0.0f, 0.0f, 2*cubeSize,
        -cubeSize, cubeSize, cubeSize, 0.0f, 0.0f, 2*cubeSize,
        -cubeSize, -cubeSize, cubeSize, 0.0f, 0.0f, 2*cubeSize,

        // Face gauche
        -cubeSize, cubeSize, cubeSize, -2*cubeSize, 0.0f, 0.0f,
        -cubeSize, cubeSize, -cubeSize, -2*cubeSize, 0.0f, 0.0f,
        -cubeSize, -cubeSize, -cubeSize, -2*cubeSize, 0.0f, 0.0f,
        -cubeSize, -cubeSize, -cubeSize, -2*cubeSize, 0.0f, 0.0f,
        -cubeSize, -cubeSize, cubeSize, -2*cubeSize, 0.0f, 0.0f,
        -cubeSize, cubeSize, cubeSize, -2*cubeSize, 0.0f, 0.0f,

        // Face droite
        cubeSize, cubeSize, cubeSize, 2*cubeSize, 0.0f, 0.0f,
        cubeSize, cubeSize, -cubeSize, 2*cubeSize, 0.0f, 0.0f,
        cubeSize, -cubeSize, -cubeSize, 2*cubeSize, 0.0f, 0.0f,
        cubeSize, -cubeSize, -cubeSize, 2*cubeSize, 0.0f, 0.0f,
        cubeSize, -cubeSize, cubeSize, 2*cubeSize, 0.0f, 0.0f,
        cubeSize, cubeSize, cubeSize, 2*cubeSize, 0.0f, 0.0f,

        // Face inférieure
        -cubeSize, -cubeSize, -cubeSize, 0.0f, -2*cubeSize, 0.0f,
        cubeSize, -cubeSize, -cubeSize, 0.0f, -2*cubeSize, 0.0f,
        cubeSize, -cubeSize, cubeSize, 0.0f, -2*cubeSize, 0.0f,
        cubeSize, -cubeSize, cubeSize, 0.0f, -2*cubeSize, 0.0f,
        -cubeSize, -cubeSize, cubeSize, 0.0f, -2*cubeSize, 0.0f,
        -cubeSize, -cubeSize, -cubeSize, 0.0f, -2*cubeSize, 0.0f,

        // Face supérieure
        -cubeSize, cubeSize, -cubeSize, 0.0f, 2*cubeSize, 0.0f,
        cubeSize, cubeSize, -cubeSize, 0.0f, 2*cubeSize, 0.0f,
        cubeSize, cubeSize, cubeSize, 0.0f, 2*cubeSize, 0.0f,
        cubeSize, cubeSize, cubeSize, 0.0f, 2*cubeSize, 0.0f,
        -cubeSize, cubeSize, cubeSize, 0.0f, 2*cubeSize, 0.0f,
        -cubeSize, cubeSize, -cubeSize, 0.0f, 2*cubeSize, 0.0f};

    glGenVertexArrays(1, &smallVAO);
    glGenBuffers(1, &smallVBO);

    glBindVertexArray(smallVAO);

    glBindBuffer(GL_ARRAY_BUFFER, smallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Simulation::createSphereVAOandVBOandEBO(){
    std::vector<float> sphereVertices;
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
}

void Simulation::setLightSpaceMatrix() {
    glm::mat4 lightProjection = glm::perspective(glm::radians(45.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, 0.5f, 50.0f);

    glm::vec3 sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f) + center;
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    lightSpaceMatrix = lightProjection * lightView;
}

void Simulation::createDepthObject(){
    glUseProgram(depthShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(depthShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));


    if (square == 0) {
        glm::mat4 modelDepthSphere = glm::mat4(1.0f);
        modelDepthSphere = glm::translate(modelDepthSphere, center);
        modelDepthSphere = glm::scale(modelDepthSphere, size);
        glUniformMatrix4fv(glGetUniformLocation(depthShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelDepthSphere));
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);
    } if(square == 1) {
        glm::mat4 modelDepth = glm::mat4(1.0f);
        modelDepth = glm::translate(modelDepth, center);
        modelDepth = glm::scale(modelDepth, size);
        glUniformMatrix4fv(glGetUniformLocation(depthShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelDepth));
        glBindVertexArray(smallVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
    } 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Simulation::createObject(unsigned int modelLoc, unsigned int objectColorLoc) {
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(color));

    if (square == 0) {
        glBindVertexArray(sphereVAO);
        
        glm::mat4 sphereModel1 = glm::mat4(1.0f);
        sphereModel1 = glm::translate(sphereModel1, center);
        sphereModel1 = glm::scale(sphereModel1, size);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel1));
        glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);
    }if(square == 1){
        glBindVertexArray(smallVAO);
        
        glm::mat4 smallCubeModel = glm::mat4(1.0f);
        smallCubeModel = glm::translate(smallCubeModel, center);
        smallCubeModel = glm::scale(smallCubeModel, size);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(smallCubeModel));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

void Simulation::createWalls(unsigned int modelLoc, unsigned int objectColorLoc){

    glm::vec3 whiteColor(1.0f, 1.0f, 1.0f);
        
    glBindVertexArray(smallVAO);
    // Mur arrière
    glm::mat4 wallModel = glm::mat4(1.0f);
    wallModel = glm::translate(wallModel, glm::vec3(0.0f, 0.0f, -PlaceSize));
    wallModel = glm::scale(wallModel, glm::vec3(PlaceSize * 2, PlaceSize * 2, wallWidth));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Mur avant
    wallModel = glm::mat4(1.0f);
    wallModel = glm::translate(wallModel, glm::vec3(0.0f, 0.0f, PlaceSize));
    wallModel = glm::scale(wallModel, glm::vec3(PlaceSize * 2, PlaceSize * 2, wallWidth));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Mur gauche
    wallModel = glm::mat4(1.0f);
    wallModel = glm::translate(wallModel, glm::vec3(-PlaceSize, 0.0f, 0.0f));
    wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    wallModel = glm::scale(wallModel, glm::vec3(PlaceSize * 2, PlaceSize * 2, wallWidth));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Mur droit
    wallModel = glm::mat4(1.0f);
    wallModel = glm::translate(wallModel, glm::vec3(PlaceSize, 0.0f, 0.0f));
    wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    wallModel = glm::scale(wallModel, glm::vec3(PlaceSize * 2, PlaceSize * 2, wallWidth));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Plafond
    wallModel = glm::mat4(1.0f);
    wallModel = glm::translate(wallModel, glm::vec3(0.0f, PlaceSize, 0.0f));
    wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    wallModel = glm::scale(wallModel, glm::vec3(PlaceSize * 2, PlaceSize * 2, wallWidth));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Sol
    wallModel = glm::mat4(1.0f);
    wallModel = glm::translate(wallModel, glm::vec3(0.0f, -PlaceSize, 0.0f));
    wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    wallModel = glm::scale(wallModel, glm::vec3(PlaceSize * 2, PlaceSize * 2, wallWidth));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModel));
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Simulation::createWallAndObject(){
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int sqrtModeLoc = glGetUniformLocation(shaderProgram, "sqrtMode");
    unsigned int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, &lightPos[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, &viewPos[0]);
    glUniform1i(sqrtModeLoc, sqrtMode);

    
    createObject(modelLoc, objectColorLoc);
    
    createWalls(modelLoc, objectColorLoc);
}

void Simulation::createLightSphere(){
    glUseProgram(sphereShaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    unsigned int modelLoc = glGetUniformLocation(sphereShaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(sphereShaderProgram, "view");
    unsigned int projectionLoc = glGetUniformLocation(sphereShaderProgram, "projection");
    unsigned int lightSpaceLoc = glGetUniformLocation(sphereShaderProgram, "lightSpaceMatrix");
    unsigned int sphereColorLoc = glGetUniformLocation(sphereShaderProgram, "sphereColor");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glUniform1i(glGetUniformLocation(sphereShaderProgram, "shadowMap"), 1);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(sphereVAO);
    glm::mat4 sphereModel = glm::mat4(1.0f);
    sphereModel = glm::translate(sphereModel, glm::vec3(lightPos)); // Positionner la sphère
    sphereModel = glm::scale(sphereModel, glm::vec3(0.1f, 0.1f, 0.1f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel));
    glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);

}

Simulation::Simulation(GLFWwindow* window, int windowWidth, int windowHeight) : window(window), windowWidth(windowWidth), windowHeight(windowHeight) {
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    createDepthMap();

    createSphereVAOandVBOandEBO();
    createCubeVAOandVBO();

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        setLightSpaceMatrix();
        
        createDepthObject();
        
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        createWallAndObject();

        createLightSphere();

        displayFPS(window);
    
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}

bool startSimulation(GLFWwindow* window){    
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int screenWidth = mode->width;
    int screenHeight = mode->height;
    
    int windowWidth = screenWidth - 100;
    int windowHeight = screenHeight - 100;
    
    glfwSetWindowSize(window, windowWidth, windowHeight);
    
    int windowPosX = (screenWidth - windowWidth) / 2;
    int windowPosY = (screenHeight - windowHeight) / 2;
    glfwSetWindowPos(window, windowPosX, windowPosY);
    
    if (!window) return -1;

    Simulation* simulation = new Simulation(window,windowWidth,windowHeight);
    
    return 0;
}


int main() {
    GLFWwindow* window = initOpenGL();
    return startSimulation(window);
    
}