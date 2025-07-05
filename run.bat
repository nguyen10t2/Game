@echo off

REM Run the C++ program
g++ caro.cpp -o caro.exe
if %errorlevel% neq 0 (
    echo Compilation failed.
    exit /b %errorlevel%
)
start cmd /k caro.exe
if %errorlevel% neq 0 (
    echo Execution failed.
    exit /b %errorlevel%
)
echo Program executed successfully.
pause
exit /b 0