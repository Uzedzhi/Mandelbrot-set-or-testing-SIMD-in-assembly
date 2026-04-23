set "myArg1=%~1"
if "%myArg1%"=="" set "myArg1=IOMB"

set "myArg2=%~2"
if "%myArg2%"=="" set "myArg2=DRAW"

set "myArg3=%~3"
if "%myArg3%"=="" set "myArg3=MEASURE"

clang++ %myArg1%.cpp -D%myarg2% -D%myarg3% -O3 -mavx2 -march=native -o %myArg1%.exe ^
    -I. -IC:/raylib/raylib/src -IC:/raylib/raylib/src/external ^
    -L. -LC:/raylib/raylib/src -LC:/raylib/raylib/src/external ^
    -lraylib -lopengl32 -lgdi32 -lwinmm^
    -Wall -D_DEFAULT_SOURCE
%myarg1%.exe