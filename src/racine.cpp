// g++ -o racine racine.cpp
// ./racine

#include <xmmintrin.h>

#include <chrono>
#include <cmath>
#include <iostream>

// méthode processeur
inline float fast_invSqrt(float x) {
    // Charge x dans un registre SIMD, calcule la racine inverse et extrait le résultat
    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
}

// méthode décalage de bits
float fast_rsqrt(float number, float epsilon = -1) {
    int i;
    float y, x2;
    const float threehalfs = 1.5F;
    x2 = number * 0.5F;

    y = number;
    i = *(int*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;

    if (epsilon != -1) {
        y = y * (threehalfs - (x2 * y * y));
        y = y * (threehalfs - (x2 * y * y));
    }

    return y;
}

// racine avec méthode de Heron
float sqrtHerron(float number, float epsilon = 0.0001) {
    float x = number;
    // condition marche pas avec des grands nombre
    for (int j = 0; j < 10; j++) {
        x = (x + number / x) / 2;
    }

    return 1 / x;
}

float simpleSqrt(float nbr, float epsilon) {
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
    // std ::cerr << "Nombre d'itération :" << iteration << std::endl;
    return racine;
}

float getX(){
    // float x = 2843843.44242;
    // nombre aléatoire entre 1 et 100000 à virgule
    float x = 1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (100000.0f - 1.0f)));
    return x;
}

int main() {
    float epsilon = 0.0001f;
    
    const int iterations = 10000000;
    
    // noraml sqrt
    auto start = std::chrono::high_resolution_clock::now();
    float error = 0;
    for (int i = 0; i < iterations; ++i) {
        float x = getX();
        volatile float result = 1 / std::sqrt(x);
        float expectedValue = 1 / std::sqrt(x);
        error += std::abs(expectedValue - result);
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    double average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "Average time for normal sqrt: " << average_duration << " nanoseconds" << " :: with average error : "<< error/iterations << std::endl;
    std::cerr << std::endl;

    // Mesure du temps pour fast_invSqrt
    start = std::chrono::high_resolution_clock::now();
    error = 0;
    for (int i = 0; i < iterations; ++i) {
        float x = getX();
        volatile float result = fast_invSqrt(x);
        float expectedValue = 1 / std::sqrt(x);
        error += std::abs(expectedValue - result);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "Average time for fast_invSqrt: " << average_duration << " nanoseconds" << " :: with average error : "<< error/iterations << std::endl;
    std::cerr << std::endl;

    // Mesure du temps pour fast_rsqrt
    start = std::chrono::high_resolution_clock::now();
    error = 0;
    for (int i = 0; i < iterations; ++i) {
        float x = getX();
        volatile float result = fast_rsqrt(x);
        float expectedValue = 1 / std::sqrt(x);
        error += std::abs(expectedValue - result);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "Average time for fast_rsqrt: " << average_duration << " nanoseconds" << " :: with average error : "<< error/iterations << std::endl;
    std::cerr << std::endl;

    
    // Mesure du temps pour fast_rsqrt avec correction
    error = 0;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        float x = getX();
        volatile float result = fast_rsqrt(x, epsilon);
        float expectedValue = 1 / std::sqrt(x);
        error += std::abs(expectedValue - result);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
    average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "Average time for fast_rsqrt with correction: " << average_duration << " nanoseconds" << " :: with average error : "<< error/iterations << std::endl;
    std::cerr << std::endl;
    
    
    // Mesure du temps pour sqrt Simple
    if(iterations > 1000){
        std::cerr << "too many iteration for simple sqrt (1000 max)" << std::endl;

    }else{
        start = std::chrono::high_resolution_clock::now();
        error = 0;
        for (int i = 0; i < iterations; ++i) {
            float x = getX();
            volatile float result = 1 / simpleSqrt(x, epsilon);
            float expectedValue = 1 / std::sqrt(x);
            error += std::abs(expectedValue - result);
        }
        stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
        
        average_duration = static_cast<double>(duration) / iterations;
        std::cerr << "Average time for sqrt simple: " << average_duration << " nanoseconds" << " :: with average error : "<< error/iterations << std::endl;
        std::cerr << std::endl;
    }

    // Mesure du temps pour sqrt Herron
    start = std::chrono::high_resolution_clock::now();
    error = 0;
    for (int i = 0; i < iterations; ++i) {
        float x = getX();
        volatile float result = sqrtHerron(x);
        float expectedValue = 1 / std::sqrt(x);
        error += std::abs(expectedValue - result);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "Average time for sqrt Herron: " << average_duration << " nanoseconds" << " :: with average error : "<< error/iterations << std::endl;
    std::cerr << std::endl;

    return 0;
}
