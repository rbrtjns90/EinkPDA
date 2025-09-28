# E-Ink Simulation

The desktop emulator includes a comprehensive e-ink display simulation that mimics the behavior of real e-ink panels, providing a more realistic development and testing environment.

## Features

### Full Refresh Simulation
- **Latency**: ~450ms (tunable)
- **Flash Effect**: Optional white→black flash (configurable)
- **Ghosting**: Very light ghosting retention (tunable)
- **Trigger**: Automatically triggered when >22% of pixels change, or manually via F6

### Partial Refresh Simulation
- **Latency**: ~150ms (tunable)
- **Detection**: Automatic dirty-rect detection
- **Animation**: Vertical wipe in stripes (tunable width)
- **Ghosting**: Stronger ghosting retention (tunable)
- **Optimization**: Only updates changed regions

## Runtime Controls

The simulation can be controlled during runtime using function keys:

- **F5** — Toggle e-ink simulation ON/OFF
- **F6** — Force full refresh on next frame
- **F7** — Cycle partial ghosting level (0 → 0.04 → 0.08 → 0.12 → 0)
- **F8** — Toggle wipe stripe width (18px ↔ 10px)

## Environment Variable

Set the environment variable to automatically enable simulation on startup:

```bash
export POCKETMAGE_EINK_SIM=1
```

Or run directly:

```bash
POCKETMAGE_EINK_SIM=1 ./build-macos/PocketMage_Desktop_Emulator
```

## Configuration Parameters

The simulation behavior can be tuned by modifying the `EInkSimConfig` struct in `desktop_display_sdl2.cpp`:

```cpp
struct EInkSimConfig {
    bool  enabled          = false; // runtime toggle; F5
    bool  flash_full       = true;  // white/black flashes on full refresh
    bool  flash_partial    = false; // real panels usually don't flash on partial
    int   full_ms          = 450;   // 400–600 ms typical
    int   partial_ms       = 150;   // 120–200 ms typical
    float ghosting_full    = 0.02f; // faint retention after full
    float ghosting_partial = 0.08f; // stronger retention on partial
    float full_threshold   = 0.22f; // >22% pixels changed ⇒ full refresh
    int   wipe_step_px     = 18;    // stripe width for partial wipe
    int   diff_threshold   = 24;    // 0..255; change threshold to count as "dirty"
};
```

## Technical Implementation

### Dirty Rectangle Detection
The simulation analyzes pixel changes to determine:
- Which pixels have changed beyond the threshold
- The bounding rectangle of all changes
- The percentage of total pixels that changed

### Refresh Type Selection
- **Full Refresh**: Triggered when >22% of pixels change or manually via F6
- **Partial Refresh**: Used for smaller changes, updates only the dirty rectangle

### Ghosting Simulation
Ghosting effects are simulated by blending previous and current pixel values:
```cpp
float out = (1.0f - ghosting) * current_pixel + ghosting * previous_pixel;
```

### Performance
- When simulation is disabled (F5), falls back to fast direct rendering
- Maintains backward compatibility with existing code
- Minimal performance impact when disabled

## Usage Examples

### Enable simulation and test different modes:
```bash
# Start with simulation enabled
POCKETMAGE_EINK_SIM=1 ./build-macos/PocketMage_Desktop_Emulator

# During runtime:
# Press F5 to toggle simulation on/off
# Press F6 to force a full refresh
# Press F7 to cycle through ghosting levels
# Press F8 to change stripe width
```

### Development workflow:
1. Develop your application with simulation disabled for fast iteration
2. Enable simulation (F5) to test realistic e-ink behavior
3. Use F6 to test full refresh scenarios
4. Adjust ghosting levels (F7) to see retention effects
5. Test different update patterns with F8

## Notes

- The simulation provides a realistic approximation of e-ink behavior
- Actual e-ink panels may vary in timing and ghosting characteristics
- Use this simulation to optimize your application's refresh patterns
- Consider the user experience implications of full vs partial refreshes
