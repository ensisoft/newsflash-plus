@echo off

REM
REM use this script to run the python packaged with Newflash as a standalone python
REM

cd Python-2.5.1
set PATH=%PATH%;%CD%\..
call python.exe