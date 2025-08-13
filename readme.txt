2025-8-12 I couldn't compile sacd_extract.exe under the MSYS2 toolchain. After repeated testing and modifying three files, the compilation was successful!

modified 3 files:

acd-ripper-0.3.9.3\tools\sacd_extract\CMakeLists.txt
sacd-ripper-0.3.9.3\libs\libsacd\scarletbook.h
sacd-ripper-0.3.9.3\libs\libsacd\scarletbook_read.c

I haven't thoroughly tested whether there are any bugs! 
Please note not to use it in critical situations, as further verification is still needed.

You should install serveral package:

pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake-gui
pacman -S mingw-w64-x86_64-gdb
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-libxml2

Build sacd_extract with following command (current path: ...\sacd-ripper-0.3.9.3.117.X\tools\sacd_extract)

mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make