#define TRACY_ENABLE
#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <immintrin.h>
#include <chrono>
#include <assert.h>

#include "../includes/MB.h"

const size_t D_PER_REGEX    = 4;

#ifndef PROGRAM_TYPE
#define PROGRAM_TYPE "DIOMB"
#endif

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

void PrintColorsToTexture(size_t *ColorIndex, int FinalColorValues[], Color MandelbrotPixels[]) {
    for (size_t i = 0; i < D_PER_REGEX; i++) {
        unsigned char ColorValue = (unsigned char) FinalColorValues[i];
        MandelbrotPixels[(*ColorIndex)++] = (Color){ColorValue,
                                                    ColorValue,
                                                    static_cast<unsigned char> (ColorValue%2*128),
                                                    255};
    }
}

void DrawTextureToTheScreen(Texture MandelbrotTexture, double *dx, double *y0_OUTLoop, double *x0_OUTLoop, int *Speed) {
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
                                    double CordsDelta = *Speed * *dx;
                                    *y0_OUTLoop  += CordsDelta;
                                    *x0_OUTLoop  += CordsDelta;
                                    *dx          -= *Speed / 400.f * *dx;
        }
        if (IsKeyDown(KEY_X)) {
                                    double CordsDelta = *Speed * *dx;
                                    *y0_OUTLoop  -= CordsDelta;
                                    *x0_OUTLoop  -= CordsDelta;
                                    *dx          += *Speed / 400.f * *dx;
        }
        DrawText(str, 10, 40, 20, BLACK);
    // ==========================done printing=============================
    EndDrawing();
}

#define CALCULATE_MB_ITERATION(X, Y, X0, Y0, N) {                                                               \
    __m256d X2 = _mm256_mul_pd(X, X);                                                                           \
    __m256d Y2 = _mm256_mul_pd(Y, Y);                                                                           \
    __m256d XY = _mm256_mul_pd(X, Y);                                                                           \
                                                                                                                \
    __m256d doubleVecMask = _mm256_cmp_pd(_mm256_add_pd(X2, Y2), MAX_RADIUS_V, _CMP_LT_OS);                     \
    int IntMask = _mm256_movemask_pd(doubleVecMask);                                                            \
    if (!(IntMask))                                                                                             \
        break;                                                                                                  \
                                                                                                                \
    X = _mm256_add_pd(_mm256_sub_pd(X2, Y2), X0);                                                               \
    Y = _mm256_add_pd(_mm256_add_pd(XY, XY), Y0);                                                               \
                                                                                                                \
    __m256d decr = _mm256_and_pd(doubleVecMask, ONE_D);                                                         \
    N = _mm256_sub_pd(N, decr);                                                                                 \
}                                                                                                               \

int main(void) {
    #ifndef NDRAW
        InitWindow(WindowWidth, WindowHeight, "Mandelbrot");

        Color *MandelbrotPixels = (Color *) calloc(WindowHeight * WindowWidth, sizeof(Color));
        RenderTexture2D MandelbrotTexture = LoadRenderTexture(800, 800);
    #endif
    
    double dx           = dx_START;
    double y0_OUTLoop   = Y_START;
    double x0_OUTLoop   = X_START;
    int    Speed        = Speed_START;
    int    control_sum  = 0;
    bool BreakFlag      = false;

    #ifndef NMEASURE
        size_t FramesElapsed = 0;
        FILE *fp = fopen(DISTR_STR, "w");
        assert(fp);
    #endif

    __m256d MAX_RADIUS_V     = _mm256_set1_pd(MAX_RADIUS);
    __m256d MAX_ITER_ARR     = _mm256_set1_pd(MAX_ITERATIONS);
    __m256d ONE_D            = _mm256_set1_pd(1.0);
    __m256d INDICES          = _mm256_set_pd(3.0, 2.0, 1.0, 0.0);
    #ifndef NDRAW
        while (!WindowShouldClose() && !BreakFlag) {
    #else
        while (true && !BreakFlag) {
    #endif
        size_t ColorIndex   = 0;
    
        __m256d DX_D     = _mm256_set1_pd(dx);
        __m256d DX_STEPS = _mm256_mul_pd(DX_D, INDICES);
        
        for (int inty = 0; inty < WindowHeight; inty++) { 
            double  y0  = y0_OUTLoop + (double)inty * dx;
            __m256d Y0  = _mm256_set1_pd(y0);
            
            for (int intx = 0; intx < WindowWidth; intx += D_PER_REGEX) {
                __m256d N = MAX_ITER_ARR;
                #ifndef NMEASURE
                    for (size_t FrameCnt = 0; FrameCnt < MEASUREMENTS_PER_FRAME && !BreakFlag; FrameCnt++) {
                    INIT_START_COUNTER(start);
                #endif

                double  x0 = x0_OUTLoop + (double)intx * dx;
                
                __m256d X  = _mm256_add_pd(_mm256_set1_pd(x0), DX_STEPS);
                __m256d Y  = Y0;
                __m256d X0 = X;
                __m256d N  = MAX_ITER_ARR;
                
                for (int n = MAX_ITERATIONS; n > 0; n--) {
                    CALCULATE_MB_ITERATION(X, Y, X0, Y0, N)
                }

                #ifndef NMEASURE
                    SAVE_SECONDS_TO_FILE(fp, start, FramesElapsed);
                    }
                #endif

                __m128i final_n = _mm256_cvtpd_epi32(N); 
                int FinalColorValues[D_PER_REGEX] = {};
                _mm_store_si128((__m128i*)FinalColorValues, final_n);
                #ifndef NDRAW
                    PrintColorsToTexture(&ColorIndex, FinalColorValues, MandelbrotPixels);
                #else
                    control_sum += FinalColorValues[1];
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