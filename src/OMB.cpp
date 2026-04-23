#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <chrono>

#include "../includes/MB.h"

#ifndef PROGRAM_TYPE
#define PROGRAM_TYPE "OMB"
#endif

typedef union {
    int   Intarr[8];
    float Farr[8];
} m256;

const int    F_PER_REGEX    = 8;

#define INIT_START_COUNTER(name) \
    auto name = std::chrono::steady_clock::now();

#define CALCULATE_MB_ITERATION(X, Y, X0, Y0, N){                                                    \
    m256 X2 = mm256_mul_ps(X, X);                                                                   \
    m256 Y2 = mm256_mul_ps(Y, Y);                                                                   \
    m256 XY = mm256_mul_ps(X, Y);                                                                   \
                                                                                                    \
    m256 Mask   = mm256_cmp_ps(mm256_add_ps(X2, Y2), MAX_DISTANCE);                                 \
    int IntMask = mm256_movemask_epi32(Mask);                                                       \
    if (!IntMask)                                                                                   \
        break;                                                                                      \
                                                                                                    \
    X = mm256_add_ps(mm256_sub_ps(X2, Y2), X0);                                                     \
    Y = mm256_add_ps(mm256_add_ps(XY, XY), Y0);                                                     \
    N = mm256_sub_epi32(N, Mask);                                                                   \
}                                                                                                   \

#define SAVE_SECONDS_TO_FILE(fp, StartName, FramesElapsed) {                                        \
    auto end    = std::chrono::steady_clock::now();                                                 \
    auto delta  = std::chrono::duration_cast<std::chrono::nanoseconds>(end - StartName).count();    \
    size_t TimePerFrame = delta;                                                                    \
                                                                                                    \
    if (FramesElapsed > NUM_OF_ITERATIONS + CACHE_WARMUP_ITERATIONS) {                              \
        BreakFlag = true;                                                                           \
        break;                                                                                      \
    }                                                                                               \
    if (FramesElapsed >= CACHE_WARMUP_ITERATIONS)                                                   \
        fprintf(fp, "%zu, %zu\n", FramesElapsed - CACHE_WARMUP_ITERATIONS, TimePerFrame);           \
}                                                                                                   \

void PrintColorsToTexture(size_t *ColorIndex, int FinalColorValues[], Color MandelbrotPixels[]);
void DrawTextureToTheScreen(Texture MandelbrotTexture, float *dx, float *y0_OUTLoop, float *x0_OUTLoop, int *Speed);

inline m256 mm256_add_ps(m256 a, m256 b);
inline m256 mm256_set1_epi32(int a);
inline int  mm256_movemask_epi32(m256 a);
inline m256 mm256_cmp_ps(m256 a, float b);
inline m256 mm256_add_epi32(m256 a, m256 b);
inline m256 mm256_sub_epi32(m256 a, m256 b);
inline m256 mm256_sub_ps(m256 a, m256 b);
inline m256 mm256_mul_ps(m256 a, m256 b);
inline m256 mm256_set1_ps(float a);
inline m256 mm256_div_ps(m256 a, m256 b);
inline m256 mm256_set_ps(float e7, float e6, float e5, float e4,
                         float e3, float e2, float e1, float e0);

int main(void) {
    #ifndef NDRAW
        InitWindow(WindowWidth, WindowHeight, "Mandelbrot");

        Color *MandelbrotPixels = (Color *) calloc(WindowHeight * WindowWidth, sizeof(Color));
        RenderTexture2D MandelbrotTexture = LoadRenderTexture(800, 800);
    #endif
    #ifndef NMEASURE
        size_t FramesElapsed = 0;
        FILE *fp = fopen(DISTR_STR, "w");
        assert(fp);
    #endif
    
    float dx            = dx_START;
    float y0_OUTLoop    = Y_START;
    float x0_OUTLoop    = X_START;
    int   Speed         = Speed_START;
    int   control_sum   = 0;
    bool  BreakFlag     = false;
    m256 Indexes = mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0);

    #ifndef NDRAW
        while (!WindowShouldClose() && !BreakFlag) {
    #else
        while (true && !BreakFlag) {
    #endif
        m256 DX = mm256_set1_ps(dx);

        size_t ColorIndex = 0;

        for (size_t inty = 0; inty < WindowHeight; inty++) { 
            float y0 = y0_OUTLoop + inty * dx;
            m256  Y0 = mm256_set1_ps(y0);

            for (size_t intx = 0; intx < WindowWidth; intx += F_PER_REGEX) {
                m256  N = {};
                #ifndef NMEASURE
                    for (size_t FrameCnt = 0; FrameCnt < MEASUREMENTS_PER_FRAME && !BreakFlag; FrameCnt++) {
                        INIT_START_COUNTER(start);
                #endif

                float x0 = x0_OUTLoop + intx * dx;
                
                N  = mm256_set1_epi32(MAX_ITERATIONS);  
                m256  X  = mm256_add_ps(mm256_set1_ps(x0), mm256_mul_ps(Indexes, DX));
                m256  X0 = X;
                m256  Y  = Y0;
                
                for (int n = 0; n < MAX_ITERATIONS; n++) {
                    CALCULATE_MB_ITERATION(X, Y, X0, Y0, N);
                }

                #ifndef NMEASURE
                    SAVE_SECONDS_TO_FILE(fp, start, FramesElapsed);
                    }
                #endif

                #ifndef NDRAW
                    PrintColorsToTexture(&ColorIndex, N.Intarr, MandelbrotPixels);
                #else
                    for (size_t i = 0; i < F_PER_REGEX; i++) {
                        control_sum += N.Intarr[i];
                    }
                #endif
            }
        }
        // =====================Calculated All The Pixels======================
        #ifndef NDRAW
            UpdateTexture(MandelbrotTexture.texture, (void *) MandelbrotPixels);
            DrawTextureToTheScreen(MandelbrotTexture.texture, &dx, &y0_OUTLoop, &x0_OUTLoop, &Speed);
        #endif

        #ifndef NMEASURE
            FramesElapsed++;
        #endif
    }

    #ifndef NDRAW
        free(MandelbrotPixels);
        CloseWindow();
    #else
        printf("%d", control_sum);
    #endif
    #ifndef NMEASURE
        fclose(fp);
    #endif
    return 0;
}                                                                                          \

