@echo off
for %%c in (clang++) do (
    for %%o in (O3) do (
        for %%i in (IOMB) do (
            for %%d in (DRAW) do (
                rm bin/*.exe
                %%c src/%%i.cpp -DPROGRAM_TYPE=\"%%i_%%o_%%c_%%d\" -D%%d -%%o -mavx2 -fno-rtti -fno-exceptions -ffast-math -march=native            ^
                                                -I. -IC:/raylib/raylib/src -IC:/raylib/raylib/src/external  ^
                                                -L. -LC:/raylib/raylib/src -LC:/raylib/raylib/src/external  ^
                                                -lraylib -lopengl32 -lgdi32 -lwinmm                         ^
                                                -Wall -D_DEFAULT_SOURCE -o bin/%%i.exe
                "bin/%%i.exe"
            )
        )
    )
)
python3 scripts/MakeCsv.py
python3 scripts/MakeGraphs.py
python3 scripts/CalcAnalog.py