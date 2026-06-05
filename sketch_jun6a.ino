#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP_I2S.h>
#include <math.h>

// ============================================================
// Pin settings
// ============================================================

// ST7735S 80x160 TFT
#define PIN_TFT_CS 27
#define PIN_TFT_DC 17  // LCD ç¬? 8 ??³ï?????è¨­æ?? DC / A0 / RS
#define PIN_TFT_RST 16
#define PIN_TFT_MOSI 23  // LCD SDA
#define PIN_TFT_SCLK 18  // LCD SCL

// MAX98357A
#define PIN_I2S_BCLK 26
#define PIN_I2S_LRC 25
#define PIN_I2S_DOUT 22
#define PIN_I2S_SD 21

// Joystick
#define PIN_JOY_X 34
#define PIN_JOY_Y 35
#define PIN_JOY_SW 32

// å¸¸è?? joystick æ¨¡ç??ï¼?å¾?ä¸???? Y ???è®?å°?ï¼????ä»¥é??è¨­å??è½? Y
#define JOY_INVERT_X false
#define JOY_INVERT_Y true

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

// 16 steps = ???å°?ç¯?ï¼?æ¯? step = ä¸????
// stepMs = 60000 / BPM

// ============================================================
// Display layout: 160x80 landscape
// ============================================================

#define SCREEN_W             160
#define SCREEN_H             80

// §Aªº ST7735 BLACKTAB ¹ê»Ú¥iµø°Ï¬O y = 24 ~ 103
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

// ============================================================
// Objects
// ============================================================

Adafruit_ST7735 tft = Adafruit_ST7735(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

I2SClass I2S;

portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

// ============================================================
// Types
// ============================================================

enum OscType : uint8_t {
  OSC_SINE = 0,
  OSC_TRIANGLE,
  OSC_SQUARE,
  OSC_SAW
};

enum ScreenMode : uint8_t {
  MODE_MAIN = 0,
  MODE_TRACK,
  MODE_MASTER_VOL,
  MODE_TRACK_VOL,
  MODE_BPM,
  MODE_OSC,
  MODE_REC_ARMED,
  MODE_RECORDING
};

enum Dir4 : uint8_t {
  DIR_CENTER = 0,
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
};

struct Step {
  int8_t note;       // -1 = empty, 0~7 = C D E F G A B C
  uint8_t velocity;  // 0~127
};

struct Track {
  Step steps[STEP_COUNT];
  bool mute;
  uint8_t volume;  // 0~127
  OscType osc;
};

struct Voice {
  bool active;
  uint8_t track;
  uint32_t phase;
  uint32_t inc;
  OscType osc;
  int32_t ageSamples;
  int32_t gateSamples;
};

// ============================================================
// Global state
// ============================================================

Track tracks[TRACK_COUNT];
Voice voices[TRACK_COUNT];

volatile bool playing = false;
volatile bool resetPlayback = false;

volatile uint8_t currentStep = 0;
volatile uint8_t previousStep = 15;

volatile uint16_t bpm = 120;
volatile uint8_t masterVolume = 100;

volatile ScreenMode screenMode = MODE_MAIN;

volatile uint8_t mainIndex = 0;
volatile uint8_t selectedTrack = 0;
volatile uint8_t trackMenuIndex = 0;
volatile uint8_t oscMenuIndex = 0;

volatile uint8_t recordTrack = 0;
volatile uint8_t recordCount = 0;
volatile int8_t currentRecNote = -1;

// Dirty rendering flags
volatile bool dirtyFull = true;
volatile bool dirtyMenu = true;
volatile bool dirtyStatus = true;
volatile bool dirtyCol[STEP_COUNT];
volatile bool dirtyCell[TRACK_COUNT][STEP_COUNT];

// Joystick calibration
int joyCenterX = 2048;
int joyCenterY = 2048;

// Sine table
int16_t sineTable[256];

// Frequencies: C D E F G A B C
const float NOTE_FREQS[8] = {
  261.63f, 293.66f, 329.63f, 349.23f,
  392.00f, 440.00f, 493.88f, 523.25f
};

const char *NOTE_NAMES[8] = {
  "C", "D", "E", "F", "G", "A", "B", "C+"
};

// Colors
uint16_t COL_BG;
uint16_t COL_PANEL;
uint16_t COL_PANEL_DARK;
uint16_t COL_TEXT;
uint16_t COL_MUTED_TEXT;
uint16_t COL_SELECT;
uint16_t COL_GRID_EMPTY;
uint16_t COL_GRID_BORDER;
uint16_t COL_NOTE;
uint16_t COL_MUTED_NOTE;
uint16_t COL_PLAYHEAD;
uint16_t COL_RECORD;

// ============================================================
// Utility
// ============================================================

const char *oscName(OscType osc) {
  switch (osc) {
    case OSC_SINE: return "Sin";
    case OSC_TRIANGLE: return "Tri";
    case OSC_SQUARE: return "Sqr";
    case OSC_SAW: return "Saw";
    default: return "?";
  }
}

uint32_t freqToInc(float freq) {
  return (uint32_t)((double)freq * 4294967296.0 / (double)SAMPLE_RATE);
}

void setDirtyFullNoLock() {
  dirtyFull = true;
}

void setDirtyStatusNoLock() {
  dirtyStatus = true;
}

void setDirtyMenuNoLock() {
  dirtyMenu = true;
}

void setDirtyCellNoLock(uint8_t t, uint8_t s) {
  if (t < TRACK_COUNT && s < STEP_COUNT) {
    dirtyCell[t][s] = true;
  }
}

void setDirtyColNoLock(uint8_t s) {
  if (s < STEP_COUNT) {
    dirtyCol[s] = true;
  }
}

void clearTrackNoLock(uint8_t t) {
  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    tracks[t].steps[s].note = -1;
    tracks[t].steps[s].velocity = 0;
    dirtyCell[t][s] = true;
  }
}

