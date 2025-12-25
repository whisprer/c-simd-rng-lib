@echo off
setlocal enabledelayedexpansion

:: Configuration
set ITERATIONS=100000000
set BIT_WIDTHS=1 2 3 4 5 6 7
set RUNS=10
set OUTPUT_DIR=benchmark_results

:: Create output directory if it doesn't exist
if not exist %OUTPUT_DIR% mkdir %OUTPUT_DIR%

echo RNG Benchmark Automation Script
echo ===============================
echo.
echo This script will run %RUNS% iterations of each bit width benchmark
echo and save results to %OUTPUT_DIR% directory.
echo.
echo Press any key to start benchmarks...
pause > nul

:: Run benchmarks
for %%b in (%BIT_WIDTHS%) do (
    echo.
    echo ========================================
    echo Starting benchmarks for bit width %%b...
    echo ========================================
    
    for /l %%r in (1,1,%RUNS%) do (
        echo Run %%r of %RUNS% for bit width %%b
        
        :: Create temporary input file - Fixed this part
        echo %%b > temp_input.txt
        echo %ITERATIONS% >> temp_input.txt
        
        :: Run benchmark with redirected input and output
        enhanced_bitwidth_benchmark.exe < temp_input.txt > "%OUTPUT_DIR%\results_%%b_%%r.txt"
        
        :: Optional: Add delay between runs to prevent thermal throttling
        timeout /t 2 > nul
    )
)

:: Clean up
del temp_input.txt

echo.
echo ========================================
echo All benchmarks completed!
echo Results saved to %OUTPUT_DIR% directory
echo ========================================

:: Run the Python analyzer if available
if exist benchmark_analyzer.py (
    echo.
    echo Running analysis script...
    python benchmark_analyzer.py
    echo Analysis complete!
) else (
    echo.
    echo To analyze results, run the benchmark_analyzer.py script
)

echo.
echo Press any key to exit...
pause > nul