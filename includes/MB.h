#ifndef MB_H
#define MB_H

#include <stdlib.h>

#define FILE_DIR "MeasurementsNew"
#define DISTR_STR FILE_DIR "/" PROGRAM_TYPE ".txt"

typedef enum {ERR_PTR_NULL = 0} MBErr_t;

const size_t NUM_OF_ITERATIONS       = 100;
const size_t CACHE_WARMUP_ITERATIONS = 10;
const size_t MEASUREMENTS_PER_FRAME  = 10;

const float Y_START         = -0.55f;
const float X_START         = -1.7f;
const float dx_START        = 0.001375f;
const int Speed_START       = 8;

const int MAX_RADIUS        = 100;
const int MAX_ITERATIONS    = 255;
const int MAX_DISTANCE      = 100;
const int WindowWidth       = 800;
const int WindowHeight      = 800;
const size_t MAX_STR_SIZE   = 300;

#endif // MB_H