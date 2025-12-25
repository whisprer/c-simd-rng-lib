@echo off
setlocal enabledelayedexpansion

echo Manual Benchmark Data Collection
echo ===============================
echo.
echo This script will help you manually collect benchmark data
echo for each bit width and save it to separate files.
echo.
echo For each bit width, you will:
echo 1. Run the benchmark manually
echo 2. Copy the complete console output
echo 3. Paste it into the file when prompted
echo.
echo Press any key to start...
pause > nul

:: List of bit widths to test
set BIT_WIDTHS=16 32 64 256 512 1024
:: Add 128 if you've implemented it
:: set BIT_WIDTHS=16 32 64 128 256 512 1024

for %%b in (%BIT_WIDTHS%) do (
    echo.
    echo ========================================
    echo Collecting data for %%b-bit benchmarks
    echo ========================================
    echo.
    echo Please run: enhanced_bitwidth_benchmark.exe
    echo.
    echo When prompted:
    echo - Select option for %%b-bit
    echo - Enter number of iterations (100000000 recommended)
    echo.
    echo After the benchmark completes, copy ALL console output
    echo.
    echo Press any key when you're ready to paste the benchmark output...
    pause > nul
    
    echo.
    echo Please paste the entire benchmark output below.
    echo When finished, press CTRL+Z and then ENTER.
    echo.
    
    :: Create the output file and wait for user input
    type con > benchmark_%%bbit.txt
    
    echo.
    echo Saved %%b-bit benchmark data to benchmark_%%bbit.txt
    echo.
)

echo.
echo ========================================
echo All benchmark data collection complete!
echo Now run benchmark_analyzer.py to process the data.
echo ========================================
echo.
echo Press any key to exit...
pause > nul