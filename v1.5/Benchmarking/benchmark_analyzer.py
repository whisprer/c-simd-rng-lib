import os
import re
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path

# Define a function to parse a single benchmark result file
def parse_benchmark_file(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Extract bit width - be more flexible with the regex
        bit_width_match = re.search(r'(?:Benchmark Results for|benchmarks with \d+ iterations for) (\d+)-bit', content)
        if not bit_width_match:
            # Try to extract from filename if it contains bit width
            filename = str(file_path.name).lower()
            if 'bit' in filename:
                bit_match = re.search(r'(\d+)bit', filename)
                if bit_match:
                    bit_width = int(bit_match.group(1))
                    print(f"  Extracted bit width {bit_width} from filename: {filename}")
                else:
                    print(f"No bit width found in {file_path}")
                    return None
            else:
                print(f"No bit width found in {file_path}")
                return None
        else:
            bit_width = int(bit_width_match.group(1))
            
        print(f"  Processing {bit_width}-bit data from {file_path.name}")
        
        # More flexible regex pattern to match benchmark results
        # This pattern looks for lines with an implementation name, mode, time, and speed
        results_pattern = r'([\w\:\+\-\d]+(?:\s+[\w\:\+\-\d]+)*)\s+(Single|Batch)\s+([\d\.]+)\s+([\d\.]+)'
        results = re.findall(results_pattern, content)
        
        if not results:
            print(f"  No benchmark results found in {file_path} using primary pattern.")
            print("  Trying a more flexible pattern...")
            
            # Try an alternate pattern
            results_pattern = r'(\S.*?)\s+(Single|Batch)\s+([\d\.]+)\s+([\d\.]+)'
            results = re.findall(results_pattern, content)
            
        if not results:
            print(f"  Still no results found in {file_path}.")
            print("  Looking for results table section...")
            
            # Look for the results table specifically
            table_section = re.search(r'Benchmark Results.*?-+\n(.*?)(?:\n\n|\nFastest|\Z)', content, re.DOTALL)
            if table_section:
                table_text = table_section.group(1)
                print(f"  Found results table section of {len(table_text)} chars")
                
                # Try line by line
                for line in table_text.split('\n'):
                    line = line.strip()
                    if line and not line.startswith('-'):
                        parts = re.split(r'\s{2,}', line)
                        if len(parts) >= 4:
                            impl = parts[0]
                            mode = parts[1]
                            time = parts[2]
                            speed = parts[3]
                            
                            if mode in ['Single', 'Batch'] and all(re.match(r'[\d\.]+', s) for s in [time, speed]):
                                results.append((impl, mode, time, speed))
                                print(f"  Extracted: {impl} | {mode} | {time} | {speed}")
        
        if not results:
            print(f"  Failed to extract data from {file_path}")
            print("  Sample content:")
            print(content[:500])
            return None
        
        print(f"  Successfully extracted {len(results)} data points")
        
        data = []
        for match in results:
            impl, mode, time_sec, speed_ms = match
            
            # Clean up implementation name by trimming and removing extra spaces
            impl = ' '.join(impl.split())
            
            # Ensure implementation names are standardized
            if 'mt19937' in impl.lower():
                impl = 'std::mt19937_64'
            
            # Try to extract run number from filename
            run_num = 1
            if '_' in file_path.stem:
                parts = file_path.stem.split('_')
                if len(parts) > 1 and parts[-1].isdigit():
                    run_num = int(parts[-1])
            
            try:
                data.append({
                    'bit_width': bit_width,
                    'implementation': impl,
                    'mode': mode,
                    'time_sec': float(time_sec),
                    'speed_ms': float(speed_ms),
                    'run': run_num,
                    'filename': str(file_path)
                })
            except ValueError as e:
                print(f"  Error converting values: {e}")
                print(f"  Problem data: {impl}, {mode}, {time_sec}, {speed_ms}")
        
        if not data:
            print(f"  No valid data points extracted from {file_path}")
            return None
            
        return data
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        return None

# Function to process all benchmark files
def process_all_benchmarks(directory='.'):
    all_data = []
    
    # Look for benchmark results in the current directory and any subdirectories
    search_paths = [Path(directory)]
    benchmark_results_dir = Path(directory, 'formatted_results')
    if benchmark_results_dir.exists():
        search_paths.append(benchmark_results_dir)
    
    print("Searching for benchmark files in:")
    for path in search_paths:
        print(f"  - {path.absolute()}")
    
    # Get list of files to process
    found_files = []
    
    # First priority: files with 'bit' in the name
    for path in search_paths:
        for file_path in path.glob('*bit*.txt'):
            found_files.append(file_path)
    
    # Second priority: files with 'benchmark' or 'results' in the name
    if not found_files:
        for path in search_paths:
            for file_path in path.glob('*.txt'):
                if 'benchmark' in file_path.name.lower() or 'results' in file_path.name.lower():
                    found_files.append(file_path)
    
    # Last resort: any txt files
    if not found_files:
        for path in search_paths:
            for file_path in path.glob('*.txt'):
                found_files.append(file_path)
    
    if not found_files:
        print("No benchmark result files found.")
        print("Creating a sample file with the provided data...")
        
        # Create a sample file from the given benchmark data
        sample_file = Path('benchmark_16bit_sample.txt')
        with open(sample_file, 'w') as f:
            f.write("""
Benchmark Results for 16-bit
================================
Implementation           Mode      Time (sec)     Speed (M/s)
-----------------------------------------------------------------
AVX2 Xoroshiro128++      Single    0.4496         222.43
AVX2 Xoroshiro128++      Batch     0.1389         720.17
AVX2 WyRand              Single    0.4247         235.45
AVX2 WyRand              Batch     0.1785         560.13
std::mt19937_64          Single    0.5109         195.74
std::mt19937_64          Batch     0.2290         436.61
Xoroshiro128+            Single    0.1150         869.47
Xoroshiro128+            Batch     0.1133         882.35
            """)
        print(f"Created sample file: {sample_file}")
        found_files.append(sample_file)
    
    print(f"\nFound {len(found_files)} potential benchmark files")
    
    # Process each file
    for idx, file_path in enumerate(found_files):
        print(f"\nProcessing file {idx+1}/{len(found_files)}: {file_path.name}")
        data = parse_benchmark_file(file_path)
        if data:
            all_data.extend(data)
    
    if not all_data:
        print("\nNo valid benchmark data found in any files.")
        return pd.DataFrame()
    
    # Convert to DataFrame
    df = pd.DataFrame(all_data)
    
    # Clean up implementation names
    implementations = df['implementation'].unique()
    print("\nFound implementations:")
    for impl in implementations:
        print(f"  - '{impl}'")
    
    # Fix any implementation name issues
    if '17' in implementations:
        print("  Fixing malformed implementation name '17' -> 'std::mt19937_64'")
        df.loc[df['implementation'] == '17', 'implementation'] = 'std::mt19937_64'
    
    # Make sure std::mt19937_64 is consistent
    if 'mt19937_64' in implementations and 'std::mt19937_64' in implementations:
        print("  Standardizing 'mt19937_64' -> 'std::mt19937_64'")
        df.loc[df['implementation'] == 'mt19937_64', 'implementation'] = 'std::mt19937_64'
    
    # Remove any rows with NaN values
    original_len = len(df)
    df = df.dropna()
    if len(df) < original_len:
        print(f"  Dropped {original_len - len(df)} rows with missing values")
    
    return df

# Generate visualizations
def generate_visualizations(df):
    if df.empty:
        print("No data to visualize!")
        return

    # Set the style
    sns.set(style="whitegrid")
    plt.rcParams.update({'font.size': 12})
    
    print("\nGenerating visualizations...")
    print(f"Data summary: {len(df)} data points across {df['bit_width'].nunique()} bit widths")
    
    # 1. Performance across bit widths by implementation and mode
    print("\nCreating performance by bit width chart...")
    plt.figure(figsize=(15, 10))
    
    # Use log scale only if we have multiple bit widths
    use_log_scale = df['bit_width'].nunique() > 1
    
    sns.lineplot(
        data=df, 
        x='bit_width', 
        y='speed_ms', 
        hue='implementation',
        style='mode',
        markers=True,
        dashes=False,
        linewidth=2.5,
        markersize=10
    )
    plt.title('RNG Performance Across Bit Widths', fontsize=18)
    plt.xlabel('Bit Width', fontsize=16)
    plt.ylabel('Speed (M operations/sec)', fontsize=16)
    
    if use_log_scale:
        plt.xscale('log', base=2)
        plt.yscale('log')
    
    plt.grid(True, which="both", ls="-", alpha=0.2)
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=12)
    plt.tight_layout()
    plt.savefig('rng_performance_by_bitwidth.png', dpi=300)
    print("  Saved to rng_performance_by_bitwidth.png")
    
    # 2. Batch vs Single mode speedup
    print("\nCreating batch vs single mode speedup chart...")
    plt.figure(figsize=(15, 10))
    speedup_data = []
    
    for bit_width in df['bit_width'].unique():
        for impl in df['implementation'].unique():
            bit_impl_data = df[(df['bit_width'] == bit_width) & (df['implementation'] == impl)]
            
            single_data = bit_impl_data[bit_impl_data['mode'] == 'Single']
            batch_data = bit_impl_data[bit_impl_data['mode'] == 'Batch']
            
            if not single_data.empty and not batch_data.empty:
                avg_single = single_data['speed_ms'].mean()
                avg_batch = batch_data['speed_ms'].mean()
                
                if avg_single > 0:
                    speedup = avg_batch / avg_single
                    speedup_data.append({
                        'bit_width': bit_width,
                        'implementation': impl,
                        'speedup': speedup
                    })
    
    speedup_df = pd.DataFrame(speedup_data)
    if not speedup_df.empty:
        sns.barplot(data=speedup_df, x='bit_width', y='speedup', hue='implementation')
        plt.title('Batch Mode Speedup Factor by Bit Width', fontsize=18)
        plt.xlabel('Bit Width', fontsize=16)
        plt.ylabel('Speedup Factor (Batch/Single)', fontsize=16)
        plt.axhline(y=1, color='r', linestyle='--', alpha=0.7)
        plt.grid(True, which="both", ls="-", alpha=0.2)
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=12)
        plt.tight_layout()
        plt.savefig('batch_vs_single_speedup.png', dpi=300)
        print("  Saved to batch_vs_single_speedup.png")
    else:
        print("  No data for batch vs single comparison")
    
    # 3. Implementation comparison for each bit width
    print("\nCreating implementation comparison charts for each bit width...")
    
    for bit_width in sorted(df['bit_width'].unique()):
        plt.figure(figsize=(14, 8))
        bit_data = df[df['bit_width'] == bit_width]
        
        # Create a simple bar chart showing performance by implementation and mode
        plt.figure(figsize=(14, 8))
        
        # Plot as grouped bar chart
        sns.barplot(
            data=bit_data,
            x='implementation',
            y='speed_ms',
            hue='mode',
            palette=['skyblue', 'lightgreen']
        )
        
        plt.title(f'RNG Implementation Performance ({bit_width}-bit)', fontsize=18)
        plt.xlabel('Implementation', fontsize=16)
        plt.ylabel('Speed (M operations/sec)', fontsize=16)
        plt.grid(True, which="both", ls="-", alpha=0.2)
        plt.xticks(rotation=15)
        plt.legend(title='Mode')
        plt.tight_layout()
        plt.savefig(f'implementation_performance_{bit_width}bit.png', dpi=300)
        print(f"  Saved to implementation_performance_{bit_width}bit.png")
    
    # 4. Relative performance compared to std::mt19937_64
    print("\nCreating relative performance charts...")
    relative_data = []
    
    for bit_width in df['bit_width'].unique():
        for mode in ['Single', 'Batch']:
            bit_mode_data = df[(df['bit_width'] == bit_width) & (df['mode'] == mode)]
            
            # Check if we have std::mt19937_64 for this bit width and mode
            std_data = bit_mode_data[bit_mode_data['implementation'] == 'std::mt19937_64']
            
            if not std_data.empty:
                std_speed = std_data['speed_ms'].mean()
                
                for impl in bit_mode_data['implementation'].unique():
                    if impl != 'std::mt19937_64':
                        impl_data = bit_mode_data[bit_mode_data['implementation'] == impl]
                        impl_speed = impl_data['speed_ms'].mean()
                        
                        if std_speed > 0:
                            relative = impl_speed / std_speed
                            relative_data.append({
                                'bit_width': bit_width,
                                'mode': mode,
                                'implementation': impl,
                                'relative_performance': relative
                            })
    
    relative_df = pd.DataFrame(relative_data)
    if not relative_df.empty:
        try:
            # Create the FacetGrid
            g = sns.catplot(
                data=relative_df, 
                x='bit_width', 
                y='relative_performance', 
                hue='implementation',
                col='mode',
                kind='bar',
                height=6, 
                aspect=1.2
            )
            g.set_axis_labels("Bit Width", "Performance Relative to std::mt19937_64")
            g.set_titles("{col_name} Mode")
            g.fig.suptitle('RNG Performance Relative to std::mt19937_64', fontsize=16, y=1.05)
            
            # Add horizontal reference line
            for ax in g.axes.flat:
                ax.axhline(y=1, color='r', linestyle='--', alpha=0.7)
            
            plt.tight_layout()
            plt.savefig('relative_performance.png', dpi=300)
            print("  Saved to relative_performance.png")
        except Exception as e:
            print(f"  Error creating relative performance chart: {e}")
            
            # Try a simpler approach if the FacetGrid fails
            for mode in ['Single', 'Batch']:
                mode_data = relative_df[relative_df['mode'] == mode]
                if not mode_data.empty:
                    plt.figure(figsize=(14, 8))
                    sns.barplot(
                        data=mode_data,
                        x='bit_width',
                        y='relative_performance',
                        hue='implementation'
                    )
                    plt.title(f'RNG Performance Relative to std::mt19937_64 ({mode} Mode)', fontsize=16)
                    plt.xlabel('Bit Width', fontsize=14)
                    plt.ylabel('Performance Relative to std::mt19937_64', fontsize=14)
                    plt.axhline(y=1, color='r', linestyle='--', alpha=0.7)
                    plt.grid(True, which="both", ls="-", alpha=0.2)
                    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=12)
                    plt.tight_layout()
                    plt.savefig(f'relative_performance_{mode}.png', dpi=300)
                    print(f"  Saved to relative_performance_{mode}.png")
    else:
        print("  No data for relative performance comparison")
    
    # 5. Summary statistics table
    print("\nGenerating summary statistics...")
    try:
        # Group by bit width, implementation, and mode
        summary = df.groupby(['bit_width', 'implementation', 'mode']).agg({
            'speed_ms': ['mean', 'std', 'min', 'max', 'count']
        }).reset_index()
        
        # Sort by bit width and then by mean speed (descending)
        summary = summary.sort_values(
            by=['bit_width', ('speed_ms', 'mean')], 
            ascending=[True, False]
        )
        
        # Print summary statistics
        print("\nSummary Statistics:")
        for bit_width in sorted(df['bit_width'].unique()):
            print(f"\n{bit_width}-bit Results:")
            bit_summary = summary[summary['bit_width'] == bit_width]
            
            # Format for pretty printing (avoiding the formatting issues)
            print("Implementation | Mode   | Mean (M/s) | Std Dev | Min     | Max     | Samples")
            print("--------------|--------|------------|---------|---------|---------|--------")
            
            for _, row in bit_summary.iterrows():
                impl = str(row['implementation'])
                mode = str(row['mode'])
                mean = float(row[('speed_ms', 'mean')])
                
                # Handle possible NaN values
                std = float(row[('speed_ms', 'std')]) if not pd.isna(row[('speed_ms', 'std')]) else 0
                min_val = float(row[('speed_ms', 'min')])
                max_val = float(row[('speed_ms', 'max')])
                count = int(row[('speed_ms', 'count')])
                
                impl_str = impl[:14]  # Truncate long implementation names
                print(f"{impl_str:<14} | {mode:<6} | {mean:10.2f} | {std:7.2f} | {min_val:7.2f} | {max_val:7.2f} | {count:7}")
        
        # Create a more human-readable CSV
        csv_data = []
        for _, row in summary.iterrows():
            impl = str(row['implementation'])
            mode = str(row['mode'])
            bit_width = int(row['bit_width'])
            mean = float(row[('speed_ms', 'mean')])
            std = float(row[('speed_ms', 'std')]) if not pd.isna(row[('speed_ms', 'std')]) else 0
            min_val = float(row[('speed_ms', 'min')])
            max_val = float(row[('speed_ms', 'max')])
            count = int(row[('speed_ms', 'count')])
            
            csv_data.append({
                'Bit Width': bit_width,
                'Implementation': impl,
                'Mode': mode,
                'Mean Speed (M/s)': round(mean, 2),
                'Std Dev': round(std, 2),
                'Min Speed': round(min_val, 2),
                'Max Speed': round(max_val, 2),
                'Sample Count': count
            })
        
        # Save as CSV
        csv_df = pd.DataFrame(csv_data)
        csv_df.to_csv('rng_benchmark_summary.csv', index=False)
        print("\nSummary statistics saved to rng_benchmark_summary.csv")
        
    except Exception as e:
        print(f"Error generating summary statistics: {e}")
        import traceback
        traceback.print_exc()
    
    # 6. Create a final comprehensive performance chart
    print("\nCreating comprehensive performance chart...")
    try:
        plt.figure(figsize=(16, 12))
        
        # Get average performance by bit width, implementation, mode
        avg_data = df.groupby(['bit_width', 'implementation', 'mode'])['speed_ms'].mean().reset_index()
        
        # If we have multiple bit widths
        if avg_data['bit_width'].nunique() > 1:
            # Create a heatmap-style visualization
            pivot_data = avg_data.pivot_table(
                index=['implementation', 'mode'],
                columns='bit_width',
                values='speed_ms'
            )
            
            # Sort by average performance across all bit widths
            pivot_data['avg'] = pivot_data.mean(axis=1)
            pivot_data = pivot_data.sort_values('avg', ascending=False)
            pivot_data = pivot_data.drop('avg', axis=1)
            
            # Plot heatmap
            plt.figure(figsize=(16, len(pivot_data) * 0.8))
            sns.heatmap(
                pivot_data, 
                annot=True, 
                fmt=".1f", 
                cmap="YlGnBu", 
                linewidths=0.5,
                cbar_kws={'label': 'Speed (M operations/sec)'}
            )
            plt.title('RNG Performance Comparison Across Bit Widths', fontsize=18)
            plt.xlabel('Bit Width', fontsize=16)
            plt.tight_layout()
            plt.savefig('comprehensive_performance.png', dpi=300)
            print("  Saved to comprehensive_performance.png")
    except Exception as e:
        print(f"  Error creating comprehensive chart: {e}")
    
    print("\nVisualization complete! Check the current directory for PNG files.")

# Main function
def main():
    print("RNG Benchmark Data Analyzer")
    print("==========================")
    
    # Process all benchmark data
    df = process_all_benchmarks()
    
    if df.empty:
        print("No benchmark data found. Please ensure result files are in the correct format.")
        return
    
    print(f"\nProcessed data for {df['bit_width'].nunique()} bit widths.")
    print(f"Found {df['implementation'].unique().size} implementations:")
    for impl in df['implementation'].unique():
        print(f"  - {impl}")
    print(f"Total data points: {len(df)}")
    
    # Generate visualizations
    generate_visualizations(df)

if __name__ == "__main__":
    main()