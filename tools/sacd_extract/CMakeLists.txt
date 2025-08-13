# CMake build file for SACD Extract
cmake_minimum_required(VERSION 3.10)
project(sacd_extract C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)

# Windows平台特定设置
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcommon")
endif()

# 查找所需的包
find_package(LibXml2 REQUIRED)
find_package(Threads REQUIRED)

# 添加所有源文件
file(GLOB COMMON_SOURCES "../../libs/libcommon/*.c")
file(GLOB DSTDEC_SOURCES "../../libs/libdstdec/*.c")
file(GLOB ID3_SOURCES "../../libs/libid3/*.c")
file(GLOB SACD_SOURCES "../../libs/libsacd/*.c")

set(SOURCES
    main.c
    getopt.c
    ${COMMON_SOURCES}
    ${DSTDEC_SOURCES}
    ${ID3_SOURCES}
    ${SACD_SOURCES}
)

# 包含头文件目录
include_directories(
    ${LIBXML2_INCLUDE_DIR}
    ../../libs/libcommon
    ../../libs/libdstdec
    ../../libs/libid3
    ../../libs/libsacd
    ../../libs/libanergistic
)

project(sacd_extract C)

# 在 Windows 下彻底禁用自带的 getopt.c 编译，改用系统版本
if(WIN32)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/getopt.c PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# Load some macros.
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

# Macros we'll need
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(FindThreads)

# Include directory paths
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${sacd_extract_SOURCE_DIR})

include_directories("../../libs/libcommon")
include_directories("../../libs/libdstdec")
include_directories("../../libs/libid3")
include_directories("../../libs/libsacd")

# 查找LibXml2
find_package(LibXml2 REQUIRED)
message(STATUS "LibXml2 found: ${LIBXML2_LIBRARIES}")
include_directories(${LIBXML2_INCLUDE_DIR})

# Extra flags for GCC
if (CMAKE_COMPILER_IS_GNUCC OR (CMAKE_C_COMPILER_ID MATCHES "Clang"))
    # 移除xml2-config调用，改用CMake变量
    add_definitions(
        -pipe
        -Wall -Wextra -Wcast-align -Wpointer-arith -O3
        -Wno-unused-parameter -msse2)
    
    # 在Windows上不使用xml2-config，改用pkg-config或直接链接
    if(NOT WIN32)
        # 只在非Windows系统上使用xml2-config
        execute_process(COMMAND xml2-config --cflags 
                       OUTPUT_VARIABLE XML2_CFLAGS
                       OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND xml2-config --libs
                       OUTPUT_VARIABLE XML2_LIBS
                       OUTPUT_STRIP_TRAILING_WHITESPACE)
        add_definitions(${XML2_CFLAGS})
    endif()
endif (CMAKE_COMPILER_IS_GNUCC OR (CMAKE_C_COMPILER_ID MATCHES "Clang"))

file(GLOB libcommon_headers ../../libs/libcommon/*.h)
file(GLOB libcommon_sources ../../libs/libcommon/*.c)
source_group(libcommon FILES ${libcommon_headers} ${libcommon_sources})

file(GLOB libdstdec_headers ../../libs/libdstdec/*.h)
file(GLOB libdstdec_sources ../../libs/libdstdec/*.c)
source_group(libdstdec FILES ${libdstdec_headers} ${libdstdec_sources})

file(GLOB libid3_headers ../../libs/libid3/*.h)
file(GLOB libid3_sources ../../libs/libid3/*.c)
source_group(libid3 FILES ${libid3_headers} ${libid3_sources})

file(GLOB libsacd_headers ../../libs/libsacd/*.h)
file(GLOB libsacd_sources ../../libs/libsacd/*.c)
source_group(libsacd FILES ${libsacd_headers} ${libsacd_sources})

file(GLOB main_headers ./*.h)
file(GLOB main_sources ./*.c)
source_group(main FILES ${main_headers} ${main_sources})

# 设置编译选项
if(WIN32)
    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS 
        -DUNICODE 
        -D_UNICODE
        -DWIN32
        -DHAVE_STDINT_H
        -DPACKAGE_NAME="sacd_extract"
        -DPACKAGE_VERSION="0.3.9"
    )
endif()

add_executable(sacd_extract ${SOURCES})

target_link_libraries(sacd_extract 
    ${LIBXML2_LIBRARIES}
    Threads::Threads
)

# 链接库设置
if (MSVC)
    SET (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    SET (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT /D PTW32_STATIC_LIB")
    ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_DEPRECATE)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
    ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_WARNINGS)
    SET (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ws2_32.lib pthreadVC2.lib iconv.lib libxml2.lib")
    SET (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ws2_32.lib pthreadVC2_static.lib iconv.lib /NODEFAULTLIB:LIBCMT.LIB") 

    # disable warnings about libiconv link directory
    cmake_policy(SET CMP0015 NEW)

    include_directories("../../libs/libiconv/include")
    link_directories("../../libs/libiconv/lib/")
elseif(WIN32)
    # 修复拼写错误：CMAKE_CXX_STANDARD_LIBRARIES
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lpthread -lws2_32 -liconv")
    target_compile_options(sacd_extract PRIVATE -municode)
    target_link_libraries(sacd_extract -municode)
    
    # 链接LibXml2
    target_link_libraries(sacd_extract ${LIBXML2_LIBRARIES})
    
    add_definitions(-D_FILE_OFFSET_BITS=64)
    add_definitions(-DUNICODE -D_UNICODE -DIN_LIBXML)
elseif(APPLE)
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -liconv -lpthread")
    target_link_libraries(sacd_extract ${LIBXML2_LIBRARIES})
else()
    add_definitions(-D_FILE_OFFSET_BITS=64)
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lpthread")
    target_link_libraries(sacd_extract ${LIBXML2_LIBRARIES})
endif()