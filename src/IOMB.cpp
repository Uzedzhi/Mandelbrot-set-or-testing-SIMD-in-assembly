#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <immintrin.h>
#include <chrono>
#include <assert.h>

#include "../includes/MB.h"

#ifndef PROGRAM_TYPE
#define PROGRAM_TYPE "IOMB"
#endif

const size_t F_PER_REGEX = 8;
__m256i ONE_V           = _mm256_set1_epi32(1);
__m256  MAX_RADIUS_V    = _mm256_set1_ps(MAX_RADIUS);

#define INIT_START_COUNTER(name) \
    auto name = std::chrono::steady_clock::now();

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
}

inline __attribute__((always_inline)) int CalculateMBIteration(__m256 *X, __m256 *Y, __m256 X0, __m256 Y0, __m256i *N) {
    __m256 X2 = _mm256_mul_ps(*X, *X);
    __m256 Y2 = _mm256_mul_ps(*Y, *Y);
    __m256 XY = _mm256_mul_ps(*X, *Y);
    __m256 FloatVecMask = _mm256_cmp_ps(_mm256_add_ps(X2, Y2), MAX_RADIUS_V, _CMP_LT_OS);
    int IntMask = _mm256_movemask_ps(FloatVecMask);
    if (!IntMask)
        return 1;
    *X = _mm256_add_ps(_mm256_sub_ps(X2, Y2), X0);
    *Y = _mm256_add_ps(_mm256_add_ps(XY, XY), Y0);
    *N = _mm256_sub_epi32(*N, _mm256_and_si256(_mm256_castps_si256(FloatVecMask), ONE_V));
    return 0;
}

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

int main(void) {
    #ifndef NDRAW
        InitWindow(WindowWidth, WindowHeight, "Mandelbrot");

        Color *MandelbrotPixels = (Color *) calloc(WindowHeight * WindowWidth, sizeof(Color));
        RenderTexture2D MandelbrotTexture = LoadRenderTexture(800, 800);
    #endif

    float dx         = dx_START;
    float y0_OUTLoop = Y_START;
    float x0_OUTLoop = X_START;
    int   Speed      = Speed_START;
    bool BreakFlag   = false;

    #ifndef NMEASURE
        size_t FramesElapsed = 0;
        FILE *fp = fopen(DISTR_STR, "w");
        assert(fp);
    #endif

    __m256i ZERO_V          = _mm256_set1_epi32(0);
    __m256i MAX_ITER_ARR    = _mm256_set1_epi32(MAX_ITERATIONS);
    __m256  Indexes         = _mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0);
    int control_sum = 0;

    #ifndef NDRAW
        while (!WindowShouldClose() && !BreakFlag) {
    #else
        while (true && !BreakFlag) {
    #endif
        size_t ColorIndex = 0;
        for (int inty = 0; inty < WindowHeight; inty++) { 
            float y0 = y0_OUTLoop + inty * dx;
            __m256 Y0 = _mm256_set1_ps(y0);
            __m256 DX = _mm256_set1_ps(dx);
            
            for (int intx = 0; intx < WindowWidth; intx += F_PER_REGEX) {
                __m256i N = ZERO_V;
                #ifndef NMEASURE
                    for (size_t FrameCnt = 0; FrameCnt < MEASUREMENTS_PER_FRAME && !BreakFlag; FrameCnt++) {
                        INIT_START_COUNTER(start);
                #endif

                N   = MAX_ITER_ARR;
                
                float x0   = x0_OUTLoop + intx * dx;
                __m256  X  = _mm256_add_ps(_mm256_set1_ps(x0), _mm256_mul_ps(Indexes, DX));
                __m256  Y  = Y0;
                __m256  X0 = X;

                for (int n = MAX_ITERATIONS; n > 0; n--) {
                    if (CalculateMBIteration(&X, &Y, X0, Y0, &N))
                        break;
                }

                #ifndef NMEASURE
                    SAVE_SECONDS_TO_FILE(fp, start, FramesElapsed);
                    }
                #endif

                int FinalColorValues[F_PER_REGEX] = {};
                _mm256_store_si256((__m256i *) FinalColorValues, N);

                #ifndef NDRAW
                    PrintColorsToTexture(&ColorIndex, FinalColorValues, MandelbrotPixels);
                #else
                    control_sum += FinalColorValues[0];
                #endif
            }
        }
        #ifndef NMEASURE
            FramesElapsed++;
        #endif

        // =====================Calculated All The Pixels======================
        #ifndef NDRAW
            UpdateTexture(MandelbrotTexture.texture, (void *) MandelbrotPixels);
            DrawTextureToTheScreen(MandelbrotTexture.texture, &dx, &y0_OUTLoop, &x0_OUTLoop, &Speed);
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
}