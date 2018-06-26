@echo off

@REM ensure we are in a VS command window
cl.exe >NUL
IF NOT %ERRORLEVEL%==0 (echo [build_all] MUST IN RUN IN VISUAL STUDIO COMMAND PROMPT WINDOW; devenv.exe not found) & (popd) & (goto :EOF)

SET VS_VERSION=vs2017
IF "%1"=="vs2012" (SET VS_VERSION=vs2012)
IF "%1"=="vs2013" (SET VS_VERSION=vs2013)
IF "%1"=="vs2015" (SET VS_VERSION=vs2015)
IF "%1"=="vs2017" (SET VS_VERSION=vs2017)
IF %VS_VERSION%=="" goto HELP

rmdir builds\%VS_VERSION%.32 /s /q
rmdir builds\%VS_VERSION%.64 /s /q
CALL scripts\generate_solution.bat %VS_VERSION% 32
CALL scripts\generate_solution.bat %VS_VERSION% 64
devenv .\builds\%VS_VERSION%.32\IGA.sln /rebuild Release /Project ALL_BUILD
devenv .\builds\%VS_VERSION%.32\IGA.sln /rebuild Debug /Project ALL_BUILD
devenv .\builds\%VS_VERSION%.64\IGA.sln /rebuild Release /Project ALL_BUILD
devenv .\builds\%VS_VERSION%.64\IGA.sln /rebuild Debug /Project ALL_BUILD

GOTO :EOF
:HELP
echo Usage:
echo build_all.bat [vs_version]
echo
echo vs_version=vs2013/vs2015/vs2017 (default: vs2017)
GOTO :EOF
