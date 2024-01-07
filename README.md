Build inscructions:

1) git clone https://github.com/oskarGrr/DreamForge

2) Download Vcpkg and Cmake if you have not already installed them.
   https://vcpkg.io/en/getting-started https://cmake.org/download/

3) Set an environment variable called VCPKG_ROOT to where you 
   decided to install vcpkg (for some reason vcpkg doesn't automatically
   set VCPKG_ROOT on installation). If you are on windows make sure that all
   path seperators are DOUBLE backslashes in VCPKG_ROOT.

3) Download the Mono project from https://www.mono-project.com/download/stable/#download.

4) Run generateProject.bat and resources/testScripts/compileC#.bat