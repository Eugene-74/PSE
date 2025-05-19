#pragma once

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
#include <sstream>
#include <map>
#include <string>


GLFWwindow* initOpenGL() ;
bool startWindow(GLFWwindow* window);



GLuint createDepthShaderProgram();
GLuint createShader(GLenum type, const char* source);
void framebuffer_size_callback(GLFWwindow* window, int width, int height) ;
GLuint createSphereShaderProgram() ;
GLuint createShaderProgram() ;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) ;
void mouse_callback(GLFWwindow* window, double xpos, double ypos) ;




class Simulation {
    public:
    Simulation (GLFWwindow* window, int windowWidth, int windowHeight);

    
    private:


    int windowWidth = 0;
    int windowHeight = 0;
    GLFWwindow* window = nullptr;

    const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;

    
    bool square = false;
    
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 size = glm::vec3(0.6f, 0.6f, 0.6f);

    float PlaceSize = 5.0f;
    float wallWidth = 0.001;

    glm::vec3 cameraPos = glm::vec3(2.0f, 0.0f, -2.0f);
    glm::vec3 lightPos = glm::vec3(0.0f, 2.0f, 0.0f);
    glm::vec3 initialLightPos = glm::vec3(0.0f, 2.0f, 0.0f);
    glm::vec3 viewPos = glm::vec3(0.0f, 0.0f, 3.0f);
    bool mouvLight = false;



    int sqrtMode = 1;


    std::vector<unsigned int> sphereIndices;

    GLuint depthMap;
    GLuint depthMapFBO;
    GLuint smallVBO, smallVAO;
    GLuint sphereVAO, sphereVBO, sphereEBO;

    glm::mat4 lightSpaceMatrix;

    GLuint shaderProgram = createShaderProgram();
    GLuint sphereShaderProgram = createSphereShaderProgram();
    GLuint depthShaderProgram = createDepthShaderProgram();


    glm::vec3 move(glm::vec3 toMove, glm::vec3 direction) ;
    void processInput(GLFWwindow *window) ;
    void displayFPS(GLFWwindow* window) ;

    void createDepthMap();
    void createCubeVAOandVBO();
    void createSphereVAOandVBOandEBO();
    void createLightSphere();

    void setLightSpaceMatrix();

    void createDepthObject();

    void createObject(unsigned int modelLoc, unsigned int objectColorLoc);
    void createWalls(unsigned int modelLoc, unsigned int objectColorLoc);
    void createWallAndObject();

    void createLightShere();


};





