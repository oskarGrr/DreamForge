::install mono before trying to call csc
@echo off
echo double check this batch file to make sure the path variable is being set to your Mono install location if it doesn't work
set path=%path%;"C:\Program Files\Mono\bin"
call csc.bat *.cs /target:library /out:testDLL.dll
pause