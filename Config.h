#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// Pin settings
// ============================================================

// ST7735S 80x160 TFT
#define PIN_TFT_CS 5
#define PIN_TFT_DC 27
#define PIN_TFT_RST 33
#define PIN_TFT_MOSI 23
#define PIN_TFT_SCLK 18
#define TFT_ROTATION 1
#define TFT_USE_BGR_COLOR_ORDER 1

// MAX98357A
#define PIN_I2S_BCLK 26
#define PIN_I2S_LRC 25
#define PIN_I2S_DOUT 22
#define PIN_I2S_SD 21

// Joystick
#define PIN_JOY_X 34
#define PIN_JOY_Y 35
#define PIN_JOY_SW 32

// Common joystick modules report a smaller Y value when pushed up.
#define JOY_INVERT_X false
#define JOY_INVERT_Y true

// Joystick anti-noise tuning
#define JOY_ADC_MIN 2175
#define JOY_ADC_MAX 4095
#define JOY_NORM_SCALE 1000
#define JOY_ANALOG_SAMPLES 4
#define JOY_FILTER_WEIGHT 4
#define JOY_NAV_DEAD 450
#define JOY_RECORD_DEAD 450
#define JOY_AXIS_MARGIN 120
#define JOY_DIR_STABLE_MS 45
#define JOY_REC_STABLE_MS 40

// ============================================================
// Audio settings
// ============================================================

#define SAMPLE_RATE 22050
#define AUDIO_FRAMES 128

// ============================================================
// Sequencer settings
// ============================================================

#define TRACK_COUNT 4
#define STEP_COUNT 16

// 16 steps = 4 bars, one step per beat.
// stepMs = 60000 / BPM

// ============================================================
// Display layout: 160x80 landscape
// ============================================================

#define SCREEN_W             160
#define SCREEN_H             80

// 24 ~ 103
#define VIEW_X               0
#define VIEW_Y               24
#define VIEW_W               160
#define VIEW_H               80

#define MENU_W               44

#define GRID_X               (VIEW_X + 47)
#define GRID_Y               (VIEW_Y + 3)

#define CELL_W               7
#define CELL_H               9
#define ROW_H                13

#define STATUS_Y             (VIEW_Y + 56)
#define STATUS_H             24

#endif