int clampInt(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ============================================================
// Init data
// ============================================================

void initSineTable() {
  for (int i = 0; i < 256; i++) {
    float phase = (float)i * 2.0f * PI / 256.0f;
    sineTable[i] = (int16_t)(sin(phase) * 32767.0f);
  }
}

void initTracks() {
  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    tracks[t].mute = false;
    tracks[t].volume = 100;
    tracks[t].osc = (OscType)t;

    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      tracks[t].steps[s].note = -1;
      tracks[t].steps[s].velocity = 0;
    }

    voices[t].active = false;
    voices[t].track = t;
    voices[t].phase = 0;
    voices[t].inc = 0;
    voices[t].osc = (OscType)t;
    voices[t].ageSamples = 0;
    voices[t].gateSamples = 0;
  }

  // Demo patternï¼???¹ä¾¿ä½?ä¸????æ©?å°±è?½è?½å?°è?²é?³ã??
  // ä¸???³è?? demo pattern ???è©±ï????????æ®µå?ªæ????³å?¯ã??
  tracks[0].steps[0].note = 0;
  tracks[0].steps[4].note = 2;
  tracks[0].steps[8].note = 4;
  tracks[0].steps[12].note = 7;

  tracks[1].steps[2].note = 4;
  tracks[1].steps[6].note = 5;
  tracks[1].steps[10].note = 4;
  tracks[1].steps[14].note = 2;

  tracks[2].steps[0].note = 0;
  tracks[2].steps[8].note = 0;

  tracks[3].steps[3].note = 7;
  tracks[3].steps[7].note = 6;
  tracks[3].steps[11].note = 5;
  tracks[3].steps[15].note = 4;
}

// ============================================================
// Joystick
// ============================================================

void calibrateJoystick() {
  delay(200);

  long sx = 0;
  long sy = 0;

  for (int i = 0; i < 100; i++) {
    sx += analogRead(PIN_JOY_X);
    sy += analogRead(PIN_JOY_Y);
    delay(2);
  }

  joyCenterX = sx / 100;
  joyCenterY = sy / 100;
}

void readJoystickDelta(int &dx, int &dy) {
  dx = analogRead(PIN_JOY_X) - joyCenterX;
  dy = analogRead(PIN_JOY_Y) - joyCenterY;

  if (JOY_INVERT_X) dx = -dx;
  if (JOY_INVERT_Y) dy = -dy;
}

Dir4 readDir4() {
  int dx, dy;
  readJoystickDelta(dx, dy);

  const int DEAD = 650;

  if (abs(dx) < DEAD && abs(dy) < DEAD) {
    return DIR_CENTER;
  }

  if (abs(dx) > abs(dy)) {
    return dx > 0 ? DIR_RIGHT : DIR_LEFT;
  } else {
    return dy > 0 ? DIR_UP : DIR_DOWN;
  }
}