void PrintColorsToTexture(size_t *ColorIndex, int FinalColorValues[], Color MandelbrotPixels[]) {
    for (size_t i = 0; i < F_PER_REGEX; i++) {
        unsigned char ColorValue = (unsigned char) FinalColorValues[i];
        MandelbrotPixels[(*ColorIndex)++] = (Color){ColorValue,
                                                    ColorValue,
                                                    static_cast<unsigned char> (ColorValue%2*128),
                                                    255};
    }
}

void DrawTextureToTheScreen(Texture MandelbrotTexture, float *dx, float *y0_OUTLoop, float *x0_OUTLoop, int *Speed) {
    BeginDrawing();
    // ===============printing mandelbrot set to the window================
        ClearBackground(WHITE);
        DrawTexture(MandelbrotTexture, 0, 0, WHITE);
        DrawFPS(10, 10);
        char str[MAX_STR_SIZE] = {};

        snprintf(str, MAX_STR_SIZE - 1, "dx: %g\n"
                                        "y0: %g\n"
                                        "x0: %g\n"
                                        "speed: %d\n",
                                    *dx, *y0_OUTLoop, *x0_OUTLoop, *Speed);
        if (IsKeyDown(KEY_DOWN))    *y0_OUTLoop    += *dx * *Speed;
        if (IsKeyDown(KEY_RIGHT))   *x0_OUTLoop    += *dx * *Speed;
        if (IsKeyDown(KEY_UP))      *y0_OUTLoop    -= *dx * *Speed;
        if (IsKeyDown(KEY_LEFT))    *x0_OUTLoop    -= *dx * *Speed;
        if (IsKeyDown(KEY_ONE) && *Speed < 20) (*Speed)++;
        if (IsKeyDown(KEY_TWO) && *Speed > 0)  (*Speed)--;
        if (IsKeyDown(KEY_Z)) {
                                    float CordsDelta = *Speed * *dx;
                                    *y0_OUTLoop  += CordsDelta;
                                    *x0_OUTLoop  += CordsDelta;
                                    *dx          -= *Speed / 400.f * *dx;
        }
        if (IsKeyDown(KEY_X)) {
                                    float CordsDelta = *Speed * *dx;
                                    *y0_OUTLoop  -= CordsDelta;
                                    *x0_OUTLoop  -= CordsDelta;
                                    *dx          += *Speed / 400.f * *dx;
        }
        DrawText(str, 10, 40, 20, BLACK);
    // ==========================done printing=============================
    EndDrawing();
}

inline m256 mm256_cmp_ps(m256 a, float b) {
    m256 mask = {.Intarr = 0};
    for (int i = 0; i < 8; i++)
        mask.Intarr[i] = ((a.Farr[i] <= b));
    return mask;
}

inline int mm256_movemask_epi32(m256 a) {
    int mask = 0;
    for (int i = 0; i < 8; i++)
        mask |= (a.Intarr[i] << i);
    return mask;
}

inline m256 mm256_add_ps(m256 a, m256 b) {
    m256 res = {.Farr = 0};
    for (int i = 0; i < 8; i++) res.Farr[i] = a.Farr[i] + b.Farr[i];
    return res;
}

inline m256 mm256_add_epi32(m256 a, m256 b) {
    m256 res = {.Intarr = 0};
    for (int i = 0; i < 8; i++) res.Intarr[i] = a.Intarr[i] + b.Intarr[i];
    return res;
}

inline m256 mm256_sub_epi32(m256 a, m256 b) {
    m256 res = {.Intarr = 0};
    for (int i = 0; i < 8; i++) res.Intarr[i] = a.Intarr[i] - b.Intarr[i];
    return res;
}

inline m256 mm256_sub_ps(m256 a, m256 b) {
    m256 res = {.Farr = 0};
    for (int i = 0; i < 8; i++) res.Farr[i] = a.Farr[i] - b.Farr[i];
    return res;
}

inline m256 mm256_mul_ps(m256 a, m256 b) {
    m256 res = {.Farr = 0};
    for (int i = 0; i < 8; i++) res.Farr[i] = a.Farr[i] * b.Farr[i];
    return res;
}

inline m256 mm256_set1_ps(float a) {
    m256 res = {.Farr = 0};
    for (int i = 0; i < 8; i++) res.Farr[i] = a;
    return res;
}

inline m256 mm256_set1_epi32(int a) {
    m256 res = {.Intarr = 0};
    for (int i = 0; i < 8; i++) res.Intarr[i] = a;
    return res;
}

inline m256 mm256_set_ps(float e7, float e6, float e5, float e4, float e3, float e2,
                  float e1, float e0) {
    m256 res = {.Farr = {e0, e1, e2, e3, e4, e5, e6, e7}};
    return res;
}

inline m256 mm256_div_ps(m256 a, m256 b) {
    m256 res = {.Farr = 0};
    for (int i = 0; i < 8; i++) res.Farr[i] = a.Farr[i] / b.Farr[i];
    return res;
}