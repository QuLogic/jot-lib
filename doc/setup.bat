@echo on

rem XXX - Setup The correct Visual Studio Version
rem  For Visual Studio 2003:
set VSYEAR=2003
rem  For Visual Studio 8 (2005):
rem  set VSYEAR=2005

rem   XXX - if VC++ is set up in D:, you will need to change C: to D: below
rem         Visual studio 8 (2005) requires different paths than Visual Studio 2003

rem  **** Visual Studio 2003 path ****
IF "%VSYEAR%"=="2003" call "C:\Program Files\Microsoft Visual Studio .Net 2003\Vc7\Bin\vcvars32.bat"

rem  ****  use this path for Visual Studio  8 (2005) instead ****
IF "%VSYEAR%"=="2005" call "C:\Program Files\Microsoft Visual Studio 8\VC\Bin\vcvars32.bat"

rem **** Visual Studio 2003 path ****
IF "%VSYEAR%"=="2003" set Path=%Path%;C:\Program Files\Microsoft Visual Studio .Net 2003\Vc7\Bin
IF "%VSYEAR%"=="2003" set cl=/I"C:\Program Files\Microsoft Visual Studio .Net 2003\Vc7\Include" /I"C:\Program Files\Microsoft Visual Studio .Net 2003\Vc7\PlatformSDK\Include"

rem  ****Visual Studio 8 (2005) path ****
IF "%VSYEAR%"=="2005" set Path=%Path%;C:\Program Files\Microsoft Visual Studio 8\VC\Bin
IF "%VSYEAR%"=="2005" set cl=/I"C:\Program Files\Microsoft Visual Studio 8\VC\Include" /I"C:\Program Files\Microsoft Visual Studio 8\VC\PlatformSDK\Include"

rem XXX: fix
rem set JOT_ROOT=C:\jot

set Path=%Path%;%JOT_ROOT%\bin\WIN32
set Path=%Path%;%JOT_ROOT%\lib\WIN32\D

set TEMP=C:\TEMP
set TMP=C:\TEMP

set UGA_ARCH=WIN32
set ARCH=WIN32
set VSVER=71

set JOT_WINDOW_WIDTH=800
set JOT_WINDOW_HEIGHT=600

rem  XXX - change drive letter if jot is not on drive C:
c:
cd %JOT_ROOT%