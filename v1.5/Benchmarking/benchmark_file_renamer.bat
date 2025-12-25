@echo off
setlocal enabledelayedexpansion

echo RNG Benchmark File Renamer
echo =========================
echo.
echo This script will rename your benchmark result files to a format
echo that's easier for the analyzer to process.
echo.

:: Check if benchmark_results directory exists
if not exist benchmark_results (
    echo No benchmark_results directory found!
    echo Please run this script in the same directory as your benchmark results.
    goto :eof
)

:: Create a directory for formatted results
if not exist formatted_results mkdir formatted_results

echo Processing files in benchmark_results directory...
echo.

:: Process file patterns for different bit widths
for %%b in (1 2 3 4 5 6 7) do (
    set "bitwidth="
    
    if "%%b"=="1" set "bitwidth=16"
    if "%%b"=="2" set "bitwidth=32"
    if "%%b"=="3" set "bitwidth=64"
    if "%%b"=="4" set "bitwidth=256"
    if "%%b"=="5" set "bitwidth=512"
    if "%%b"=="6" set "bitwidth=1024"
    if "%%b"=="7" set "bitwidth=128"
    
    if defined bitwidth (
        :: Process all run files for this bit width
        set "count=0"
        for %%f in (benchmark_results\results_%%b_*.txt) do (
            set /a count+=1
            echo Copying %%f to formatted_results\benchmark_!bitwidth!bit_!count!.txt
            copy "%%f" "formatted_results\benchmark_!bitwidth!bit_!count!.txt" > nul
        )
        
        if !count! GTR 0 (
            echo Processed !count! files for !bitwidth!-bit width.
        ) else (
            echo No files found for !bitwidth!-bit width.
        )
    )
)

:: Also copy any existing benchmark_*bit.txt files
for %%f in (benchmark_*bit.txt) do (
    echo Copying %%f to formatted_results\
    copy "%%f" "formatted_results\" > nul
)

:: Copy the analyzer script to the formatted_results directory
if exist "optimized-analyzer.py" (
    echo Copying optimized-analyzer.py to formatted_results\
    copy "optimized-analyzer.py" "formatted_results\" > nul
)

echo.
echo All files processed. Please run the analyzer script in the
echo formatted_results directory:
echo.
echo   cd formatted_results
echo   python optimized-analyzer.py
echo.
pause