int8_t readRecordNote8() {
  int dx, dy;
  readJoystickDelta(dx, dy);

  const int TH = 650;

  bool left = dx < -TH;
  bool right = dx > TH;
  bool up = dy > TH;
  bool down = dy < -TH;

  if (!left && !right && !up && !down) return -1;

  // å·¦ä??ï¼?é«? C
  if (left && up) return 7;

  // ä¸?ï¼?D
  if (up && !left && !right) return 1;

  // ??³ä??ï¼?E
  if (right && up) return 2;

  // ??³ï??F
  if (right && !up && !down) return 3;

  // ??³ä??ï¼?G
  if (right && down) return 4;

  // ä¸?ï¼?A
  if (down && !left && !right) return 5;

  // å·¦ä??ï¼?B
  if (left && down) return 6;

  // å·¦ï??ä½? C
  if (left && !up && !down) return 0;

  return -1;
}

// ============================================================
// I2S / MAX98357A
// ============================================================

void setupI2S() {
  pinMode(PIN_I2S_SD, OUTPUT);
  digitalWrite(PIN_I2S_SD, HIGH);

  // BCLK, WS/LRC, DOUT, DIN, MCLK
  // DIN ??? MCLK ä¸???¨ï?????ä»¥å¡« -1
  I2S.setPins(
    PIN_I2S_BCLK,
    PIN_I2S_LRC,
    PIN_I2S_DOUT,
    -1,
    -1);

  bool ok = I2S.begin(
    I2S_MODE_STD,
    SAMPLE_RATE,
    I2S_DATA_BIT_WIDTH_16BIT,
    I2S_SLOT_MODE_STEREO);

  if (!ok) {
    Serial.println("I2S begin failed");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("I2S OK");
}

// ============================================================
// Audio engine
// ============================================================

int16_t renderOscillator(Voice &v) {
  uint32_t ph = v.phase;

  switch (v.osc) {
    case OSC_SINE:
      {
        return sineTable[ph >> 24];
      }

    case OSC_TRIANGLE:
      {
        uint16_t p = ph >> 16;
        int32_t tri;

        if (p < 32768) {
          tri = (int32_t)p * 2 - 32768;
        } else {
          tri = 98304 - (int32_t)p * 2;
        }

        return (int16_t)tri;
      }

    case OSC_SQUARE:
      {
        return (ph & 0x80000000UL) ? -32767 : 32767;
      }

    case OSC_SAW:
      {
        return (int16_t)((int32_t)(ph >> 16) - 32768);
      }

    default:
      return 0;
  }
}

int32_t envelopeQ15(Voice &v) {
  const int32_t attackSamples = SAMPLE_RATE * 5 / 1000;
  const int32_t releaseSamples = SAMPLE_RATE * 60 / 1000;

  if (v.ageSamples < attackSamples) {
    return (v.ageSamples * 32767) / attackSamples;
  }

  if (v.ageSamples < v.gateSamples) {
    return 32767;
  }

  int32_t releaseAge = v.ageSamples - v.gateSamples;

  if (releaseAge < releaseSamples) {
    return ((releaseSamples - releaseAge) * 32767) / releaseSamples;
  }

  v.active = false;
  return 0;
}

void triggerVoice(uint8_t track, int8_t note, OscType osc, uint32_t samplesPerStep) {
  if (track >= TRACK_COUNT || note < 0 || note > 7) return;

  voices[track].active = true;
  voices[track].track = track;
  voices[track].phase = 0;
  voices[track].inc = freqToInc(NOTE_FREQS[note]);
  voices[track].osc = osc;
  voices[track].ageSamples = 0;

  int32_t gate = (int32_t)(samplesPerStep * 75 / 100);
  if (gate < 1) gate = 1;
  voices[track].gateSamples = gate;
}

int16_t renderAudioSample() {
  int32_t mix = 0;

  uint8_t localMaster = masterVolume;

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    Voice &v = voices[t];

    if (!v.active) continue;
    if (tracks[t].mute) continue;

    int16_t osc = renderOscillator(v);
    int32_t env = envelopeQ15(v);

    if (env <= 0) continue;

    int32_t sample = ((int32_t)osc * env) >> 15;
    sample = (sample * tracks[t].volume) / 127;

    mix += sample;

    v.phase += v.inc;
    v.ageSamples++;
  }

  mix = (mix * localMaster) / 127;

  // 4 è»?æ··é?³é????? headroomï¼???¿å????????
  mix /= 4;

  if (mix > 32767) mix = 32767;
  if (mix < -32768) mix = -32768;

  return (int16_t)mix;
}

