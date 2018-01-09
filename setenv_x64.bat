 @echo off
set GRA_ROOT=E:\projects
set OPENSSL_ROOT=%GRA_ROOT%\OpenSSL.x64
set OPENSSL_ROOT_DIR=%OPENSSL_ROOT%
set OPENSSL_INCLUDE_DIR=%OPENSSL_ROOT%\include
set BOOST_ROOT=%GRA_ROOT%\boost_1_60_0

set PATH=%GRA_ROOT%\cmake\bin;%BOOST_ROOT%\lib;%PATH%

echo Setting up VS2013 environment...
call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64