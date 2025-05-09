﻿# CMakeList.txt : CMake project for ImageLoader, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ImageLoader")

# Add source to this project's executable.
add_executable (ImageLoader "main.cpp" "main.h" "Image.h" "ImageCache.h" "ImageLoader.h" "ImageDataReader.h" "ImageFactory.h" "Implementations/ImageSource.h" "Implementations/ImageCache.h" "Implementations/ImageCache.inl" "stb/stb_image.h" "stb/stb_image_resize2.h" "Implementations/ImageDataReader.h" "Implementations/ImageLoader.h" "UnitTests/AcceptanceTests.h" "UnitTests/ImageDataReaderTests.h" "UnitTests/UnitTestsSetup.h" "UnitTests/ImageCacheTests.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ImageLoader PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