void advanceStepFromAudio(uint32_t samplesPerStep) {
  uint8_t newStep;
  uint8_t oldStep;

  int8_t notesToTrigger[TRACK_COUNT];
  OscType oscToTrigger[TRACK_COUNT];
  bool muteToUse[TRACK_COUNT];

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    notesToTrigger[t] = -1;
    oscToTrigger[t] = OSC_SINE;
    muteToUse[t] = true;
  }

  portENTER_CRITICAL(&stateMux);

  oldStep = currentStep;
  currentStep = (currentStep + 1) & 0x0F;
  newStep = currentStep;
  previousStep = oldStep;

  setDirtyColNoLock(oldStep);
  setDirtyColNoLock(newStep);
  setDirtyStatusNoLock();

  if (newStep == 0) {
    setDirtyFullNoLock();
  }

  // Record armedï¼?ç­? loop ?????? step 0 ??????å§????
  if (screenMode == MODE_REC_ARMED && newStep == 0) {
    screenMode = MODE_RECORDING;
    recordCount = 0;
    clearTrackNoLock(recordTrack);
    setDirtyFullNoLock();
  }

  // Recordingï¼?æ¯???? step è®?ä¸?æ¬¡ç?®å?? joystick ??¹å??
  if (screenMode == MODE_RECORDING) {
    int8_t n = currentRecNote;

    tracks[recordTrack].steps[newStep].note = n;
    tracks[recordTrack].steps[newStep].velocity = (n >= 0) ? 100 : 0;

    setDirtyCellNoLock(recordTrack, newStep);
    setDirtyStatusNoLock();

    recordCount++;

    if (recordCount >= STEP_COUNT) {
      screenMode = MODE_TRACK;
      trackMenuIndex = 1;
      recordCount = 0;
      setDirtyFullNoLock();
    }
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    notesToTrigger[t] = tracks[t].steps[newStep].note;
    oscToTrigger[t] = tracks[t].osc;
    muteToUse[t] = tracks[t].mute;
  }

  portEXIT_CRITICAL(&stateMux);

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    if (!muteToUse[t] && notesToTrigger[t] >= 0) {
      triggerVoice(t, notesToTrigger[t], oscToTrigger[t], samplesPerStep);
    }
  }
}

void audioTask(void *param) {
  int16_t audioBuffer[AUDIO_FRAMES * 2];
  uint32_t samplesToNextStep = 1;

  while (true) {
    bool localPlaying;
    bool localReset;
    uint16_t localBpm;

    portENTER_CRITICAL(&stateMux);
    localPlaying = playing;
    localReset = resetPlayback;
    localBpm = bpm;

    if (resetPlayback) {
      resetPlayback = false;
      currentStep = 15;
      previousStep = 15;
      setDirtyFullNoLock();
    }
    portEXIT_CRITICAL(&stateMux);

    if (localReset) {
      samplesToNextStep = 0;
    }

    uint32_t samplesPerStep = ((uint64_t)SAMPLE_RATE * 60) / localBpm;
    if (samplesPerStep < 100) samplesPerStep = 100;

    for (uint16_t i = 0; i < AUDIO_FRAMES; i++) {
      if (localPlaying) {
        if (samplesToNextStep == 0) {
          advanceStepFromAudio(samplesPerStep);
          samplesToNextStep = samplesPerStep;
        }

        samplesToNextStep--;
      }

      int16_t s = renderAudioSample();

      // stereo frame: L/R ??½è¼¸??ºä??æ¨??????²é??
      audioBuffer[i * 2] = s;
      audioBuffer[i * 2 + 1] = s;
    }

    size_t bytesWritten = I2S.write((uint8_t *)audioBuffer, sizeof(audioBuffer));

    if (bytesWritten == 0) {
      taskYIELD();
    }

    taskYIELD();
  }
}

// ============================================================
// UI drawing
// ============================================================

void drawMenuItem(uint8_t row, const char *text, bool selected, bool muted = false) {
  int y = VIEW_Y + row * 10;

  uint16_t bg = selected ? COL_SELECT : COL_PANEL;
  uint16_t fg = muted ? COL_MUTED_TEXT : COL_TEXT;

  tft.fillRect(0, y, MENU_W, 10, bg);
  tft.setTextSize(1);
  tft.setTextColor(fg, bg);
  tft.setCursor(2, y + 1);
  tft.print(text);
}

