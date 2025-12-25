Folder PATH listing for volume D:/
Volume serial number is 6058-300D
D:.
|   .gitattributes
|   .gitignore
|   CHANGELOG.md
|   CONTRIBUTING.md
|   Development\_Roadmap.md
|   docs.zip
|   File-Structure.md
|   INSTALLATION.md
|   LICENSE.md
|   README.md
|   Scrambled-Linear-PRNGs-D.Blackman\_and\_S.Vigna.pdf
|   src.zip
|  
+---lib\_files
|   |   usage.md
|   |  
|   +---linux\_shared
|   |       libuniversal\_rng.dll
|   |       libuniversal\_rng.dll.a
|   |       rng\_selftest.exe
|   |  
|   +---mingw\_shared
|   |       libuniversal\_rng.dll
|   |       libuniversal\_rng.dll.a
|   |  
|   +---msvc\_shared
|   |       rng\_selftest.exe
|   |       universal\_rng.dll
|   |       universal\_rng.lib
|   |  
|   ---universal
|           rng\_selftest.exe
|           universal\_rng.a
|           universal\_rng.lib
|  
---non-core-files
|   CMakeLists.bth
|   CMakeLists.lnx
|   CMakeLists.stc
|   CMakeLists.txt
|   CMakeLists.win
|   universal\_rng.def
|  
+---extras
|       ---windows
|       |   env\_mingw64.bat
|       |  
|        ---Testing
|                ---Temporary
|                   CTestCostData.txt
|                   LastTest.log
|  
+---include
|       avx2\_impl.h
|       avx512\_impl.h
|       avx\_impl.h
|       cpu\_detect.h
|       gpu\_optimization\_detection.h
|       opencl\_rng\_integration.h
|       opencl\_rng\_kernels.h
|       rng\_common.h
|       rng\_core.h
|       rng\_includes.h
|       runtime\_detect.h
|       scalar\_impl.h
|       sse2\_impl.h
|       universal\_rng.h
|       universal\_rng.hnew.h
|       universal\_rng.old
|       universal\_rng\_export.h
|       universal\_rng\_types.h
|       wyrand\_impl.h
|       xoroshiro\_impl.h
|  
+---src
|       cpu\_detect.cpp
|       opencl\_isi.cpp
|       runtime\_detect.cpp
|       simd\_avx2.cpp
|       simd\_avx512.cpp
|       simd\_sse2.cpp
|       universal\_rng.cpp
|       universal\_rng\_api.cpp
|       universal\_rng\_bitwidth.cpp
|       universal\_rng\_scalar.cpp
|  
---tests
CMakeLists.txt
test\_rng.cpp

