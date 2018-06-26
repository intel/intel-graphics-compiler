@echo off

IF "%1"=="/h" goto :HELP
IF "%1"=="/H" goto :HELP
IF "%1"=="/?" goto :HELP
IF "%1"=="-h" goto :HELP
IF "%1"=="--help" goto :HELP

set VS_VERSION=""
IF "%1"=="vs2013" (SET VS_VERSION=vs2013)
IF "%1"=="vs2015" (SET VS_VERSION=vs2015)
IF "%1"=="vs2017" (SET VS_VERSION=vs2017)
IF %VS_VERSION%=="" goto :HELP

:VS_DEFAULT
SET X86_OR_X64=""
IF "%2"=="" (SET X86_OR_X64=32) & (goto SET_IGA_BUILD_TYPE)
IF "%2"=="32" (SET X86_OR_X64=32)
IF "%2"=="64" (SET X86_OR_X64=64)
IF %X86_OR_X64%=="" goto :HELP

:SET_X86_OR_X64
SET BUILD_DIR=""
IF NOT "%3"=="" (SET BUILD_DIR=%3) & (goto INVOKE_CMAKE)
SET BUILD_DIR=builds\%VS_VERSION%.%X86_OR_X64%
:INVOKE_CMAKE
echo [generate_solution] Going to attempt to build VS solution/project files for %VS_VERSION% for %X86_OR_X64% in %BUILD_DIR%

SET CMAKE_ARCH=
IF %VS_VERSION%==vs2010 (SET CMAKE_MODE_VS=10)
IF %VS_VERSION%==vs2012 (SET CMAKE_MODE_VS=11)
IF %VS_VERSION%==vs2013 (SET CMAKE_MODE_VS=12)
IF %VS_VERSION%==vs2015 (SET CMAKE_MODE_VS=14)
IF %VS_VERSION%==vs2017 (SET CMAKE_MODE_VS=15)
IF %X86_OR_X64%==64 (SET CMAKE_ARCH= Win64)

echo [generate_solution] BUILDING in %BUILD_DIR%
IF "%BUILD_DIR%" == "." (
    ECHO [generate_solution] ERROR: Cannot clobber .^^!  Choose a directory that you're willing to have nuked first.
    GOTO :FAIL
)

IF EXIST "%BUILD_DIR%" (
    echo [generate_solution] clobbering existing %BUILD_DIR%
    rd /s /q %BUILD_DIR%
    IF NOT %ERRORLEVEL%==0 (echo ) & (goto :FAIL)
)
md "%BUILD_DIR%"
pushd "%BUILD_DIR%"

IF EXIST ..\..\..\cmake\cmake-3.8.0-rc1-win32-x86\bin\cmake.exe (
    set CMAKE_EXE=..\..\..\cmake\cmake-3.8.0-rc1-win32-x86\bin\cmake.exe
) ELSE (
    IF EXIST ..\..\..\cmake\cmake-3.2.1-win32-x86\bin\cmake.exe (
        set CMAKE_EXE=..\..\..\cmake\cmake-3.2.1-win32-x86\bin\cmake.exe
    ) ELSE (
        IF EXIST ..\..\..\..\Tools\cmake\cmake-3.2.1-win32-x86\bin\cmake.exe (
            set CMAKE_EXE=..\..\..\..\Tools\cmake\cmake-3.2.1-win32-x86\bin\cmake.exe
        ) ELSE (
            set CMAKE_EXE=cmake.exe
            REM try and use cmake from %PATH%
            echo [generate_solution] WARNING: repo cmake not in path; get //gfx_Development/DEV/DEV_IGC/Tools/cmake/cmake-3.2.1-win32-x86/bin/cmake.exe
            cmake.exe --version>NUL
            IF NOT %ERRORLEVEL%==0 (echo [generate_solution] cmake not found in path) & (popd) & (goto :EOF)
            echo [generate_solution] WARNING: using cmake from %%PATH%%
        )
    )
)

@echo on
%CMAKE_EXE% -G "Visual Studio %CMAKE_MODE_VS%%CMAKE_ARCH%" ..\..
@echo off
popd

:SUCCESS
EXIT /b 0

:HELP
@echo usage: generate_solution.bat [vs2013^|vs2015^|vs2017] [32^|64] [BUILD_DIR]
@echo WHERE
@echo   [vs2013^|vs2015^|vs2017]      corresponds to the version of Visual Studio to build with
@echo   [32^|64]                     corresponds to a 32-bit or 64-bit project files
@echo   [BUILD_DIR]                 is an optional directory to store the solution files
@echo                               *** WARNING: this dirctory gets clobbered first! ***
EXIT /b 0

:FAIL
ECHO [generate_solution] SCRIPT FAILED
EXIT /b 1