void drawLeftPanel() {
  tft.fillRect(VIEW_X, VIEW_Y, MENU_W, VIEW_H, COL_PANEL);

  ScreenMode mode = screenMode;

  if (mode == MODE_MAIN) {
    char buf[12];

    for (uint8_t t = 0; t < TRACK_COUNT; t++) {
      snprintf(buf, sizeof(buf), "%d %s", t + 1, oscName(tracks[t].osc));
      drawMenuItem(t, buf, mainIndex == t, tracks[t].mute);
    }

    drawMenuItem(4, playing ? "Pause" : "Play", mainIndex == 4);
    drawMenuItem(5, "Vol", mainIndex == 5);
    drawMenuItem(6, "BPM", mainIndex == 6);
  }

  else if (mode == MODE_TRACK) {
    drawMenuItem(0, tracks[selectedTrack].mute ? "Unmut" : "Mute", trackMenuIndex == 0);
    drawMenuItem(1, "Record", trackMenuIndex == 1);
    drawMenuItem(2, "Vol", trackMenuIndex == 2);
    drawMenuItem(3, "OSC", trackMenuIndex == 3);

    char buf[10];
    snprintf(buf, sizeof(buf), "T%d", selectedTrack + 1);
    drawMenuItem(6, buf, false);
  }

  else if (mode == MODE_MASTER_VOL) {
    drawMenuItem(0, "M Vol", true);
    drawMenuItem(2, "Up +", false);
    drawMenuItem(3, "Dn -", false);
    drawMenuItem(6, "L Back", false);
  }

  else if (mode == MODE_TRACK_VOL) {
    char buf[10];
    snprintf(buf, sizeof(buf), "T%dVol", selectedTrack + 1);

    drawMenuItem(0, buf, true);
    drawMenuItem(2, "Up +", false);
    drawMenuItem(3, "Dn -", false);
    drawMenuItem(6, "L Back", false);
  }

  else if (mode == MODE_BPM) {
    drawMenuItem(0, "BPM", true);
    drawMenuItem(2, "Up +", false);
    drawMenuItem(3, "Dn -", false);
    drawMenuItem(6, "L Back", false);
  }

  else if (mode == MODE_OSC) {
    drawMenuItem(0, "Sin", oscMenuIndex == 0);
    drawMenuItem(1, "Tri", oscMenuIndex == 1);
    drawMenuItem(2, "Sqr", oscMenuIndex == 2);
    drawMenuItem(3, "Saw", oscMenuIndex == 3);

    char buf[10];
    snprintf(buf, sizeof(buf), "T%dOSC", selectedTrack + 1);
    drawMenuItem(6, buf, false);
  }

  else if (mode == MODE_REC_ARMED) {
    drawMenuItem(0, "ARM", true);
    char buf[10];
    snprintf(buf, sizeof(buf), "T%d", recordTrack + 1);
    drawMenuItem(1, buf, false);
    drawMenuItem(3, "Wait", false);
  }

  else if (mode == MODE_RECORDING) {
    drawMenuItem(0, "REC", true);
    char buf[10];
    snprintf(buf, sizeof(buf), "T%d", recordTrack + 1);
    drawMenuItem(1, buf, false);

    snprintf(buf, sizeof(buf), "%02d/16", recordCount + 1);
    drawMenuItem(3, buf, false);
  }
}

void drawGridCell(uint8_t t, uint8_t s) {
  if (t >= TRACK_COUNT || s >= STEP_COUNT) return;

  int x = GRID_X + s * CELL_W;
  int y = GRID_Y + t * ROW_H;

  bool hasNote = tracks[t].steps[s].note >= 0;
  bool muted = tracks[t].mute;

  uint16_t fillColor;

  if (hasNote && muted) {
    fillColor = COL_MUTED_NOTE;
  } else if (hasNote) {
    fillColor = COL_NOTE;
  } else {
    fillColor = COL_GRID_EMPTY;
  }

  tft.fillRect(x, y, CELL_W - 1, CELL_H, fillColor);
  tft.drawRect(x, y, CELL_W - 1, CELL_H, COL_GRID_BORDER);

  if (playing && s == currentStep) {
    tft.drawRect(x - 1, y - 1, CELL_W + 1, CELL_H + 2, COL_PLAYHEAD);
  }
}

void drawSelectedTrackOutline() {
  ScreenMode mode = screenMode;

  if (
    mode == MODE_TRACK || mode == MODE_TRACK_VOL || mode == MODE_OSC || mode == MODE_REC_ARMED || mode == MODE_RECORDING) {
    uint8_t t = selectedTrack;

    if (mode == MODE_REC_ARMED || mode == MODE_RECORDING) {
      t = recordTrack;
    }

    int y = GRID_Y + t * ROW_H;
    tft.drawRect(GRID_X - 2, y - 2, CELL_W * STEP_COUNT + 3, CELL_H + 4, COL_SELECT);
  }
}

