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

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <string>


// Structure pour stocker les informations sur chaque caractère
struct Character {
    GLuint TextureID;  // ID de la texture glyph
    glm::ivec2 Size;   // Taille du glyph
    glm::ivec2 Bearing; // Offset du glyph par rapport à la ligne de base
    GLuint Advance;    // Offset horizontal pour avancer au prochain glyph
};

std::map<GLchar, Character> Characters;
GLuint VAO, VBO;

// Shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";



const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 FragPos;
    in vec3 Normal;
    
    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform bool fastSqrt;
    uniform vec3 objectColor;
    
    uniform vec3 centerSphere1;
    uniform vec3 centerSphere2;
    uniform vec3 centerSquare1;

    uniform vec3 sizeSphere1;
    uniform vec3 sizeSphere2;
    uniform vec3 sizeSquare1;


    float simpleSqrt(float nbr) {
    float epsilon = 0.01;
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

    // Approximation de la racine carrée inverse
    float fastInvSqrt(float x) {
        int i = floatBitsToInt(x);
        i = 0x5F3759DF - (i >> 1);
        float y = intBitsToFloat(i);
        return y * (1.5 - 0.5 * x * y * y);
    }
    
    float sqrtHerron(float number) {
        float x = number;
        
        for (int i = 0; i < 10; i++) {
            x = (x + number / x) / 2;
        }

    return 1 / x;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = lightPos - FragPos;
    
    float lengthSq = dot(lightDir, lightDir);
    
    float invLength;
    if (fastSqrt) {
        invLength = fastInvSqrt(lengthSq);
    } else {
        invLength = simpleSqrt(lengthSq);
    }
    lightDir *= invLength;

    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.5 * (distance * distance));

    // Diffuse component for lightDir
    float diff1 = max(dot(norm, lightDir), 0.0);
    vec3 diffuse1 = attenuation * diff1 * objectColor;

    // Diffuse component for -lightDir
    float diff2 = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse2 = (attenuation * diff2 * objectColor)*0.5; 
    diffuse2 = vec3(0.0, 0.0, 0.0);

    // Specular component for lightDir
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = attenuation * spec * vec3(1.0, 1.0, 1.0);

    // Combine both diffuse components
    vec3 result = diffuse1 + diffuse2 + specular;



    FragColor = vec4(result, 1);
}
)";


    const char* fragmentShaderSourceBis = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform bool fastSqrt;
        uniform vec3 objectColor;
        
        float simpleSqrt(float nbr) {
        float epsilon = 0.01;
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
    
        // Approximation de la racine carrée inverse
        float fastInvSqrt(float x) {
            int i = floatBitsToInt(x);
            i = 0x5F3759DF - (i >> 1);
            float y = intBitsToFloat(i);
            return y * (1.5 - 0.5 * x * y * y);
        }
        
        float sqrtHerron(float number) {
            float x = number;
            
            for (int i = 0; i < 10; i++) {
                x = (x + number / x) / 2;
            }
    
        return 1 / x;
    }
    
        void main()
        {
           vec3 norm = normalize(Normal);
            vec3 lightDir = lightPos - FragPos;
            
            float lengthSq = dot(lightDir, lightDir);
           
            float invLength;
            if (fastSqrt) {
                invLength = fastInvSqrt(lengthSq);
            } else {
                invLength = simpleSqrt(lengthSq);
            }
            lightDir *= invLength;
    
            float distance = length(lightPos - FragPos);
            float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.5 * (distance * distance));
    
            // Diffuse component for lightDir
            float diff1 = max(dot(norm, lightDir), 0.0);
            // vec3 diffuse1 = attenuation * diff1;
            vec3 diffuse1 = attenuation * diff1 * objectColor;
            
            diffuse1 = vec3(0.0, 0.0, 0.0);
            
            // Diffuse component for -lightDir
            float diff2 = max(dot(norm, -lightDir), 0.0);
            // vec3 diffuse2 = attenuation * diff2 ;
            vec3 diffuse2 = attenuation * diff2 * objectColor;
    
            // Specular component for lightDir
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
            vec3 specular = attenuation * spec * vec3(1.0, 1.0, 1.0); // White light
    
            // Combine both diffuse components
            vec3 result = diffuse1 + diffuse2 + specular;
    
            FragColor = vec4(result, 1);
        }
        )";



        // Compilation des shaders
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





