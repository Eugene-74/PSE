cmake_minimum_required(VERSION 3.10)
project(OpenGLTextRendering)

set(CMAKE_CXX_STANDARD 17)

# Inclure les répertoires d'en-tête
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(C:/msys64/mingw64/include)

# Ajouter les répertoires de bibliothèque
link_directories(${CMAKE_SOURCE_DIR}/lib)
link_directories(C:/msys64/mingw64/lib)

# Ajouter les fichiers source
add_executable(simulation simulation.cpp glad.c)

# Lier les bibliothèques
target_link_libraries(simulation
    glfw3
    opengl32
    gdi32
)