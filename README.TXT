folk from: https://github.com/setmind/sacd-ripper	
The original code could not compile successfully under Windows 10 + CLion 2025.2. After the following operations, the build was successful!

1. Modify the beginning of CMakeLists.txt: change cmake_minimum_required(VERSION 2.6) from 2.6 to 3.5
2. Download the libiconv library from the internet and copy it to CLion's mingw directory.
	(https://sourceforge.net/projects/mingw-w64/files/External%20binary%20packages%20%28Win64%20hosted%29/libiconv/libiconv-1.13.1-win64-20100707.zip/download?use_mirror=master&use_mirror=master&r)
3. Build. At this point, you will get the error: 
	collect2.exe: error: ld returned 1 exit status ninja: build stopped: subcommand failed.
4. The problem is due to duplicate variable name definitions. Simply add the following line to CMakeLists.txt: 
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition")
5. Rebuild successfully!