// Fonction pour créer le shader pour le texte
GLuint createTextShaderProgram() {
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
    out vec2 TexCoords;

    uniform mat4 projection;

    void main() {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
    )";

    const char* fragmentShaderSourceText = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 color;

    uniform sampler2D text;
    uniform vec3 textColor;

    void main() {
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        color = vec4(textColor, 1.0) * sampled;
    }
    )";

    GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSourceText);

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




// std::map<GLchar, Character> Characters;
// GLuint VAO, VBO;

// Fonction pour initialiser FreeType et charger les caractères
void initFreeType() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not init FreeType Library" << std::endl;
        return;
    }

    FT_Face face;
    if (FT_New_Face(ft, "font.ttf", 0, &face)) {
        std::cerr << "Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Désactiver l'alignement des pixels

    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph" << std::endl;
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}



// Fonction pour afficher du texte
void RenderText(GLuint shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // x += (ch.Advance >> 6) * scale;
        // std::cerr << "scale: " << scale << std::endl;
        // std::cerr << "ch.Advance: " << ch.Advance << std::endl;
        // std::cerr << "ch.Advance >> 6: " << (ch.Advance >> 6) * scale<< std::endl;
        x += (ch.Advance >> 6) * scale;

    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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

GLuint createShaderProgramBis() {
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSourceBis);

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



// Variables pour la position de la caméra
glm::vec3 cameraPos = glm::vec3(2.0f, 0.0f, -2.0f);
glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 centerSphere1 = glm::vec3(-2.0f, 0.0f, 0.0f);
glm::vec3 centerSphere2 = glm::vec3(0.0f, 0.0f, 2.0f);
glm::vec3 centerSquare1 = glm::vec3(0.0f, 2.0f, 0.0f);

glm::vec3 sizeSphere1 = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 sizeSphere2 = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 sizeSquare1 = glm::vec3(1.0f, 1.0f, 1.0f);





bool fastSqrt = true;


glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.005f; // ajustez la vitesse de la caméra

// Variables pour la gestion de la souris
float lastX = 400, lastY = 300;
float yaw = -90.0f, pitch = 0.0f;
bool firstMouse = true;
float sensitivity = 0.1f;

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        lightPos.y += cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        lightPos.y -= cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        lightPos.x -= cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        lightPos.x += cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
        fastSqrt = true;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        fastSqrt = false;


}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // change la puissnace de la lumiere
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
    float yoffset = lastY - ypos; // inversé car les coordonnées y vont de bas en haut
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Assurez-vous que lorsque le pitch est hors de ces limites, lZ'écran ne se retourne pas
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

