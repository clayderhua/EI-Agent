@echo off
setlocal enableextensions
REM =================================================
REM Parameters:
REM  %1: [IN]  version
REM Return:
REM  Revision
REM =================================================
SET TOOLS_ROOT=tools
SET INCLUDE_ROOT=..\Include
SET SRC_PATH=%CD%\..

set version=3.0.0.0
set MAIN_VERSION=3
set SUB_VERSION=0
set BUILD_VERSION=0
set REVISION=0
set argC=0
for %%x in (%*) do Set /A argC+=1

if %argC% LSS  1 goto result

set version=%1
if defined version goto nextVar
goto result

:nextVar
	for /F "tokens=1-4 delims=." %%a in ("%version%") do (
		set MAIN_VERSION=%%a
		set SUB_VERSION=%%b
		set BUILD_VERSION=%%c
		set REVISION=%%d
REM		if [%%d] EQU [] goto getsvnrevision
		if [%%d] EQU [] set REVISION=0
		goto result
	)	
:getsvnrevision
	set cmd=git describe --long --tags --always
	FOR /F %%i IN ('%cmd%') DO SET REVISION=%%i

	REM Remove M/S/P characters at last
	set REVISION=%REVISION:M=%
	set REVISION=%REVISION:S=%
	set REVISION=%REVISION:P=%

	REM Parse a mixed-revision, Ex: 4123:4168
	for /f "tokens=2-3 delims=- " %%a in ("%REVISION%") do (
		SET REVISION=%%a
		IF NOT "%%b"=="" SET REVISION=%%b
	)
	set REVISION=%REVISION:g=%
:result
echo %REVISION%
SET version=%MAIN_VERSION%.%SUB_VERSION%.%BUILD_VERSION%.%REVISION%

SET INC_SVN_VER=%INCLUDE_ROOT%\svnversion.h
SET MAKE_SO_VER=%SRC_PATH%\VERSION

attrib -r "%INC_SVN_VER%"
echo #ifndef __SVN_REVISION_H__ > "%INC_SVN_VER%"
echo #define __SVN_REVISION_H__ >> "%INC_SVN_VER%"
echo #define MAIN_VERSION %MAIN_VERSION% >> "%INC_SVN_VER%"
echo #define SUB_VERSION %SUB_VERSION% >> "%INC_SVN_VER%"
echo #define BUILD_VERSION %BUILD_VERSION% >> "%INC_SVN_VER%"
echo #define SVN_REVISION 0x%REVISION% >> "%INC_SVN_VER%"
echo #endif /* __SVN_REVISION_H__ */ >> "%INC_SVN_VER%"

attrib -r "%MAKE_SO_VER%"
echo %version% > "%MAKE_SO_VER%"

COPY /Y "%MAKE_SO_VER%" "%SRC_PATH%\build\WISEAgent\"

SET XML_AGENT_VER=%SRC_PATH%\build\config\agent_config.xml
type "%XML_AGENT_VER%"|%TOOLS_ROOT%\repl "(<SWVersion>).*(</SWVersion>)" "$1%version%$2" >"%XML_AGENT_VER%.new"

echo %version%
