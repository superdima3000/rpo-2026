::set ABI=armeabi-v7a
::set ABI=x86
::set ABI=arm64-v8a
set ABI=x86_64

set ANDROID_NDK=D:\androidSDK\Sdk\ndk\28.2.13676358
set TOOL_CHAIN=%ANDROID_NDK%\build\cmake\android.toolchain.cmake
set CMAKE=D:\androidSDK\Sdk\cmake\3.22.1\bin\cmake.exe
set NINJA=D:\androidSDK\Sdk\cmake\3.22.1\bin\ninja.exe

mkdir %ABI%
cd %ABI%

%CMAKE% ..\..\mbedtls -GNinja -DCMAKE_MAKE_PROGRAM=%NINJA% -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=23 -DANDROID_ABI=%ABI% -DCMAKE_TOOLCHAIN_FILE=%TOOL_CHAIN% -DUSE_SHARED_MBEDTLS_LIBRARY=On -DENABLE_TESTING=Off

%CMAKE% --build .