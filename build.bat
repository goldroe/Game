@echo off 
SET INCLUDES=-I ..\ext -I ..\ext\glad\include -I ..\ext\GLFW\include
SET SRC=..\src\win32_game.cpp ..\ext\glad\src\glad.c
SET WARNING_FLAGS=-W4 -WX -wd4100 -wd4101 -wd4189 -wd4996 -wd4530 -wd4201 -wd4505 -wd4098 -wd4700
SET COMPILER_FLAGS=-nologo -FC -MDd -Zi %WARNING_FLAGS% %INCLUDES% -Fe:Game.exe
SET LINKER_FLAGS=-debug -SUBSYSTEM:CONSOLE -IGNORE:4098 -LIBPATH:ext\GLFW ..\ext\GLFW\glfw3.lib opengl32.lib user32.lib gdi32.lib winmm.lib shell32.lib

IF NOT EXIST .build MKDIR .build
PUSHD .build
CL %COMPILER_FLAGS% %SRC% -DDEVELOPER -link %LINKER_FLAGS%
COPY *.exe ..

POPD
