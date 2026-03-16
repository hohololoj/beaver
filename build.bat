::Переменные
set "ROOT=%~dp0"
set "%ROOT:~0,-1%"
echo %ROOT%

if exist build ( rmdir /s /q %ROOT%\build )
mkdir build

::gcc compile
cd bin
call build.bat
cd ..

::переместить exe
move %ROOT%\gui\beaver.exe %ROOT%\build\

::neu compile
cd gui
call neu build --embed-resources

::переместить exe
move %ROOT%\gui\dist\beaver\beaver-win_x64.exe %ROOT%\build\

::удалить dist папку
rmdir /s /q %ROOT%\gui\dist

::переименовать gui приложение
cd ..
cd build
rename beaver-win_x64.exe beaver-app.exe
cd ..

::подменить ресурсы
resourcehacker  -open %ROOT%\build\beaver-app.exe ^
				-save %ROOT%\build\beaver-app.exe ^
				 -action delete ^
				-mask VERSIONINFO,,

resourcehacker  -open %ROOT%\build\beaver-app.exe ^
				-save %ROOT%\build\beaver-app.exe ^
				-action addoverwrite ^
				-mask VERSIONINFO,, ^
				-resource %ROOT%\build_resources\beaver-resources.res
				
resourcehacker  -open %ROOT%\build\beaver-app.exe ^
				-save %ROOT%\build\beaver-app.exe ^
				-action addoverwrite ^
				-mask ICONGROUP,, ^
				-resource %ROOT%\build_resources\beaver-resources.res

resourcehacker -open %ROOT%\build\beaver-app.exe ^
               -save %ROOT%\build\beaver-app.exe ^
               -action addoverwrite ^
               -mask MANIFEST,1, ^
               -resource %ROOT%\build_resources\beaver.manifest

move %ROOT%\build %ROOT%\bin\setup\

iscc.exe /O"%ROOT%\bin\setup\build" "%ROOT%\bin\setup\setup.iss"

move %ROOT%\bin\setup\build %ROOT%\