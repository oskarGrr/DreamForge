::If you have not installed the Mono project yet, then do that before you run this script.

@echo off
echo double check this batch file to make sure the path variable is being set to your Mono install location if it doesn't work
set path=%path%;"C:\Program Files\Mono\bin"

::For now just compile all scripts into one test library.
call csc.bat *.cs /target:library /out:testDLL.dll
pause