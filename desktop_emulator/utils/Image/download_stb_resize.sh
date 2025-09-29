#!/bin/bash

# Download STB image resize library header
echo "Downloading STB image resize library..."

curl -o stb_image_resize.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_resize.h

if [ $? -eq 0 ]; then
    echo "Successfully downloaded stb_image_resize.h"
else
    echo "Failed to download stb_image_resize.h"
    echo "You can manually download it from: https://github.com/nothings/stb/blob/master/stb_image_resize.h"
fi
