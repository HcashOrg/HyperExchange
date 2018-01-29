 @echo off
set GRA_ROOT=E:\blocklink_project
set OPENSSL_ROOT=%GRA_ROOT%\OpenSSL.x64
set OPENSSL_ROOT_DIR=%OPENSSL_ROOT%
set OPENSSL_INCLUDE_DIR=%OPENSSL_ROOT%\include
set BOOST_ROOT=%GRA_ROOT%\boost_1_57_0


set PATH=%GRA_ROOT%\CMake\bin;%BOOST_ROOT%\lib;%OPENSSL_ROOT%\lib;%PATH%

echo Setting up VS2013 environment...
call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64