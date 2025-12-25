#include "universal_rng.h"
#include <iostream>
#include <iomanip>
#include <vector>

void print_hex(const char* label, const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    std::cout << label << ": ";
    for (size_t i = 0; i < size; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

int main() {
    std::cout << "Bit Width Test\n";
    std::cout << "==============\n\n";
    
    // Create RNG
    universal_rng_t* rng = universal_rng_new(42, 0, 1); // Xoroshiro, double precision
    if (!rng) {
        std::cerr << "Failed to create RNG!" << std::endl;
        return 1;
    }
    
    const char* impl = universal_rng_get_implementation(rng);
    std::cout << "Using implementation: " << impl << std::endl << std::endl;
    
    // Test 16-bit generation
    uint16_t val16 = universal_rng_next_u16(rng);
    print_hex("16-bit", &val16, sizeof(val16));
    
    // Test 32-bit generation
    uint32_t val32 = universal_rng_next_u32(rng);
    print_hex("32-bit", &val32, sizeof(val32));
    
    // Test 64-bit generation
    uint64_t val64 = universal_rng_next_u64(rng);
    print_hex("64-bit", &val64, sizeof(val64));
    
    // Fix for line 43-44
    uint64_t val128[2] = {0};
    universal_rng_next_u128(rng, val128);
    print_hex("128-bit", val128, sizeof(val128));
    
    // Test 256-bit generation
    uint64_t val256[4] = {0};
    universal_rng_next_u256(rng, val256);
    print_hex("256-bit", val256, sizeof(val256));
    
    // Test 512-bit generation
    uint64_t val512[8] = {0};
    universal_rng_next_u512(rng, val512);
    print_hex("512-bit", val512, sizeof(val512));
    
    // Test 1024-bit generation
    uint64_t val1024[16] = {0};
    universal_rng_next_u1024(rng, val1024);
    print_hex("1024-bit", val1024, sizeof(val1024));
    
    // Generate more values to demonstrate randomness
    std::cout << "\nAdditional 16-bit values:\n";
    for (int i = 0; i < 5; i++) {
        uint16_t value = universal_rng_next_u16(rng);
        std::cout << "  " << std::hex << std::setw(4) << std::setfill('0') << value << std::dec << std::endl;
    }
    
    std::cout << "\nAdditional 32-bit values:\n";
    for (int i = 0; i < 5; i++) {
        uint32_t value = universal_rng_next_u32(rng);
        std::cout << "  " << std::hex << std::setw(8) << std::setfill('0') << value << std::dec << std::endl;
    }
    
    // Clean up
    universal_rng_free(rng);
    universal_rng_free_string(impl);
    
    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}