void drawGrid() {
  tft.fillRect(GRID_X - 2, VIEW_Y, VIEW_W - GRID_X + 2, STATUS_Y - VIEW_Y - 1, COL_BG);

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      drawGridCell(t, s);
    }
  }

  drawSelectedTrackOutline();
}

void drawStepColumn(uint8_t s) {
  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    drawGridCell(t, s);
  }
  drawSelectedTrackOutline();
}

void drawBar(int x, int y, int w, int h, int value, int maxValue) {
  value = clampInt(value, 0, maxValue);

  int fillW = (w * value) / maxValue;

  tft.drawRect(x, y, w, h, COL_GRID_BORDER);
  tft.fillRect(x + 1, y + 1, w - 2, h - 2, COL_GRID_EMPTY);

  if (fillW > 2) {
    tft.fillRect(x + 1, y + 1, fillW - 2, h - 2, COL_SELECT);
  }
}

void drawStatus() {
  tft.fillRect(GRID_X - 2, STATUS_Y, VIEW_W - GRID_X + 2, STATUS_H, COL_PANEL_DARK);

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT, COL_PANEL_DARK);

  char buf[32];
  ScreenMode mode = screenMode;

  if (mode == MODE_MASTER_VOL) {
    tft.setCursor(GRID_X, STATUS_Y + 2);
    tft.print("Master Volume");
    drawBar(GRID_X, STATUS_Y + 13, 100, 8, masterVolume, 127);

    snprintf(buf, sizeof(buf), "%03d", masterVolume);
    tft.setCursor(GRID_X + 104, STATUS_Y + 13);
    tft.print(buf);
    return;
  }

  if (mode == MODE_TRACK_VOL) {
    tft.setCursor(GRID_X, STATUS_Y + 2);
    snprintf(buf, sizeof(buf), "T%d Volume", selectedTrack + 1);
    tft.print(buf);

    drawBar(GRID_X, STATUS_Y + 13, 100, 8, tracks[selectedTrack].volume, 127);

    snprintf(buf, sizeof(buf), "%03d", tracks[selectedTrack].volume);
    tft.setCursor(GRID_X + 104, STATUS_Y + 13);
    tft.print(buf);
    return;
  }

  if (mode == MODE_BPM) {
    tft.setCursor(GRID_X, STATUS_Y + 2);
    tft.print("BPM");

    tft.setCursor(GRID_X, STATUS_Y + 13);
    tft.setTextColor(COL_SELECT, COL_PANEL_DARK);
    tft.print(bpm);
    return;
  }

  if (mode == MODE_RECORDING || mode == MODE_REC_ARMED) {
    int8_t n = currentRecNote;

    tft.setCursor(GRID_X, STATUS_Y + 2);

    if (mode == MODE_REC_ARMED) {
      tft.print("ARM: wait loop");
    } else {
      snprintf(buf, sizeof(buf), "REC T%d %02d/16", recordTrack + 1, recordCount + 1);
      tft.print(buf);
    }

    tft.setCursor(GRID_X, STATUS_Y + 13);

    if (n >= 0) {
      snprintf(buf, sizeof(buf), "Note: %s", NOTE_NAMES[n]);
    } else {
      snprintf(buf, sizeof(buf), "Note: --");
    }

    tft.print(buf);
    return;
  }

  tft.setCursor(GRID_X, STATUS_Y + 2);
  snprintf(buf, sizeof(buf), "BPM%03d V%03d S%02d", bpm, masterVolume, currentStep + 1);
  tft.print(buf);

  tft.setCursor(GRID_X, STATUS_Y + 13);

  if (playing) {
    tft.print("PLAY");
  } else {
    tft.print("STOP");
  }

  tft.print("  ");

  if (mode == MODE_MAIN) {
    tft.print("MAIN");
  } else if (mode == MODE_TRACK) {
    snprintf(buf, sizeof(buf), "T%d MENU", selectedTrack + 1);
    tft.print(buf);
  } else if (mode == MODE_OSC) {
    tft.print("OSC");
  }
}

void drawFullScreen() {
  tft.fillScreen(COL_BG);
  drawLeftPanel();
  drawGrid();
  drawStatus();
}

