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
glm::vec3 move(glm::vec3 toMove, glm::vec3 direction) ;
void processInput(GLFWwindow *window) ;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) ;
void mouse_callback(GLFWwindow* window, double xpos, double ypos) ;
void displayFPS(GLFWwindow* window) ;





