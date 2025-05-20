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
// marche pas avec des grands nombre
float sqrtHerron(float number, float epsilon = 0.0001, int maxIterations = 1000) {
    float x = number;
    int iteration = 0;
    // condition marche pas avec des grands nombre
    while (std::fabs(x * x - number) > epsilon && iteration < maxIterations) {
        x = (x + number / x) / 2;
        iteration += 1;
    }
    std::cerr << "Nombre d'itération :" << iteration << std::endl;

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
    std ::cerr << "Nombre d'itération :" << iteration << std::endl;
    return racine;
}

int main() {
    // float x = 25.0f;
    float x = 24742845.04737283f;
    float epsilon = 0.0001f;

    float expectedValue = 1 / std::sqrt(x);
    const int iterations = 100;

    // noraml sqrt
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile float result = 1 / std::sqrt(x);
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
    float result = 1 / std::sqrt(x);

    double average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "values : " << expectedValue << " -> " << result << std::endl;
    std::cerr << "Average time for normal sqrt: " << average_duration << " nanoseconds" << std::endl;
    std::cerr << std::endl;

    // Mesure du temps pour fast_invSqrt
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile float result = fast_invSqrt(x);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    result = fast_invSqrt(x);
    average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "values : " << expectedValue << " -> " << result << std::endl;
    std::cerr << "Average time for fast_invSqrt: " << average_duration << " nanoseconds" << std::endl;
    std::cerr << std::endl;

    // Mesure du temps pour fast_rsqrt
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile float result = fast_rsqrt(x);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
    result = fast_rsqrt(x);

    average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "values : " << expectedValue << " -> " << result << std::endl;
    std::cerr << "Average time for fast_rsqrt: " << average_duration << " nanoseconds" << std::endl;
    std::cerr << std::endl;

    // Mesure du temps pour fast_rsqrt avec correction
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile float result = fast_rsqrt(x, epsilon);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
    result = fast_rsqrt(x, epsilon);

    average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "values : " << expectedValue << " -> " << result << std::endl;
    std::cerr << "Average time for fast_rsqrt with correction: " << average_duration << " nanoseconds" << std::endl;
    std::cerr << std::endl;

    // Mesure du temps pour sqrt Simple
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile float result = 1 / simpleSqrt(x, epsilon);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    result = 1 / simpleSqrt(x, epsilon);
    average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "values : " << expectedValue << " -> " << result << std::endl;
    std::cerr << "Average time for sqrt simple: " << average_duration << " nanoseconds" << std::endl;
    std::cerr << std::endl;

    // Mesure du temps pour sqrt Herron
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile float result = sqrtHerron(x);
    }
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    result = sqrtHerron(x);
    average_duration = static_cast<double>(duration) / iterations;
    std::cerr << "values : " << expectedValue << " -> " << result << std::endl;
    std::cerr << "Average time for sqrt Herron: " << average_duration << " nanoseconds" << std::endl;
    std::cerr << std::endl;

    return 0;
}