void drawDirtyUI() {
  bool localFull;
  bool localMenu;
  bool localStatus;
  bool localCol[STEP_COUNT];
  bool localCell[TRACK_COUNT][STEP_COUNT];

  portENTER_CRITICAL(&stateMux);

  localFull = dirtyFull;
  localMenu = dirtyMenu;
  localStatus = dirtyStatus;

  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    localCol[s] = dirtyCol[s];
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      localCell[t][s] = dirtyCell[t][s];
    }
  }

  dirtyFull = false;
  dirtyMenu = false;
  dirtyStatus = false;

  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    dirtyCol[s] = false;
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      dirtyCell[t][s] = false;
    }
  }

  portEXIT_CRITICAL(&stateMux);

  if (localFull) {
    drawFullScreen();
    return;
  }

  if (localMenu) {
    drawLeftPanel();
  }

  bool gridChanged = false;

  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    if (localCol[s]) {
      drawStepColumn(s);
      gridChanged = true;
    }
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      if (localCell[t][s]) {
        drawGridCell(t, s);
        gridChanged = true;
      }
    }
  }

  if (gridChanged) {
    drawSelectedTrackOutline();
  }

  if (localStatus) {
    drawStatus();
  }
}

void uiTask(void *param) {
  while (true) {
    drawDirtyUI();
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }
}

// ============================================================
// UI state machine
// ============================================================

void handleDirEvent(Dir4 dir) {
  portENTER_CRITICAL(&stateMux);

  ScreenMode mode = screenMode;

  if (mode == MODE_MAIN) {
    if (dir == DIR_UP) {
      mainIndex = (mainIndex == 0) ? 6 : mainIndex - 1;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_DOWN) {
      mainIndex = (mainIndex + 1) % 7;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_RIGHT) {
      if (mainIndex < 4) {
        selectedTrack = mainIndex;
        screenMode = MODE_TRACK;
        trackMenuIndex = 0;
        setDirtyFullNoLock();
      }

      else if (mainIndex == 4) {
        if (!playing) {
          playing = true;
          resetPlayback = true;
        } else {
          playing = false;
        }

        setDirtyFullNoLock();
      }

      else if (mainIndex == 5) {
        screenMode = MODE_MASTER_VOL;
        setDirtyFullNoLock();
      }

      else if (mainIndex == 6) {
        screenMode = MODE_BPM;
        setDirtyFullNoLock();
      }
    }
  }

  else if (mode == MODE_TRACK) {
    if (dir == DIR_UP) {
      trackMenuIndex = (trackMenuIndex == 0) ? 3 : trackMenuIndex - 1;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_DOWN) {
      trackMenuIndex = (trackMenuIndex + 1) % 4;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_LEFT) {
      screenMode = MODE_MAIN;
      setDirtyFullNoLock();
    }

    else if (dir == DIR_RIGHT) {
      if (trackMenuIndex == 0) {
        tracks[selectedTrack].mute = !tracks[selectedTrack].mute;
        setDirtyFullNoLock();
      }

      else if (trackMenuIndex == 1) {
        recordTrack = selectedTrack;
        recordCount = 0;
        currentRecNote = -1;
        screenMode = MODE_REC_ARMED;

        if (!playing) {
          playing = true;
          resetPlayback = true;
        }

        setDirtyFullNoLock();
      }

      else if (trackMenuIndex == 2) {
        screenMode = MODE_TRACK_VOL;
        setDirtyFullNoLock();
      }

      else if (trackMenuIndex == 3) {
        screenMode = MODE_OSC;
        oscMenuIndex = tracks[selectedTrack].osc;
        setDirtyFullNoLock();
      }
    }
  }

  else if (mode == MODE_MASTER_VOL) {
    if (dir == DIR_UP) {
      masterVolume = clampInt(masterVolume + 5, 0, 127);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_DOWN) {
      masterVolume = clampInt(masterVolume - 5, 0, 127);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_LEFT || dir == DIR_RIGHT) {
      screenMode = MODE_MAIN;
      setDirtyFullNoLock();
    }
  }

  else if (mode == MODE_TRACK_VOL) {
    if (dir == DIR_UP) {
      tracks[selectedTrack].volume = clampInt(tracks[selectedTrack].volume + 5, 0, 127);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_DOWN) {
      tracks[selectedTrack].volume = clampInt(tracks[selectedTrack].volume - 5, 0, 127);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_LEFT || dir == DIR_RIGHT) {
      screenMode = MODE_TRACK;
      setDirtyFullNoLock();
    }
  }

  else if (mode == MODE_BPM) {
    if (dir == DIR_UP) {
      bpm = clampInt(bpm + 5, 40, 240);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_DOWN) {
      bpm = clampInt(bpm - 5, 40, 240);
      setDirtyStatusNoLock();
    }

    else if (dir == DIR_LEFT || dir == DIR_RIGHT) {
      screenMode = MODE_MAIN;
      setDirtyFullNoLock();
    }
  }

  else if (mode == MODE_OSC) {
    if (dir == DIR_UP) {
      oscMenuIndex = (oscMenuIndex == 0) ? 3 : oscMenuIndex - 1;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_DOWN) {
      oscMenuIndex = (oscMenuIndex + 1) % 4;
      setDirtyMenuNoLock();
    }

    else if (dir == DIR_LEFT) {
      screenMode = MODE_TRACK;
      setDirtyFullNoLock();
    }

    else if (dir == DIR_RIGHT) {
      tracks[selectedTrack].osc = (OscType)oscMenuIndex;
      screenMode = MODE_TRACK;
      setDirtyFullNoLock();
    }
  }

  // REC_ARMED / RECORDING ??????ä¸?ï¼???¹å????µæ?¿ä??è¼¸å?¥é?³ç¬¦ï¼?ä¸??????¸å?®æ??ä½????

  portEXIT_CRITICAL(&stateMux);
}

