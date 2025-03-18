
mkdir build
cd build
if exist OpenGLTextRendering.exe (
    del OpenGLTextRendering.exe
)
cmake -G "MinGW Makefiles" ..
mingw32-make

if exist OpenGLTextRendering.exe (
    OpenGLTextRendering.exe
) else (
    echo "Executable not found."
)
cd ..

