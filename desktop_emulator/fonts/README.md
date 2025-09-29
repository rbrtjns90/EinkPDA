# PocketMage Desktop Emulator Fonts

This directory contains open-source fonts bundled with the PocketMage desktop emulator to ensure consistent cross-platform operation.

## Included Fonts

### DejaVu Sans (DejaVuSans.ttf)
- **License**: DejaVu License (based on Bitstream Vera License)
- **Usage**: Primary UI font for text rendering
- **Size**: 12pt for main text, 8pt for small text
- **Source**: https://github.com/dejavu-fonts/dejavu-fonts

### DejaVu Sans Mono (DejaVuSansMono.ttf)
- **License**: DejaVu License (based on Bitstream Vera License)
- **Usage**: Monospace font for code and fixed-width text
- **Size**: 12pt for main text, 8pt for small text
- **Source**: https://github.com/dejavu-fonts/dejavu-fonts

## Font Loading Priority

The emulator loads fonts in the following order:

1. **Bundled fonts** (this directory) - Ensures consistent cross-platform appearance
2. **System fonts** (fallback) - macOS, Linux, and Windows system font paths

## License Information

The DejaVu fonts are licensed under a free license that allows redistribution and modification. The full license text is available at:
https://dejavu-fonts.github.io/License.html

### Key License Points:
- ✅ **Free to use** for any purpose
- ✅ **Free to redistribute** 
- ✅ **Free to modify**
- ✅ **No attribution required** (but appreciated)
- ✅ **Commercial use allowed**

## Technical Details

- **Format**: TrueType Font (.ttf)
- **Encoding**: Unicode (full international character support)
- **Quality**: High-quality hinting for crisp rendering at small sizes
- **Coverage**: Extensive character set including Latin, Cyrillic, Greek, and more

## Cross-Platform Compatibility

These fonts ensure the PocketMage emulator displays text consistently across:
- **macOS** (all versions)
- **Linux** (all distributions)
- **Windows** (all versions)

No additional font installation required by end users.
