@echo off
setlocal

REM Save the current directory
set "PWD=%CD%"

cd .\cli

go build -o ../bin/rsp.exe


cd "%PWD%"

endlocal
