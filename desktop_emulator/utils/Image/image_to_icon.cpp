#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class ImageToIcon {
private:
    int width, height, channels;
    unsigned char* imageData;
    unsigned char* resizedData;
    std::vector<unsigned char> bitmapData;
    
    static const int ICON_SIZE = 40;  // PocketMage icon size
    
public:
    ImageToIcon() : imageData(nullptr), resizedData(nullptr), width(0), height(0), channels(0) {}
    
    ~ImageToIcon() {
        if (imageData) {
            stbi_image_free(imageData);
        }
        if (resizedData) {
            free(resizedData);
        }
    }
    
    bool loadImage(const std::string& filename) {
        imageData = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (!imageData) {
            std::cerr << "Error: Could not load image " << filename << std::endl;
            std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
            return false;
        }
        
        std::cout << "Loaded image: " << width << "x" << height << " with " << channels << " channels" << std::endl;
        return true;
    }
    
    bool resizeToIcon() {
        if (!imageData) {
            std::cerr << "Error: No image data loaded" << std::endl;
            return false;
        }
        
        // Find content bounds (non-white pixels)
        int minX = width, maxX = -1, minY = height, maxY = -1;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixelIndex = (y * width + x) * channels;
                
                // Convert to grayscale
                int gray;
                if (channels >= 3) {
                    int r = imageData[pixelIndex];
                    int g = imageData[pixelIndex + 1];
                    int b = imageData[pixelIndex + 2];
                    gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);
                } else {
                    gray = imageData[pixelIndex];
                }
                
                // If pixel is not white/light (content pixel)
                if (gray < 240) {
                    minX = std::min(minX, x);
                    maxX = std::max(maxX, x);
                    minY = std::min(minY, y);
                    maxY = std::max(maxY, y);
                }
            }
        }
        
        // If no content found, use full image
        if (maxX == -1) {
            minX = 0; maxX = width - 1;
            minY = 0; maxY = height - 1;
            std::cout << "No content bounds detected, using full image" << std::endl;
        } else {
            std::cout << "Content bounds: (" << minX << "," << minY << ") to (" << maxX << "," << maxY << ")" << std::endl;
        }
        
        // Calculate content dimensions
        int contentWidth = maxX - minX + 1;
        int contentHeight = maxY - minY + 1;
        
        // Allocate memory for resized image
        resizedData = (unsigned char*)malloc(ICON_SIZE * ICON_SIZE * channels);
        if (!resizedData) {
            std::cerr << "Error: Could not allocate memory for resized image" << std::endl;
            return false;
        }
        
        // Resize content to fill 40x40 (crop to content bounds first)
        for (int y = 0; y < ICON_SIZE; y++) {
            for (int x = 0; x < ICON_SIZE; x++) {
                // Map icon coordinates to content coordinates
                int srcX = minX + (x * contentWidth) / ICON_SIZE;
                int srcY = minY + (y * contentHeight) / ICON_SIZE;
                
                // Clamp to content bounds
                srcX = std::min(srcX, maxX);
                srcY = std::min(srcY, maxY);
                
                // Copy pixel data
                int srcIndex = (srcY * width + srcX) * channels;
                int dstIndex = (y * ICON_SIZE + x) * channels;
                
                for (int c = 0; c < channels; c++) {
                    resizedData[dstIndex + c] = imageData[srcIndex + c];
                }
            }
        }
        
        std::cout << "Resized content (" << contentWidth << "x" << contentHeight << ") to fill " << ICON_SIZE << "x" << ICON_SIZE << " pixels" << std::endl;
        return true;
    }
    
    void convertToBitmap(int threshold = 128, bool invert = false) {
        if (!resizedData) {
            std::cerr << "Error: No resized image data" << std::endl;
            return;
        }
        
        // Calculate bytes needed for 40x40 bitmap
        int bytesPerRow = (ICON_SIZE + 7) / 8;
        bitmapData.reserve(bytesPerRow * ICON_SIZE);
        
        for (int y = 0; y < ICON_SIZE; y++) {
            unsigned char byte = 0;
            for (int x = 0; x < ICON_SIZE; x++) {
                int pixelIndex = (y * ICON_SIZE + x) * channels;
                
                // Convert to grayscale
                int gray;
                if (channels >= 3) {
                    int r = resizedData[pixelIndex];
                    int g = resizedData[pixelIndex + 1];
                    int b = resizedData[pixelIndex + 2];
                    gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);
                } else {
                    gray = resizedData[pixelIndex];
                }
                
                // Apply threshold (with optional inversion)
                bool isBlack = gray < threshold;
                if (invert) {
                    isBlack = !isBlack; // Invert the logic
                }
                
                if (isBlack) {
                    byte |= (1 << (7 - (x % 8))); // Black pixel
                }
                
                if ((x + 1) % 8 == 0 || x == ICON_SIZE - 1) {
                    bitmapData.push_back(byte);
                    byte = 0;
                }
            }
        }
        
        std::string invertStr = invert ? " (inverted)" : "";
        std::cout << "Converted to " << ICON_SIZE << "x" << ICON_SIZE << " bitmap (" << bitmapData.size() << " bytes)" << invertStr << std::endl;
    }
    
    void saveCIcon(const std::string& outputFile, const std::string& arrayName, bool invert = false) {
        if (bitmapData.empty()) {
            std::cerr << "Error: No bitmap data to save" << std::endl;
            return;
        }
        
        std::ofstream file(outputFile);
        if (!file.is_open()) {
            std::cerr << "Error: Could not create output file " << outputFile << std::endl;
            return;
        }
        
        // Write header comment
        std::string invertStr = invert ? " (inverted)" : "";
        file << "// '" << arrayName << "', " << ICON_SIZE << "x" << ICON_SIZE << "px - PocketMage icon" << invertStr << std::endl;
        file << "const unsigned char " << arrayName << " [] PROGMEM = {" << std::endl;
        
        // Write bitmap data (5 bytes per row for 40x40)
        int bytesPerRow = (ICON_SIZE + 7) / 8;
        for (int y = 0; y < ICON_SIZE; y++) {
            file << "  ";
            for (int x = 0; x < bytesPerRow; x++) {
                int byteIndex = y * bytesPerRow + x;
                file << "0x" << std::hex << std::uppercase;
                if (bitmapData[byteIndex] < 16) file << "0";
                file << (int)bitmapData[byteIndex];
                
                if (x < bytesPerRow - 1) {
                    file << ", ";
                }
            }
            if (y < ICON_SIZE - 1) {
                file << ",";
            }
            file << std::endl;
        }
        
        file << "};" << std::endl;
        file.close();
        
        std::cout << "Saved PocketMage icon to " << outputFile << std::endl;
        std::cout << "Array name: " << arrayName << std::endl;
        std::cout << "Size: " << ICON_SIZE << "x" << ICON_SIZE << " pixels (" << bitmapData.size() << " bytes)" << std::endl;
        std::cout << "Ready for integration into PocketMage assets.cpp" << std::endl;
    }
    
    void printUsage() {
        std::cout << "PocketMage Icon Converter - Converts images to 40x40 C bitmap icons" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: image_to_icon <input_image> <output_file> <array_name> [threshold] [--invert]" << std::endl;
        std::cout << "  input_image: PNG or JPEG file to convert" << std::endl;
        std::cout << "  output_file: Output C header file (.h)" << std::endl;
        std::cout << "  array_name:  Name for the C array (e.g., myAppIcon)" << std::endl;
        std::cout << "  threshold:   Grayscale threshold (0-255, default: 128)" << std::endl;
        std::cout << "  --invert:    Invert colors (white becomes black, black becomes white)" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  ./image_to_icon pokeball.png pokeball_icon.h pokeballIcon" << std::endl;
        std::cout << "  ./image_to_icon atom.jpg atom_icon.h atomIcon 100" << std::endl;
        std::cout << "  ./image_to_icon pokemon.png pokemon_icon.h pokemonIcon 150" << std::endl;
        std::cout << "  ./image_to_icon hydrogen.png hydrogen_inv.h hydrogenIcon 128 --invert" << std::endl;
        std::cout << std::endl;
        std::cout << "Features:" << std::endl;
        std::cout << "  - AUTO-CROPS: Detects content bounds and removes white space" << std::endl;
        std::cout << "  - FILLS SQUARE: Scales content to use entire 40x40 pixel area" << std::endl;
        std::cout << "  - Converts to grayscale and 1-bit monochrome" << std::endl;
        std::cout << "  - Generates exactly 200 bytes (perfect for PocketMage)" << std::endl;
        std::cout << "  - Ready for copy-paste into assets.cpp" << std::endl;
        std::cout << std::endl;
        std::cout << "Auto-Crop Details:" << std::endl;
        std::cout << "  - Finds non-white pixels (gray < 240) to determine content area" << std::endl;
        std::cout << "  - Crops to content bounds, eliminating empty borders" << std::endl;
        std::cout << "  - Scales cropped content to fill entire 40x40 icon space" << std::endl;
        std::cout << "  - Result: Maximum icon utilization with no wasted space!" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 6) {
        ImageToIcon converter;
        converter.printUsage();
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    std::string arrayName = argv[3];
    int threshold = 128;
    bool invert = false;
    
    // Parse optional arguments
    for (int i = 4; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--invert") {
            invert = true;
        } else {
            // Assume it's a threshold value
            threshold = std::atoi(argv[i]);
        }
    }
    
    // Validate threshold
    if (threshold < 0 || threshold > 255) {
        std::cerr << "Error: Threshold must be between 0 and 255" << std::endl;
        return 1;
    }
    
    ImageToIcon converter;
    
    if (!converter.loadImage(inputFile)) {
        return 1;
    }
    
    if (!converter.resizeToIcon()) {
        return 1;
    }
    
    converter.convertToBitmap(threshold, invert);
    converter.saveCIcon(outputFile, arrayName, invert);
    
    std::cout << std::endl << "Icon conversion complete!" << std::endl;
    std::cout << "Copy the generated array into Code/PocketMage_V3/src/assets.cpp" << std::endl;
    
    return 0;
}
