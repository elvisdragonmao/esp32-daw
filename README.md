# ESP32 WROVER Mini DAW 使用說明書

這是一台以 ESP32 WROVER 製作的 4 軌 step sequencer / mini DAW。它會在 ST7735S 80x160 TFT 上顯示選單與 16-step grid，透過 MAX98357A I2S amplifier 輸出合成音，並使用類比搖桿完成選單操作、參數調整與即時錄音。

## 快速開始

1. 確認 ST7735S、MAX98357A、搖桿與 ESP32 WROVER 已依照接線表連接。
2. 接上 USB，確認序列埠出現，例如 `/dev/cu.usbserial-0001`。
3. 編譯並上傳：

```sh
"/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" compile --upload --fqbn esp32:esp32:esp32wrover --port /dev/cu.usbserial-0001 .
```

4. 上傳完成後，ESP32 會自動 reset，畫面會進入主選單。

如果上傳卡在 `Connecting...`，按住板子上的 `BOOT` 鍵直到開始寫入 flash。

## 硬體規格

目標板子：

- ESP32 WROVER Module
- Arduino FQBN: `esp32:esp32:esp32wrover`

使用模組：

- ST7735S 80x160 TFT
- MAX98357A I2S DAC/amplifier
- 2-axis analog joystick with switch

## 接線表

| 功能 | ESP32 pin |
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

## 操作方式

搖桿用來操作所有畫面。

| 動作 | 功能 |
| --- | --- |
| 上 / 下 | 移動選單游標，或增加 / 減少數值 |
| 右 | 進入選單、確認項目、切換播放 |
| 左 | 返回上一層，或離開參數調整畫面 |

錄音模式中，搖桿方向不再控制選單，而是用來輸入音符。放開搖桿代表空拍。

## 主畫面

主畫面左側會顯示 4 軌與主要功能：

- `1 Sin`
- `2 Tri`
- `3 Sqr`
- `4 Saw`
- `Play` / `Pause`
- `Vol`
- `BPM`

右側 grid 是 4 軌、16 step 的 pattern。亮色格代表該 step 有音符。播放時白色框會顯示目前 step。

## 軌道選單

在主畫面選擇第 1 到第 4 軌後，向右進入軌道選單。

| 項目 | 功能 |
| --- | --- |
| `Mute` / `Unmut` | 靜音或取消靜音目前軌道 |
| `Record` | 進入錄音待命 |
| `Vol` | 調整目前軌道音量 |
| `OSC` | 選擇目前軌道的 oscillator |

## 音量與 BPM

`M Vol` 是 master volume，影響整體輸出音量。

`TnVol` 是單一軌道音量，只影響目前選取的軌道。

`BPM` 會調整 sequencer 速度。每個 step 是一拍，16 steps 是 16 拍循環。

## Oscillator

每條軌道可以選擇一種 oscillator：

| 顯示 | 波形 |
| --- | --- |
| `Sin` | Sine |
| `Tri` | Triangle |
| `Sqr` | Square |
| `Saw` | Saw |

## 錄音流程

1. 進入某一軌的軌道選單。
2. 選擇 `Record` 並向右確認。
3. 畫面進入 `ARM`，系統會等待 loop 回到 step 1。
4. 進入 `REC` 後，每個 step 會讀取當下搖桿方向。
5. 連續錄完 16 steps 後，自動回到軌道選單。

錄音時的音符對應：

| 搖桿方向 | 音符 |
| --- | --- |
| 左 | C |
| 上 | D |
| 右上 | E |
| 右 | F |
| 右下 | G |
| 下 | A |
| 左下 | B |
| 左上 | C+ |
| 放開 | 空拍 |

## 搖桿校正

目前搖桿原始 ADC 範圍設定在 `Config.h`：

```cpp
#define JOY_ADC_MIN 2175
#define JOY_ADC_MAX 4095
#define JOY_NORM_SCALE 1000
#define JOY_NAV_DEAD 450
#define JOY_RECORD_DEAD 450
```

開機時會自動讀取搖桿當下的靜止位置作為中心點。執行時會把 ADC 數值限制在 `2175..4095`，再轉成 `-1000..1000`，所以 dead zone 比較好調。

如果搖桿太容易誤觸，調高：

- `JOY_NAV_DEAD`
- `JOY_RECORD_DEAD`

如果搖桿太不靈敏，調低這兩個值。

## 畫面顏色

實機 ST7735S 目前顯示出來是紅藍通道互換的狀態，所以程式裡設定的橘色在螢幕上會接近藍色，亮藍色會接近土黃色。海報用 SVG 已經依照實機顏色輸出。

## 海報用 SVG

海報素材位於：

```text
poster/ui-svg/
```

包含 9 張目前 UI 狀態：

- `01-main-stopped.svg`
- `02-main-playing.svg`
- `03-track-menu.svg`
- `04-master-volume.svg`
- `05-track-volume.svg`
- `06-bpm.svg`
- `07-osc-menu.svg`
- `08-record-armed.svg`
- `09-recording.svg`

重新產生 SVG：

```sh
python3 tools/generate_ui_svgs.py
```

SVG 使用 `viewBox="0 24 160 80"` 對齊 Arduino 程式中的 TFT 可見區域，並使用內縮像素線模擬 `drawRect()`，避免右側邊框在設計軟體中被裁切。

## 編譯

如果 `arduino-cli` 在 `PATH` 裡：

```sh
arduino-cli compile --fqbn esp32:esp32:esp32wrover .
```

這台 Mac 上可直接使用 Arduino IDE 內建 CLI：

```sh
"/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" compile --fqbn esp32:esp32:esp32wrover .
```

## 上傳

列出目前連接的板子：

```sh
arduino-cli board list
```

上傳到 ESP32 WROVER：

```sh
arduino-cli compile --upload --fqbn esp32:esp32:esp32wrover --port /dev/cu.usbserial-0001 .
```

## 檔案結構

| 檔案 | 說明 |
| --- | --- |
| `sketch_jun6a.ino` | Arduino 入口、初始化順序、FreeRTOS task 建立 |
| `Config.h` | pin、畫面尺寸、音訊參數、搖桿設定 |
| `Types.h` | enum 與固定大小 struct |
| `State.h/.cpp` | 共享狀態、dirty flags、初始化 pattern |
| `Joystick.h/.cpp` | ADC 校正、濾波、dead zone、方向判斷 |
| `AudioEngine.h/.cpp` | I2S、oscillator、envelope、mixing、audio task |
| `Sequencer.h/.cpp` | step 推進、錄音寫入、voice trigger |
| `DisplayUI.h/.cpp` | TFT 繪圖與 dirty UI 更新 |
| `InputController.h/.cpp` | 搖桿事件、debounce、選單狀態機 |

## 性能設計

音訊路徑維持固定且可預期：

- 使用固定大小陣列與固定大小 audio buffer
- 不使用 `new`、`malloc`、Arduino `String`
- audio path 不使用 virtual function
- critical section 只做短時間共享狀態讀寫
- audio task pinned to core 1，priority 高於 input/UI

