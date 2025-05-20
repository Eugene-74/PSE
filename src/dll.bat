cd build
C:\mingw-bundledlls\mingw-bundledlls simulation.exe > dependencies.txt

:copy_dependencies
for /f "tokens=*" %%i in ('type dependencies.txt') do (
    xcopy /Y "%%i" .
    C:\mingw-bundledlls\mingw-bundledlls %%i > temp_dependencies.txt
    for /f "tokens=*" %%j in (temp_dependencies.txt) do (
        if not exist "%%j" (
            xcopy /Y "%%j" .
        )
    )
)   
del dependencies.txt
del temp_dependencies.txt



C:\mingw-bundledlls\mingw-bundledlls racine.exe > dependencies.txt

:copy_dependencies
for /f "tokens=*" %%i in ('type dependencies.txt') do (
    xcopy /Y "%%i" .
    C:\mingw-bundledlls\mingw-bundledlls %%i > temp_dependencies.txt
    for /f "tokens=*" %%j in (temp_dependencies.txt) do (
        if not exist "%%j" (
            xcopy /Y "%%j" .
        )
    )
)   
del dependencies.txt
del temp_dependencies.txt

cd ..