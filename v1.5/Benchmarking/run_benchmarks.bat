@echo off
setlocal enabledelayedexpansion

set ITERATIONS=100000000
set BIT_WIDTHS=1 2 3 4 5 6 7
set RUNS=10

echo Running %RUNS% iterations of benchmark for each bit width...

for %%b in (%BIT_WIDTHS%) do (
    echo Starting benchmarks for bit width %%b...
    
    for /l %%r in (1,1,%RUNS%) do (
        echo Run %%r of %RUNS% for bit width %%b
        echo %%b > temp_input.txt
        echo %ITERATIONS% >> temp_input.txt
        
        enhanced_bitwidth_benchmark.exe < temp_input.txt > results_%%b_%%r.txt
    )
)

echo All benchmarks completed