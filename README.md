# Desk Watch — MELODY & TEACHER Editions

![Desk Watch](docs/photos/main_cover.png)
![Desk Watch assembled](docs/2.jpg)

An Arduino UNO R3 desk console with Grove modules, a 16×2 RGB LCD, one button and one rotary dial. It combines climate monitoring, timekeeping and desk tools. **TEACHER Edition adds an EXAM app that combines an exam countdown and paper collection on one screen.**

## Choose your edition

| Edition | Menu | Includes |
|---|---:|---|
| **MELODY** | 10 entries | Climate display, Pomodoro timer, counter, stopwatch, alarm, dice, chess clock, Melody player and settings |
| **TEACHER** | 11 entries | All MELODY features plus **EXAM** |

Each edition ships in **two language builds: FR (French) and EN (English)**. In a given build, the LCD interface and the code comments use that single language, and there is no language choice in Settings. Keeping one language frees about 6 KB of program storage on the Arduino UNO. **Versions in other languages (Spanish, German...) and other editions will be created on request: tell us in the comments.**

| Edition | French build (FR) | English build (EN) |
|---|---|---|
| MELODY | [MELODY FR](Desk_Watch_1_MELODY_Edition_FR/Desk_Watch_1_MELODY_Edition_FR.ino) | [MELODY EN](Desk_Watch_1_MELODY_Edition_EN/Desk_Watch_1_MELODY_Edition_EN.ino) |
| TEACHER | [TEACHER FR](Desk_Watch_1_TEACHER_Edition_FR/Desk_Watch_1_TEACHER_Edition_FR.ino) | [TEACHER EN](Desk_Watch_1_TEACHER_Edition_EN/Desk_Watch_1_TEACHER_Edition_EN.ino) |

Keep each `.ino` inside its identically named folder when opening it in Arduino IDE. Earlier releases and Git history retain previous firmware versions; the four files above are the current edition sources. The embedded LCD manual has been removed.

## EXAM: countdown and paper collection

This app grew from a classroom use case: a **35-minute exam with 24 students**, using the countdown and the counter as each paper was handed in. TEACHER brings these two tasks together.

1. Choose **1–240 minutes** with the dial and click.
2. Choose **1–99 expected papers** and click.
3. Check the summary and click to start. Initial suggestions are **35 min / 24 papers**.

Example LCD layout:

```text
EXAMEN     34:59
Papers     00/24
```

Every short press counts a paper immediately. Two quick presses count two papers; a double click does not reset the count. The counter stops at the expected total.

A **long press, between 0.8 and 2.5 seconds**, opens the exam controls:

- Back to collection.
- Subtract one paper to correct an extra press.
- Pause or resume the countdown.
- Add five minutes, capped at four hours remaining.
- End the exam, with confirmation.
- Open the main menu while the exam countdown continues in the background.

At zero, two short beeps sound once and collection remains available. At the expected paper count, the screen turns green. The final message alternates with **actual elapsed working time, excluding pauses**; the paper count stays visible. Elapsed time freezes at the last expected paper or at expiry. Collection after expiry does not increase it.

```text
TOTAL      35:00
Papers     24/24
```

Ending manually shows a stable summary. Another click opens setup for the next exam. Automatic sleep is suspended during the session; the very long press still provides manual sleep. The running session is held in RAM and does not survive power loss or reset.

After the startup introduction, **TEACHER opens the menu at Favorite, 1/11**, regardless of the dial's initial position. Turning the dial enables navigation. EXAM is the second entry.

Detailed instructions: [French guide](Desk_Watch_1_TEACHER_Edition_FR/README_FR.md) · [English guide](Desk_Watch_1_TEACHER_Edition_EN/README_EN.md).

## Common features and latest changes

- **Temp & Hum:** RTC time and background letters stay on the first row. The second row shows **T** and **H**, with OK while both measurements are in range. Only threshold alerts alternate with the measurements. The clock stays visible. The RGB backlight affects the entire screen: green for OK, red for high, blue for low.
- **Alarm:** eight choices—the four Melody songs plus **Aurora, Cosmic Pulse, Arcade Hero and Epic Rise**. Melody selection previews at minimum volume. Volume selection previews at the chosen **Low / Medium / High** level. The actual alarm uses its saved volume. RTC required.
- **Counter:** the active timer or stopwatch appears at the lower left while the count stays at the right. If both run, their time displays alternate every two seconds.
- **Timer:** after starting, turn the dial to open the menu without stopping the countdown. Duration is adjustable from one minute to four hours before starting.
- **Background timer indicator:** **M**, including on the Counter page. **C** indicates a running stopwatch and **A** an armed alarm.
- **Menu order:** Timer is followed immediately by Counter. The French chess entry is explicitly labelled **Pend. echecs**.
- **Melody player:** Happy Birthday, Für Elise, Super Bros and Tetrix. The four extra alarm themes are reserved for the alarm.
- **Dice:** D2, D4, D6, D8, D10, D12, D20, D30 and D100.
- **Chess clock:** ten time controls, including 5+3.
- **Settings:** Brightness, Volume, Boot sound, Auto sleep, Night mode, Set time and Favorite. The language is fixed by the build (FR or EN).

