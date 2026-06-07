# ESP32 WROVER Mini DAW 使用說明書

這是一台以 ESP32 WROVER 製作的 4 軌 step sequencer / mini DAW。它會在 ST7735S 80x160 TFT 上顯示選單與可調長度 step grid，透過 MAX98357A I2S amplifier 輸出合成音，並使用類比搖桿完成選單操作、參數調整與即時錄音。

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

| 功能        | ESP32 pin |
| ----------- | --------- |
| TFT CS      | GPIO 5    |
| TFT DC      | GPIO 27   |
| TFT RST     | GPIO 33   |
| TFT MOSI    | GPIO 23   |
| TFT SCLK    | GPIO 18   |
| I2S BCLK    | GPIO 26   |
| I2S LRC/WS  | GPIO 25   |
| I2S DOUT    | GPIO 22   |
| I2S SD      | GPIO 21   |
| Joystick X  | GPIO 34   |
| Joystick Y  | GPIO 35   |
| Joystick SW | GPIO 32   |

## 操作方式

搖桿用來操作所有畫面。

| 動作    | 功能                            |
| ------- | ------------------------------- |
| 上 / 下 | 移動選單游標，或增加 / 減少數值 |
| 右      | 進入選單、確認項目、切換播放    |
| 左      | 返回上一層，或離開參數調整畫面  |

錄音模式中，搖桿方向不再控制選單，而是用來輸入音符。放開搖桿代表空拍。

## 主畫面

主畫面左側會顯示 4 軌與主要功能：

- `1` + sine 波形圖示
- `2` + triangle 波形圖示
- `3` + square 波形圖示
- `4` + saw 波形圖示
- `Play` / `Pause`
- `Vol`
- `BPM`
- `Bars`

右側 grid 是 4 軌 pattern。每小節 4 step，可設定 1 到 8 小節，預設 4 小節也就是 16 step。亮色格代表該 step 有音符。播放時白色框會顯示目前 step。

## 軌道選單

在主畫面選擇第 1 到第 4 軌後，向右進入軌道選單。

| 項目             | 功能                      |
| ---------------- | ------------------------- |
| `Mute` / `Unmut` | 靜音或取消靜音目前軌道    |
| `Record`         | 進入錄音待命              |
| `Vol`            | 調整目前軌道音量          |
| `OSC`            | 選擇目前軌道的 oscillator |

## 音量、BPM 與小節數

`M Vol` 是 master volume，影響整體輸出音量。

`TnVol` 是單一軌道音量，只影響目前選取的軌道。

`BPM` 會調整 sequencer 速度。每個 step 的時間是 `30000 / BPM` ms。

`Bars` 會設定 loop 小節數，範圍是 1 到 8。每小節 4 step，所以 loop 長度會是 4、8、12、16、20、24、28 或 32 step。

## Oscillator

每條軌道可以選擇一種 oscillator：

| 顯示          | 波形                                  |
| ------------- | ------------------------------------- |
| Sine 圖示     | Sine                                  |
| Triangle 圖示 | Triangle                              |
| Square 圖示   | Square                                |
| Saw 圖示      | Saw                                   |
| `Drum`        | 4 種 drum 聲音：Kick、Snare、Hat、Tom |

## 錄音流程

1. 進入某一軌的軌道選單。
2. 選擇 `Record` 並向右確認。
3. 畫面進入 `ARM`，系統會等待 loop 回到 step 1。
4. 進入 `REC` 後，每個 step 會讀取當下搖桿方向。
5. 連續錄完目前 loop 的所有 steps 後，自動回到軌道選單。

錄音時的音符對應：

如果目前軌道選的是 `Drum`，同一組方向會映射成 4 種 drum 聲音：左 / 右下是 Kick，上 / 下是 Snare，右上 / 左下是 Hat，右 / 左上是 Tom。

