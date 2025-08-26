#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

// Simple test program to verify Pokemon sprite loading
// This avoids the Metal GPU crashes in the full emulator

struct SpriteHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    uint32_t reserved;
};

bool loadPokemonSprite(uint16_t pokemonId, uint8_t* spriteBuffer, size_t bufferSize) {
    // Load sprite from pokemon_sprites.bin file (using correct format from POKEDEX.cpp)
    FILE* spriteFile = fopen("data/pokemon/pokemon_sprites.bin", "rb");
    if (!spriteFile) {
        std::cout << "[TEST] Could not open pokemon_sprites.bin" << std::endl;
        return false;
    }
    
    // Read header: count (2 bytes)
    uint16_t spriteCount;
    if (fread(&spriteCount, 2, 1, spriteFile) != 1) {
        std::cout << "[TEST] Error reading sprite count" << std::endl;
        fclose(spriteFile);
        return false;
    }
    
    std::cout << "[TEST] Sprite file contains " << spriteCount << " Pokemon sprites" << std::endl;
    
    // Validate Pokemon ID
    if (pokemonId == 0 || pokemonId > spriteCount) {
        std::cout << "[TEST] Invalid Pokemon ID for sprite: " << pokemonId << " (max: " << spriteCount << ")" << std::endl;
        fclose(spriteFile);
        return false;
    }
    
    // Read offset for this Pokemon (4 bytes each, 0-indexed)
    uint32_t spriteOffset;
    fseek(spriteFile, 2 + (pokemonId - 1) * 4, SEEK_SET);
    if (fread(&spriteOffset, 4, 1, spriteFile) != 1) {
        std::cout << "[TEST] Error reading sprite offset for Pokemon " << pokemonId << std::endl;
        fclose(spriteFile);
        return false;
    }
    
    // Seek to sprite data location (after header)
    uint32_t dataStart = 2 + spriteCount * 4;
    fseek(spriteFile, dataStart + spriteOffset, SEEK_SET);
    
    // Read sprite size (2 bytes)
    uint16_t spriteSize;
    if (fread(&spriteSize, 2, 1, spriteFile) != 1) {
        std::cout << "[TEST] Error reading sprite size for Pokemon " << pokemonId << std::endl;
        fclose(spriteFile);
        return false;
    }
    
    // Validate buffer size
    if (spriteSize > bufferSize) {
        std::cout << "[TEST] Sprite too large for buffer: " << spriteSize << " > " << bufferSize << std::endl;
        fclose(spriteFile);
        return false;
    }
    
    // Read sprite data
    if (fread(spriteBuffer, 1, spriteSize, spriteFile) != spriteSize) {
        std::cout << "[TEST] Error reading sprite data for Pokemon " << pokemonId << std::endl;
        fclose(spriteFile);
        return false;
    }
    
    fclose(spriteFile);
    std::cout << "[TEST] Loaded sprite for Pokemon " << pokemonId << " (" << spriteSize << " bytes)" << std::endl;
    return true;
}

void testSpriteCache() {
    std::cout << "[TEST] Testing Pokemon sprite loading..." << std::endl;

    // Test loading a few Pokemon sprites (sprites are 64x64 = 512 bytes)
    uint8_t spriteBuffer[64 * 64 / 8]; // 64x64 1-bit bitmap = 512 bytes

    // Test Pokemon #1 (Bulbasaur)
    if (loadPokemonSprite(1, spriteBuffer, sizeof(spriteBuffer))) {
        std::cout << "[TEST] Successfully loaded Bulbasaur sprite" << std::endl;
        
        // Print first few bytes as hex for verification
        std::cout << "[TEST] First 16 bytes: ";
        for (int i = 0; i < 16 && i < sizeof(spriteBuffer); i++) {
            printf("%02X ", spriteBuffer[i]);
        }
        std::cout << std::endl;
    }

    // Test Pokemon #25 (Pikachu)
    if (loadPokemonSprite(25, spriteBuffer, sizeof(spriteBuffer))) {
        std::cout << "[TEST] Successfully loaded Pikachu sprite" << std::endl;
        
        // Print first few bytes as hex for verification
        std::cout << "[TEST] First 16 bytes: ";
        for (int i = 0; i < 16 && i < sizeof(spriteBuffer); i++) {
            printf("%02X ", spriteBuffer[i]);
        }
        std::cout << std::endl;
    }

    // Test Pokemon #150 (Mewtwo)
    if (loadPokemonSprite(150, spriteBuffer, sizeof(spriteBuffer))) {
        std::cout << "[TEST] Successfully loaded Mewtwo sprite" << std::endl;
        
        // Print first few bytes as hex for verification
        std::cout << "[TEST] First 16 bytes: ";
        for (int i = 0; i < 16 && i < sizeof(spriteBuffer); i++) {
            printf("%02X ", spriteBuffer[i]);
        }
        std::cout << std::endl;
    }

    // Test 1bpp to 4bpp conversion (like in SpriteCache::get64 since sprites are 64x64)
    std::cout << "[TEST] Testing 1bpp to 4bpp conversion..." << std::endl;
    
    if (loadPokemonSprite(1, spriteBuffer, sizeof(spriteBuffer))) {
        uint8_t spriteBuffer4bpp[64 * 64 / 2]; // 64x64 4bpp bitmap for display
        memset(spriteBuffer4bpp, 0, sizeof(spriteBuffer4bpp));
        int stride = (64 + 1) / 2; // Bytes per row in 4bpp format

        // Convert 1bpp to 4bpp format (same logic as in PokedexUI.cpp but for 64x64)
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                int srcByteIndex = (y * 64 + x) / 8;
                int srcBitIndex = (y * 64 + x) % 8;
                bool pixel = (spriteBuffer[srcByteIndex] >> (7 - srcBitIndex)) & 1;

                if (pixel) {
                    int dstByteIndex = y * stride + x / 2;
                    if (x % 2 == 0) {
                        spriteBuffer4bpp[dstByteIndex] |= 0x0F; // Low nibble - black pixel
                    } else {
                        spriteBuffer4bpp[dstByteIndex] |= 0xF0; // High nibble - black pixel
                    }
                }
            }
        }

        // Count non-zero pixels in 4bpp output
        int pixelCount = 0;
        for (int i = 0; i < sizeof(spriteBuffer4bpp); i++) {
            if (spriteBuffer4bpp[i] != 0) pixelCount++;
        }
        std::cout << "[TEST] Conversion complete, " << pixelCount << " non-zero bytes in 4bpp output" << std::endl;
    }

    std::cout << "[TEST] Sprite loading test complete!" << std::endl;
}

int main() {
    std::cout << "[TEST] Pokemon Sprite Loading Test" << std::endl;
    std::cout << "[TEST] ================================" << std::endl;
    
    testSpriteCache();
    
    return 0;
}
