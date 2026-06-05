# ESP32 WROVER Mini DAW Sequencer

這是一個跑在 ESP32 WROVER 上的 4 軌 step sequencer / mini DAW。它使用 ST7735 TFT 顯示選單與 16-step grid，透過 MAX98357A I2S amplifier 輸出合成音，並用類比搖桿操作選單與錄音。

## Hardware

目標板子：

- ESP32 WROVER Module
- Arduino FQBN: `esp32:esp32:esp32wrover`

主要模組：

- ST7735S 80x160 TFT
- MAX98357A I2S DAC/amplifier
- 2-axis analog joystick with switch

## Pin Map

| Function | Pin |
| --- | --- |
| TFT CS | GPIO 5 |
| TFT DC | GPIO 27 |
| TFT RST | GPIO 33 |
| TFT MOSI | GPIO 23 |
| TFT SCLK | GPIO 18 |
| I2S BCLK | GPIO 26 |
| I2S LRC/WS | GPIO 25 |
| I2S DOUT | GPIO 22 |
| I2S SD | GPIO 21 |
| Joystick X | GPIO 34 |
| Joystick Y | GPIO 35 |
| Joystick SW | GPIO 32 |

## Project Structure

| File | Purpose |
| --- | --- |
| `sketch_jun6a.ino` | Arduino entry point, setup order, FreeRTOS task creation |
| `Config.h` | Pins, display layout, audio constants, joystick tuning |
| `Types.h` | Shared enums and fixed-size structs |
| `State.h/.cpp` | Shared state, dirty flags, note tables, initialization |
| `Joystick.h/.cpp` | ADC calibration, filtering, dead zone, direction mapping |
| `AudioEngine.h/.cpp` | I2S setup, oscillators, envelope, mixing, audio task |
| `Sequencer.h/.cpp` | Step advancement, recording writes, voice triggering |
| `DisplayUI.h/.cpp` | TFT drawing and dirty UI refresh |
| `InputController.h/.cpp` | Joystick event debounce and menu state machine |

## Build

If `arduino-cli` is in your `PATH`:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32wrover .
```

On this Mac, Arduino IDE's bundled CLI is available at:

```sh
"/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" compile --fqbn esp32:esp32:esp32wrover .
```

## Upload

List connected boards:

```sh
arduino-cli board list
```

Upload to the ESP32 WROVER. Replace the port if your system shows a different one:

```sh
arduino-cli compile --upload --fqbn esp32:esp32:esp32wrover --port /dev/cu.usbserial-0001 .
```

If upload stalls at `Connecting...`, hold the board's `BOOT` button until flashing starts.

## Joystick Calibration

The joystick ADC range is configured in `Config.h`:

```cpp
#define JOY_ADC_MIN 2175
#define JOY_ADC_MAX 4095
#define JOY_NORM_SCALE 1000
#define JOY_NAV_DEAD 450
#define JOY_RECORD_DEAD 450
```

At boot, the current resting value is measured as the center point. Runtime readings are clamped to the configured ADC range and normalized to `-1000..1000`, so dead zone tuning is stable even when the raw ADC range is asymmetric.

If the joystick still moves too easily, increase `JOY_NAV_DEAD` and `JOY_RECORD_DEAD`. If it feels too slow to respond, lower them.

## Performance Notes

The audio path is designed to stay deterministic:

- Fixed-size arrays for tracks, voices, and audio buffers
- No `new`, `malloc`, or Arduino `String`
- No virtual dispatch in the audio path
- Short critical sections that copy shared state, then do heavier work outside the lock
- Audio task pinned to core 1 with higher priority than input/UI

