setlocal
call "F:\bts\blocklink-core\setenv_x64.bat"
cd %GRA_ROOT%
cmake-gui -G "Visual Studio 12"