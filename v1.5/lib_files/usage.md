Using Your Libraries



Windows:

cpp// Compile: cl myprogram.cpp universal\_rng.lib

// Runtime: needs universal\_rng.dll in PATH



Linux:

cpp// Compile: g++ myprogram.cpp -lunivers­al\_rng -L/path/to/lib

// Runtime: needs libunivers­al\_rng.so in LD\_LIBRARY\_PATH

