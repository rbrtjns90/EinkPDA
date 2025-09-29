#!/bin/bash

# Download STB image library header
echo "Downloading STB image library..."

curl -o stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h

if [ $? -eq 0 ]; then
    echo "Successfully downloaded stb_image.h"
else
    echo "Failed to download stb_image.h"
    echo "You can manually download it from: https://github.com/nothings/stb/blob/master/stb_image.h"
fi
