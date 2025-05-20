
mkdir build
cd build
if exist simulation.exe (
    del simulation.exe
)
cmake -G "MinGW Makefiles" ..
mingw32-make

if exist simulation.exe (
    simulation.exe
) else (
    echo "Executable not found."
)
cd ..