| 搖桿方向 | 音符 |
| -------- | ---- |
| 左       | C    |
| 上       | D    |
| 右上     | E    |
| 右       | F    |
| 右下     | G    |
| 下       | A    |
| 左下     | B    |
| 左上     | C+   |
| 放開     | 空拍 |

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

如果實機 ST7735S 出現橘色顯示成藍色、亮藍色顯示成土黃色，代表 TFT 的 RGB/BGR 色序需要在 firmware 裡修正。海報用 SVG 保持預期 UI 顏色，不用紅藍交換補償。

## 海報用 SVG

海報素材位於：

```text
poster/ui-svg/
```

包含 10 張目前 UI 狀態：

- `01-main-stopped.svg`
- `02-main-playing.svg`
- `03-track-menu.svg`
- `04-master-volume.svg`
- `05-track-volume.svg`
- `06-bpm.svg`
- `07-bars.svg`
- `08-osc-menu.svg`
- `09-record-armed.svg`
- `10-recording.svg`

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

| 檔案                     | 說明                                          |
| ------------------------ | --------------------------------------------- |
| `sketch_jun6a.ino`       | Arduino 入口、初始化順序、FreeRTOS task 建立  |
| `Config.h`               | pin、畫面尺寸、音訊參數、搖桿設定             |
| `Types.h`                | enum 與固定大小 struct                        |
| `State.h/.cpp`           | 共享狀態、dirty flags、初始化 pattern         |
| `Joystick.h/.cpp`        | ADC 校正、濾波、dead zone、方向判斷           |
| `AudioEngine.h/.cpp`     | I2S、oscillator、envelope、mixing、audio task |
| `Sequencer.h/.cpp`       | step 推進、錄音寫入、voice trigger            |
| `DisplayUI.h/.cpp`       | TFT 繪圖與 dirty UI 更新                      |
| `InputController.h/.cpp` | 搖桿事件、debounce、選單狀態機                |

## 性能設計

音訊路徑維持固定且可預期：

- 使用固定大小陣列與固定大小 audio buffer
- 不使用 `new`、`malloc`、Arduino `String`
- audio path 不使用 virtual function
- critical section 只做短時間共享狀態讀寫
- audio task pinned to core 1，priority 高於 input/UI

## Commit And PR Guidance

Use Linux kernel/Git-style commit subjects with a concrete area, subsystem, component, directory, package, or file prefix:

```
area: imperative patch summary
sub/sys: imperative patch summary
```

The prefix should name the repository area primarily changed. Prefer specific prefixes such as a directory, package, file, subsystem, or component name.

Do not use generic Conventional Commit prefixes such as `fix:`, `feat:`, `chore:`, `docs:`, or `refactor:` unless they are actual repository areas in this repository. The prefix should describe where the change belongs, not what type of change it is.

Use an imperative verb in the summary after the colon, such as `fix`, `clarify`, `split`, `validate`, `rename`, `remove`, `add`, `update`, `document`, etc. Do not use past tense verbs like `fixed` or `added`.

When a patch spans multiple areas, choose the narrowest common area if one exists. If no clear common area exists, use the primary behavior changed rather than listing multiple unrelated prefixes.

The summary after the colon should briefly describe what the patch does, because it becomes the first line shown in the git changelog. Keep it short, imperative, and specific. Prefer subjects under 72 characters. Use lowercase for the first word after the colon unless it is a proper noun, and do not end the subject with a period.

Examples:

```
storybook: clarify build ownership
web/routes: split route-level chunks
ui/field: fix select menu positioning
server/auth: validate session cookie
githooks.txt: improve the intro section
```

Use a commit body when the reason for the change is not obvious from the diff. Explain why the change is needed, not just what changed.

PRs should describe the changed area, summarize the user-visible or developer-visible impact, list validation commands run, link related issues, and include screenshots for visible web UI changes.

If no validation was run, state that explicitly and explain why. Do not claim to have run commands that were not actually executed.

For UI changes that are not easily captured in a screenshot, describe the visual change and any manual checks performed.
