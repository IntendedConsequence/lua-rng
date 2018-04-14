@REM Batch file to build the library from the Visual Studio command shell
@REM (or the Windows SDK command shell)

@setlocal

@REM *******************************************************************
@REM TODO: Customize these lines for your library and Lua file locations
@REM *******************************************************************
@set LIBNAME=rng
@set LUAINC="C:\LuaAll\include\lua\5.1"
@set LUALIB="C:\LuaAll\lib\lua51.lib"



@REM **************************************************
@REM Generally no need to customize anything below here
@REM **************************************************

@set MYCOMPILE=cl /nologo /D "NDEBUG" /MD /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE
@set MYLINK=link /nologo
@set MYMT=mt /nologo


@echo.
@echo  *** CLEAN STAGE ***
@echo.
if exist *.obj ( del *.obj )
if exist *.dll.manifest ( del %LIBNAME%.dll.manifest )
if exist *.dll ( del %LIBNAME%.dll )
@echo.
@echo  *** COMPILE STAGE ***
@echo.
%MYCOMPILE%  /I%LUAINC% *.c
@echo.
@echo  *** LINK STAGE ***
@echo.
%MYLINK% /DLL /out:%LIBNAME%.dll *.obj %LUALIB%
@echo.
@echo  *** MANIFEST STAGE ***
@echo.
if exist %LIBNAME%.dll.manifest (
  %MYMT% -manifest %LIBNAME%.dll.manifest -outputresource:%LIBNAME%.dll;2
)
@echo.

