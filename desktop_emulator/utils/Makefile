# Makefile for Image to Bitmap Converter

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET1 = image_to_bitmap
TARGET2 = image_to_icon
SOURCE1 = image_to_bitmap.cpp
SOURCE2 = image_to_icon.cpp

# Default target - build both converters
all: $(TARGET1) $(TARGET2)

# Build the general image converter
$(TARGET1): $(SOURCE1) stb_image.h
	$(CXX) $(CXXFLAGS) -o $(TARGET1) $(SOURCE1)
	@echo "Built $(TARGET1) successfully!"

# Build the PocketMage icon converter
$(TARGET2): $(SOURCE2) stb_image.h
	$(CXX) $(CXXFLAGS) -o $(TARGET2) $(SOURCE2)
	@echo "Built $(TARGET2) successfully!"
	@echo "PocketMage Icon Converter ready!"

# Download STB libraries if not present
stb_image.h:
	@echo "STB image library not found, downloading..."
	./download_stb.sh

stb_image_resize.h:
	@echo "STB image resize library not found, downloading..."
	./download_stb_resize.sh

# Clean build artifacts
clean:
	rm -f $(TARGET1) $(TARGET2)
	@echo "Cleaned build artifacts"

# Test PocketMage icon converter with example images
test-icon: $(TARGET2)
	@echo "Testing PocketMage icon converter..."
	@if [ -f "../../Unknown.png" ]; then \
		./$(TARGET2) ../../Unknown.png pokeball_40x40.h pokeballIcon 128; \
		echo "Pokeball icon test completed - check pokeball_40x40.h"; \
	fi
	@if [ -f "../../pokemon.png" ]; then \
		./$(TARGET2) ../../pokemon.png pokemon_40x40.h pokemonIcon 128; \
		echo "Pokemon icon test completed - check pokemon_40x40.h"; \
	fi

# Install dependencies (macOS)
install-deps:
	@echo "No additional dependencies needed - using STB header-only library"

.PHONY: all clean test install-deps