void inputTask(void *param) {
  Dir4 heldDir = DIR_CENTER;
  uint32_t holdStartMs = 0;
  uint32_t lastRepeatMs = 0;

  while (true) {
    ScreenMode mode = screenMode;

    if (mode == MODE_REC_ARMED || mode == MODE_RECORDING) {
      int8_t n = readRecordNote8();

      portENTER_CRITICAL(&stateMux);
      currentRecNote = n;
      setDirtyStatusNoLock();
      portEXIT_CRITICAL(&stateMux);

      vTaskDelay(20 / portTICK_PERIOD_MS);
      continue;
    }

    Dir4 dir = readDir4();
    uint32_t now = millis();

    if (dir == DIR_CENTER) {
      heldDir = DIR_CENTER;
      holdStartMs = 0;
      lastRepeatMs = 0;
      vTaskDelay(15 / portTICK_PERIOD_MS);
      continue;
    }

    if (dir != heldDir) {
      heldDir = dir;
      holdStartMs = now;
      lastRepeatMs = now;
      handleDirEvent(dir);
    } else {
      bool repeatable = (dir == DIR_UP || dir == DIR_DOWN);

      if (repeatable && now - holdStartMs >= 500 && now - lastRepeatMs >= 130) {
        handleDirEvent(dir);
        lastRepeatMs = now;
      }
    }

    vTaskDelay(15 / portTICK_PERIOD_MS);
  }
}

// ============================================================
// Setup / loop
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BOOT 1");

  pinMode(PIN_JOY_SW, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_JOY_X, ADC_11db);
  analogSetPinAttenuation(PIN_JOY_Y, ADC_11db);

  Serial.println("BOOT 2 joystick");
  calibrateJoystick();

  Serial.println("BOOT 3 data");
  initSineTable();
  initTracks();

  for (uint8_t s = 0; s < STEP_COUNT; s++) {
    dirtyCol[s] = false;
  }

  for (uint8_t t = 0; t < TRACK_COUNT; t++) {
    for (uint8_t s = 0; s < STEP_COUNT; s++) {
      dirtyCell[t][s] = false;
    }
  }

  Serial.println("BOOT 4 SPI");
  SPI.begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI, PIN_TFT_CS);

  Serial.println("BOOT 5 TFT init");
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  COL_BG = ST77XX_BLACK;
  COL_PANEL = tft.color565(8, 35, 45);
  COL_PANEL_DARK = tft.color565(4, 20, 28);
  COL_TEXT = tft.color565(220, 245, 255);
  COL_MUTED_TEXT = tft.color565(120, 140, 145);
  COL_SELECT = tft.color565(255, 150, 0);
  COL_GRID_EMPTY = tft.color565(25, 90, 115);
  COL_GRID_BORDER = tft.color565(70, 150, 170);
  COL_NOTE = tft.color565(80, 210, 240);
  COL_MUTED_NOTE = tft.color565(70, 80, 85);
  COL_PLAYHEAD = ST77XX_WHITE;
  COL_RECORD = ST77XX_RED;

  tft.fillScreen(COL_BG);
  tft.setTextWrap(false);

  Serial.println("BOOT 6 I2S");
  setupI2S();

  Serial.println("BOOT 7 tasks");

  xTaskCreatePinnedToCore(audioTask, "audioTask", 8192, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(inputTask, "inputTask", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(uiTask, "uiTask", 8192, NULL, 1, NULL, 0);

  Serial.println("BOOT DONE");
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}