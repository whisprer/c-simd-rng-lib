#include "rng_includes.h"

// universal_bench.cpp - Updated to use std::unique_ptr
class BenchmarkStats {
    public:
        // Constructors
        BenchmarkStats() = default;
        
        // Compute statistics from a vector of times
        static BenchmarkStats compute(const std::vector<double>& times) {
            BenchmarkStats stats;
            
            if (times.empty()) {
                return stats;
            }
            
            // Find min and max
            stats.min = *std::min_element(times.begin(), times.end());
            stats.max = *std::max_element(times.begin(), times.end());
            
            // Calculate average
            stats.avg = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
            
            // Calculate standard deviation
            double variance_sum = 0.0;
            for (double time : times) {
                double diff = time - stats.avg;
                variance_sum += diff * diff;
            }
            
            stats.std_dev = std::sqrt(variance_sum / times.size());
            
            // Calculate variance percentage
            stats.variance_percentage = (stats.std_dev / stats.avg) * 100.0;
            
            return stats;
        }
        
        // Getters with performance-friendly const references
        [[nodiscard]] double getMin() const { return min; }
        [[nodiscard]] double getMax() const { return max; }
        [[nodiscard]] double getAvg() const { return avg; }
        [[nodiscard]] double getStdDev() const { return std_dev; }
        [[nodiscard]] double getVariancePercentage() const { return variance_percentage; }
        
        // Performance metrics calculation
        [[nodiscard]] double calculateThroughput(uint64_t count) const {
            return count / avg / 1e6;  // Million operations per second
        }
        
        // Speedup calculation
        [[nodiscard]] double calculateSpeedup(const BenchmarkStats& other) const {
            return other.avg / this->avg;
        }
    
    private:
        double min = 0.0;
        double max = 0.0;
        double avg = 0.0;
        double std_dev = 0.0;
        double variance_percentage = 0.0;
    };
    
    // Comparison function for benchmark stats
    inline bool operator<(const BenchmarkStats& lhs, const BenchmarkStats& rhs) {
        return lhs.getAvg() < rhs.getAvg();
    }
    
    // Output stream operator for easy printing
    std::ostream& operator<<(std::ostream& os, const BenchmarkStats& stats) {
        os << "Min: " << stats.getMin() << " s, "
           << "Max: " << stats.getMax() << " s, "
           << "Avg: " << stats.getAvg() << " s, "
           << "StdDev: " << stats.getStdDev() << " s, "
           << "Variance: " << stats.getVariancePercentage() << "%";
        return os;
    }
