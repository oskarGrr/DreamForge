
## Build inscructions:

1. git clone https://github.com/oskarGrr/DreamForge

2. Install Vcpkg and Cmake if you do not have them installed already.
   Tell the cmake installer to add cmake to the system path. Otherwise do it yourself 
   so that the generateProjects script knows where to look for cmake.exe. 
   [download vcpkg](https://vcpkg.io/en/getting-started)
   [download cmake](https://cmake.org/download/)

3. Set an environment variable called VCPKG_ROOT to where you 
   decided to install vcpkg (for some reason vcpkg doesn't automatically
   set VCPKG_ROOT on installation). If you are on windows make sure that all
   path seperators are DOUBLE backslashes in VCPKG_ROOT.

3. Install Mono [download Mono](https://www.mono-project.com/download/stable/#download).

4. Run generateProject.bat and resources/testScripts/compileC#.bat

