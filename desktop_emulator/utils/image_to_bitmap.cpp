#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class ImageToBitmap {
private:
    int width, height, channels;
    unsigned char* imageData;
    std::vector<unsigned char> bitmapData;
    
public:
    ImageToBitmap() : imageData(nullptr), width(0), height(0), channels(0) {}
    
    ~ImageToBitmap() {
        if (imageData) {
            stbi_image_free(imageData);
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
    
    void convertToGrayscaleAndBitmap(int threshold = 128) {
        if (!imageData) {
            std::cerr << "Error: No image data loaded" << std::endl;
            return;
        }
        
        // Calculate bytes needed for bitmap (1 bit per pixel, rounded up to byte boundary)
        int bytesPerRow = (width + 7) / 8;
        int totalBytes = bytesPerRow * height;
        bitmapData.resize(totalBytes, 0);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixelIndex = (y * width + x) * channels;
                
                // Convert to grayscale using standard weights
                int gray;
                if (channels >= 3) {
                    // RGB or RGBA
                    int r = imageData[pixelIndex];
                    int g = imageData[pixelIndex + 1];
                    int b = imageData[pixelIndex + 2];
                    gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);
                } else {
                    // Already grayscale
                    gray = imageData[pixelIndex];
                }
                
                // Convert to 1-bit (black/white) using threshold
                bool isBlack = gray < threshold;
                
                if (isBlack) {
                    int byteIndex = y * bytesPerRow + x / 8;
                    int bitIndex = 7 - (x % 8);  // MSB first
                    bitmapData[byteIndex] |= (1 << bitIndex);
                }
            }
        }
        
        std::cout << "Converted to " << width << "x" << height << " bitmap (" << totalBytes << " bytes)" << std::endl;
    }
    
    void saveCBitmap(const std::string& outputFile, const std::string& arrayName) {
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
        file << "// '" << arrayName << "', " << width << "x" << height << "px bitmap" << std::endl;
        file << "const unsigned char " << arrayName << " [] PROGMEM = {" << std::endl;
        
        // Write bitmap data
        int bytesPerRow = (width + 7) / 8;
        for (int y = 0; y < height; y++) {
            file << "  ";
            for (int x = 0; x < bytesPerRow; x++) {
                int byteIndex = y * bytesPerRow + x;
                file << "0x" << std::hex << std::uppercase;
                if (bitmapData[byteIndex] < 16) file << "0";
                file << (int)bitmapData[byteIndex];
                
                if (byteIndex < bitmapData.size() - 1) {
                    file << ", ";
                }
            }
            if (y < height - 1) {
                file << ",";
            }
            file << std::endl;
        }
        
        file << "};" << std::endl;
        file.close();
        
        std::cout << "Saved C bitmap to " << outputFile << std::endl;
        std::cout << "Array name: " << arrayName << std::endl;
        std::cout << "Size: " << width << "x" << height << " pixels (" << bitmapData.size() << " bytes)" << std::endl;
    }
    
    void printUsage() {
        std::cout << "Usage: image_to_bitmap <input_image> <output_file> <array_name> [threshold]" << std::endl;
        std::cout << "  input_image: PNG or JPEG file to convert" << std::endl;
        std::cout << "  output_file: Output C header file (.h)" << std::endl;
        std::cout << "  array_name:  Name for the C array" << std::endl;
        std::cout << "  threshold:   Grayscale threshold (0-255, default: 128)" << std::endl;
        std::cout << std::endl;
        std::cout << "Example: ./image_to_bitmap pokeball.png pokeball.h pokeballIcon 128" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        ImageToBitmap converter;
        converter.printUsage();
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    std::string arrayName = argv[3];
    int threshold = (argc == 5) ? std::atoi(argv[4]) : 128;
    
    // Validate threshold
    if (threshold < 0 || threshold > 255) {
        std::cerr << "Error: Threshold must be between 0 and 255" << std::endl;
        return 1;
    }
    
    ImageToBitmap converter;
    
    // Load image
    if (!converter.loadImage(inputFile)) {
        return 1;
    }
    
    // Convert to bitmap
    converter.convertToGrayscaleAndBitmap(threshold);
    
    // Save C bitmap
    converter.saveCBitmap(outputFile, arrayName);
    
    std::cout << "Conversion complete!" << std::endl;
    return 0;
}
