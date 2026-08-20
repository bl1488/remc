@echo off
setlocal enabledelayedexpansion

if "%BUILD_TYPE%"=="" set BUILD_TYPE=DEBUG
if "%CC%"=="" set CC=clang++
set BUILD_DIR=build

if "%1"=="" goto help
goto %1 2>nul
if errorlevel 1 (
    echo Unknown target: %1
    goto help
)

:help
echo "Usage: %0 <target>"
echo Targets:
echo   configure
echo   build-server
echo   build-client
echo   build-tests
echo   build-gui
echo   run-server
echo   run-client
echo   run-tests
echo   run-gui
echo   clean
exit /b 1

:configure
if not exist "%BUILD_DIR%" (
    cmake -G Ninja ^
          -DCMAKE_CXX_COMPILER=%CC% ^
          -DCMAKE_C_COMPILER=clang ^
          -DREMC_BUILD_TYPE=%BUILD_TYPE% ^
          -DBUILD_CRYPTO_MODULE=ON ^
          -DBUILD_NET_MODULE=ON ^
          -DBUILD_GUI_MODULE=ON ^
          -DBUILD_TEST_MODULE=ON ^
          -DBUILD_SERVER=ON ^
          -DBUILD_CLIENT=ON ^
          -B "%BUILD_DIR%"
)
exit /b 0

:build-server
call :configure
cmake --build "%BUILD_DIR%" --target remc-server
exit /b 0

:build-client
call :configure
cmake --build "%BUILD_DIR%" --target remc-client
exit /b 0

:build-tests
call :configure
cmake --build "%BUILD_DIR%" --target tests-main
exit /b 0

:build-gui
call :configure
cmake --build "%BUILD_DIR%" --target gui-main
exit /b 0

:run-server
call :build-server
call :print_exec SERVER
"%BUILD_DIR%\remc-server.exe"
exit /b 0

:run-client
call :build-client
call :print_exec CLIENT
"%BUILD_DIR%\remc-client.exe"
exit /b 0

:run-tests
call :build-tests
call :print_exec TESTS
"%BUILD_DIR%\src\tests\tests-main.exe"
exit /b 0

:run-gui
call :build-gui
call :print_exec GUI
"%BUILD_DIR%\src\tests\gui-main.exe"
exit /b 0

:clean
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
)
exit /b 0

:print_exec
set "name=%~1"
powershell -command "Write-Host '============< exec !name!::%BUILD_TYPE% >============' -ForegroundColor Green"
exit /b 0