int main() {
    GLFWwindow* window = initOpenGL();

    // Obtenir la taille de l'écran
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int screenWidth = mode->width;
    int screenHeight = mode->height;

    // Définir la taille de la fenêtre
    int windowWidth = 2880 - 100;
    int windowHeight = 1920 - 200;
    glfwSetWindowSize(window, windowWidth, windowHeight);

    // Calculer la position pour centrer la fenêtre
    int windowPosX = (screenWidth - windowWidth) / 2;
    int windowPosY = (screenHeight - windowHeight) / 2;
    glfwSetWindowPos(window, windowPosX, windowPosY);

    if (!window) return -1;

    initFreeType();


    // Définir le rappel de la souris et de la molette
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Capturer le curseur
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLuint shaderProgram = createShaderProgram();

    GLuint shaderProgramBis = createShaderProgramBis();

    GLuint sphereShaderProgram = createSphereShaderProgram();

    GLuint textShader = createTextShaderProgram();


    // Initialisation des données du petit cube
    float smallCubeVertices[] = {
        // positions          // normales
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

    // Génération des données de la sphère
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
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Définir la matrice de projection pour le texte
        glm::mat4 textProjection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
        glUseProgram(textShader);
        glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, glm::value_ptr(textProjection));
        
        RenderText(textShader, "test bis", 540.0f, 570.0f, 0.5f, glm::vec3(0.3, 0.7f, 0.9f));
        
        // Utiliser le shader pour le petit cube (avec éclairage)
        glUseProgram(shaderProgram);
    
        // Définir les matrices de transformation
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    
        // Passer les matrices aux shaders cube
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        unsigned int fastSqrtLoc = glGetUniformLocation(shaderProgram, "fastSqrt");
        unsigned int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        
        // unsigned int centerSphere1Loc = glGetUniformLocation(shaderProgram, "centerSphere1");
        // unsigned int centerSphere2Loc = glGetUniformLocation(shaderProgram, "centerSphere2");
        // unsigned int centerSquare1Loc = glGetUniformLocation(shaderProgram, "centerSquare1");

        // unsigned int sizeSphere1Loc = glGetUniformLocation(shaderProgram, "sizeSphere1");
        // unsigned int sizeSphere2Loc = glGetUniformLocation(shaderProgram, "sizeSphere2");
        // unsigned int sizeSquare1Loc = glGetUniformLocation(shaderProgram, "sizeSquare1");


        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Passer les positions de la lumière et de la caméra aux shaders
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, &viewPos[0]);
        glUniform1i(fastSqrtLoc, fastSqrt);

        glm::vec3 objectColor(1.0f, 1.0f, 1.0f);
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(objectColor));
        
        glm::vec3 blueColor(0.0f, 0.0f, 1.0f);
        glm::vec3 redColor(1.0f, 0.0f, 0.0f);
        glm::vec3 greenColor(0.0f, 1.0f, 0.0f);
        glm::vec3 whiteColor(1.0f, 1.0f, 1.0f);
        
        
        // Dessiner le petit cube
        glBindVertexArray(smallVAO);
        // glUniform3fv(objectColorLoc, 1, glm::value_ptr(blueColor));
        glm::mat4 smallCubeModel = glm::mat4(1.0f);
        smallCubeModel = glm::translate(smallCubeModel, centerSquare1); // Positionner le cube
        smallCubeModel = glm::scale(smallCubeModel, sizeSquare1);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(smallCubeModel));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        

        glBindVertexArray(sphereVAO);

        // glUniform3fv(objectColorLoc, 1, glm::value_ptr(greenColor));
        glm::mat4 sphereModel1 = glm::mat4(1.0f);
        sphereModel1 = glm::translate(sphereModel1, centerSphere1); // Positionner la sphère
        sphereModel1 = glm::scale(sphereModel1, sizeSphere1);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel1));
        glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // glUniform3fv(objectColorLoc, 1, glm::value_ptr(greenColor));
        glm::mat4 sphereModel2 = glm::mat4(1.0f);
        sphereModel2 = glm::translate(sphereModel2, centerSphere2); // Positionner la sphère
        sphereModel2 = glm::scale(sphereModel2,sizeSphere2);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel2));
        glDrawElements(GL_TRIANGLE_STRIP, sphereIndices.size(), GL_UNSIGNED_INT, 0);


        glUseProgram(shaderProgramBis);

        unsigned int modelLocBis = glGetUniformLocation(shaderProgramBis, "model");
        unsigned int viewLocBis = glGetUniformLocation(shaderProgramBis, "view");
        unsigned int projectionLocBis = glGetUniformLocation(shaderProgramBis, "projection");
        unsigned int fastSqrtLocBis = glGetUniformLocation(shaderProgramBis, "fastSqrt");

        glUniformMatrix4fv(modelLocBis, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLocBis, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLocBis, 1, GL_FALSE, glm::value_ptr(projection));

        // Passer les positions de la lumière et de la caméra aux shaders
        glUniform3fv(glGetUniformLocation(shaderProgramBis, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(shaderProgramBis, "viewPos"), 1, &viewPos[0]);
        glUniform1i(fastSqrtLocBis, fastSqrt);

        // Dessiner la pièce (grand cube)
        glBindVertexArray(largeVAO);
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(whiteColor));
        glm::mat4 largeCubeModel = glm::mat4(1.0f);
        largeCubeModel = glm::translate(largeCubeModel, glm::vec3(0.0f, 0.0f, 0.0f)); // Positionner le grand cube
        largeCubeModel = glm::scale(largeCubeModel, glm::vec3(10.0f, 10.0f, 10.0f)); // Définir la taille du grand cube
        glUniformMatrix4fv(modelLocBis, 1, GL_FALSE, glm::value_ptr(largeCubeModel));
        glDrawArrays(GL_TRIANGLES, 0, 36);

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