## Menu tree

```text
Desk Watch
├── Favorite
├── Exam                  [TEACHER only]
├── Temp & Hum
├── Timer
├── Counter
├── Stopwatch
├── Alarm
├── Dice
├── Chess clock
├── Melody
└── Settings
```

The clock can also be selected as the Favorite page. The alarm and clock require the RTC; EXAM does not.

## Hardware and wiring

![Wiring diagram](docs/wiring.png)
![Components](docs/photos/desk_watch_components.jpg)

| Part | Connection |
|---|---|
| Arduino UNO R3 + Grove Base Shield V2.0 | USB for power/programming |
| Grove LCD RGB Backlight V4.0, 16×2 | I2C |
| Grove SHT31/SHT40 temperature/humidity sensor, 0x44 | I2C |
| Optional DS1307 or DS3231 RTC, 0x68 | I2C |
| Grove buzzer | D3 |
| Grove button | D4 |
| Grove rotary angle sensor | A1 |

Modules use Grove cables; no soldering is needed. LCD, SHT31/SHT40 and RTC share the I2C bus at different addresses. The current driver uses the earlier module command set at I2C 0x44; SHT40 hardware compatibility remains to be verified.

## Build and validation

Install **Grove - LCD RGB Backlight** in Arduino IDE, select **Arduino Uno**, open the desired edition and upload. The sketch reads SHT31/SHT40 and RTC directly over I2C.

The current TEACHER FR and EN builds compile for `arduino:avr:uno` using **Arduino AVR Boards 1.8.8**, **Grove - LCD RGB Backlight 1.0.2** and Arduino CLI 1.5.1. Each now uses about **80 % of the program storage** (TEACHER previously used 100 %), leaving roughly 6 KB free, and about **23 % of global RAM**. Firmware size depends on the build configuration.

EXAM has passed source-derived logic tests for setup, 35 min / 24 papers, rapid presses, pause/resume, correction, extensions, expiry, final elapsed time, end confirmation and `millis()` rollover. Startup menu selection was checked at different initial dial positions. The new EXAM app still needs a physical device check; the earlier separate timer/counter workflow was used in class.

## Existing Wokwi simulation

[Open the existing Desk Watch simulator](https://wokwi.com/projects/478193958373775361).

The existing simulation uses a DHT22 on D2 as a substitute for the physical SHT31/SHT40. It is an earlier simulator build, not verification of the current TEACHER firmware or EXAM app.

## Photos and documentation

The existing gallery is retained below. Menu photos show earlier firmware and do not represent the new 10/11-entry menus.

| Base and electronics | Enclosure |
|---|---|
| ![Base with UNO](docs/photos/desk_watch_2.jpg) | ![Assembled case](docs/photos/desk_watch_3.jpg) |
| ![UNO and shield on the base](docs/photos/desk_watch_1.jpg) | ![Top tray](docs/photos/desk_watch_4.jpg) |

![Top tray modules](docs/photos/desk_watch_5.jpg)
![Earlier menu](docs/photos/desk_watch_m1.jpg)
![Earlier timer menu](docs/photos/desk_watch_m2.jpg)
![Earlier stopwatch menu](docs/photos/desk_watch_m3.jpg)
![Earlier counter menu](docs/photos/desk_watch_m4.jpg)
![Earlier dice menu](docs/photos/desk_watch_m5.jpg)
![Earlier chess menu](docs/photos/desk_watch_m6.jpg)
![Birthday melody](docs/photos/Birthday.jpg)

See the `docs` directory for the wiring image and the guides: user guide ([English](docs/Desk_Watch_1_User_Guide.pdf) · [French](docs/Desk_Watch_1_Guide_utilisateur_FR.pdf)) and screw guide ([English](docs/Desk_Watch_Screw_Guide_EN.pdf) · [French](docs/Desk_Watch_Guide_de_visserie_FR.pdf)). The TEACHER edition guides above detail the EXAM controls.

## Project pages and enclosure

- [GitHub](https://github.com/PrBelamine/desk-watch)
- [Hackster project](https://www.hackster.io/belaminelfay/desk-watch-arduino-uno-console-melody-teacher-editions-0709c7)
- [Arduino Forum topic](https://forum.arduino.cc/t/desk-watch-an-arduino-uno-desk-console-with-9-apps-climate-alerts-pomodoro-dice-chess-clock/1464130)
- [3D-printed case and print profiles on MakerWorld](https://makerworld.com/models/3451026)

## License

Firmware: **GNU GPL v3.0**, as specified in the repository's [LICENSE](LICENSE).
