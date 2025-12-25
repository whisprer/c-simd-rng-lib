1\. Unpack and organise

universal-rng/
├── CMakeLists.txt
├── include/                 ← all \*public\* headers
│   ├── universal\_rng.h
│   ├── universal\_rng\_types.h
│   ├── rng\_common.h
│   └── … (every other \*.h\* from /release)
├── src/                     ← implementation (\*.cpp\*)
│   ├── universal\_rng.cpp
│   ├── universal\_rng\_api.cpp
│   ├── universal\_rng\_scalar.cpp
│   ├── universal\_rng\_bitwidth.cpp
│   ├── cpu\_detect.cpp
│   ├── runtime\_detect.cpp
│   ├── simd\_sse2.cpp
│   ├── simd\_avx2.cpp
│   ├── simd\_avx512.cpp
│   └── opencl\_isi.cpp        (optional—GPU)
└── tests/
&nbsp;   ├── test\_rng.cpp
&nbsp;   └── CMakeLists.txt

Everything under include/ is what downstream code will #include.
Everything under src/ is compiled into the library.

2\. Top-level CMakeLists.txt

Now Specifically tuned for both Linux and windows.

3\. Simple self-test (tests/test\_rng.cpp)

cpp
\#include <cstdio>
\#include "universal\_rng.h"

int main() {
&nbsp;   universal\_rng\_t\* rng = universal\_rng\_new(0xDEADBEEF, /\*algorithm\*/0, /\*precision\*/0);

&nbsp;   for (int i = 0; i < 10; ++i) {
&nbsp;       uint64\_t v = universal\_rng\_next\_u64(rng);
&nbsp;       std::printf("%llu\\n", static\_cast<unsigned long long>(v));
&nbsp;   }
&nbsp;   universal\_rng\_free(rng);
&nbsp;   return 0;
}

4\. Build \& install

bash
`# Linux / WSL / macOS
mkdir build \&\& cd build
cmake .. -DCMAKE\_BUILD\_TYPE=Release              # add -DRNG\_BUILD\_SHARED=ON if you prefer .so
cmake --build . --parallel

ctest                                            # runs rng\_selftest
sudo cmake --install .`

powershell
`# PowerShell + MSVC
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE\_BUILD\_TYPE=Release
cmake --build . --config Release
ctest -C Release
cmake --install . --config Release --prefix "C:\\Program Files\\universal\_rng"`


Outputs (static build):

Linux/macOS → build/libuniversal\_rng.a
Windows → build/Release/universal\_rng.lib
For a shared build you’ll get libuniversal\_rng.so / universal\_rng.dll (+ import-lib).

5\. Using it from another C++ project
cmake

\# In your bigger project’s CMakeLists.txt

add\_subdirectory(external/universal-rng)    # assumes you vendored the folder
target\_link\_libraries(my\_app PRIVATE universal\_rng)

target\_include\_directories(my\_app PRIVATE
&nbsp;   ${PROJECT\_SOURCE\_DIR}/external/universal-rng/include)
cpp
// my\_app.cpp

\#include "universal\_rng.h"

int main() {
&nbsp;   universal\_rng\_t\* rng = universal\_rng\_new(1234, 0, 0);

&nbsp;   double x = universal\_rng\_next\_double(rng);
&nbsp;   universal\_rng\_free(rng);
}

6\. ABI-safe C interface
Because universal\_rng.h wraps everything in extern "C" { … }, any language that can call C functions (Rust FFI, Python ctypes, C#, etc.) can link against the shared library. Ship:

bash
/usr/include/universal\_rng.h
/usr/lib/libuniversal\_rng.so
and consumers just dlopen/LoadLibrary + use the C symbols.

