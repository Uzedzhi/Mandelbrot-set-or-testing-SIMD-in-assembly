#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <chrono>

#include "../includes/MB.h"

#ifndef PROGRAM_TYPE
#define PROGRAM_TYPE "NOMB"
#endif

#define INIT_START_COUNTER(name) \
    auto name = std::chrono::steady_clock::now();

#define SAVE_SECONDS_TO_FILE(fp, StartName, FramesElapsed) {\
    auto end    = std::chrono::steady_clock::now();\
    auto delta  = std::chrono::duration_cast<std::chrono::nanoseconds>(end - StartName).count();\
    size_t TimePerFrame = delta;\
    \
    if (FramesElapsed > NUM_OF_ITERATIONS + CACHE_WARMUP_ITERATIONS) {\
        BreakFlag = true;\
        break;\
    }\
    if (FramesElapsed >= CACHE_WARMUP_ITERATIONS)\
        fprintf(fp, "%zu, %zu\n", FramesElapsed - CACHE_WARMUP_ITERATIONS, TimePerFrame);\
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

int main(int argc, char *argv[]) {
    #ifndef NDRAW
        InitWindow(WindowWidth, WindowHeight, "Mandelbrot");

        RenderTexture2D MandelbrotTexture = LoadRenderTexture(800, 800);
        SetTargetFPS(0);
    #endif
    
    Color *MandelbrotPixels = (Color *) calloc(WindowHeight * WindowWidth, sizeof(Color));
    float dx            = dx_START;
    float y0_OUTLoop    = Y_START;
    float x0_OUTLoop    = X_START;
    int   Speed         = Speed_START;
    int   control_sum   = 0;
    bool  BreakFlag     = false;

    #ifndef NMEASURE
        size_t FramesElapsed = 0;
        FILE *fp = fopen(DISTR_STR, "w");
        assert(fp);
    #endif

    #ifndef NDRAW
        while (!WindowShouldClose() && !BreakFlag) {
    #else
        while (true && !BreakFlag) {
    #endif
        size_t ColorIndex = 0;
        
        // =============Main Logic for calculating Mandelbrot Set==============
        for (int inty = 0; inty < WindowHeight; inty++) {
            float y0 = y0_OUTLoop + inty * dx;
            for (int intx = 0; intx < WindowWidth; intx++) {
                unsigned char n = MAX_ITERATIONS;
                #ifndef NMEASURE
                    for (size_t FrameCnt = 0; FrameCnt < MEASUREMENTS_PER_FRAME && !BreakFlag; FrameCnt++) {
                        INIT_START_COUNTER(start);
                #endif
                
                float x0 = x0_OUTLoop + intx * dx;
                float x  = x0;
                float y  = y0;

                n    = MAX_ITERATIONS;
                for (; n > 0; n--) {
                    float x2 = x * x;
                    float y2 = y * y;

                    float xy = x * y;
                    x  = x2 - y2 + x0;
                    y  = 2 * xy + y0;

                    if (x2 + y2 > MAX_DISTANCE)
                        break;   
                }

                #ifndef NMEASURE
                    SAVE_SECONDS_TO_FILE(fp, start, FramesElapsed);
                    }
                #endif

                MandelbrotPixels[ColorIndex++] = (Color) {n, n, static_cast<unsigned char> (n%2*128), 255};
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
        control_sum += MandelbrotPixels[100].r;
    #else
        control_sum += MandelbrotPixels[100].r;
        printf("%d", control_sum);
    #endif
    #ifndef NMEASURE
        fclose(fp);
    #endif
    return 0;
}