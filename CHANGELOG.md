# Firmware update — 28 October 2026 (volume)

- Volume now has five levels instead of three: Min, Low, Mid, High and Max (FR: Min, Bas, Moyen, Fort, Max). The buzzer duty cycle goes from 5 % to 50 % (5, 10, 20, 35, 50 %); Max is the loudest a square wave can drive a piezo buzzer, twice the previous High level. This fixes melodies being too quiet at the top level.
- The default is Mid, the middle level, for the settings and for the alarm. The alarm volume uses the same five levels.
- A volume already saved in the EEPROM keeps its number but its meaning changes (the old High becomes Mid); use Settings > Factory reset or choose the level again.
- The user guides (English and French) describe the five levels.
- Size is about the same (MELODY about 82 % of the flash, TEACHER about 89 %; estimates, the Arduino IDE gives the exact value). Not yet tested on the device.

# Firmware update — 28 October 2026

- User guides (English and French) rewritten for MELODY and TEACHER in a single guide per language: controls, menu, every page with its screens and backlight colours, EXAM, settings and troubleshooting.
- TEACHER EN: the Exam screen now shows "EXAM" and "PAUSED" instead of the French words "EXAMEN" and "PAUSE".
- Header comments (all four sketches): the Temp & Hum status colour in range is turquoise, not green. Only a comment changes; the behaviour of MELODY and TEACHER FR is identical.

# Firmware update — 27 October 2026

- MELODY (FR and EN) is rebuilt from the JOY STUDENT base: same apps and same 10-entry menu, controlled with the button (D4) and the dial (A1) instead of a joystick.
- New in MELODY: Temp & Hum with five pages (readings, minimum, maximum, average, automatic cycle), a Sleep mode menu entry, factory reset in Settings (8 settings), an eleventh chess time control (20+0), a boot sound that is off by default, and SHT31/SHT40 auto-detection.
- The RTC module is now required for the clock, the alarm and the automatic night mode; without it, Temp & Hum keeps working and the other pages show "Install RTC module".
- Controls: dial = scroll or change value; short press = OK; double click = page action; long press = back; sleep is started from the "Sleep mode" menu entry (no very long press). Counter: each press adds one; turn the dial for −1 or reset. Alarm: short press enables and sets, double click disables; any press or turn of the dial stops the ringing.
- Timer colours: blue when ready, orange while working, yellow when paused, green for the short break, cyan for the long break; the menu shows the same colour as the page.
- Default brightness is now 85 %. The High volume level is softer (the buzzer saturated at the old level). The alarm preview now plays when choosing the ringtone (lowest volume) and the volume (chosen level).
- The menu opens on the entry of the page just left (1/10 from Temp & Hum); turning the dial then takes over.
- TEACHER is rebuilt on the new MELODY: the same apps and controls plus EXAM, 11 menu entries with Exam first (the menu opens on Exam), and Exam as a possible start-up page in Settings > Favorite. The EXAM code is unchanged; the new build is not yet tested on the device.
- Both editions now live in their own folder: `Desk_Watch_1_MELODY/` and `Desk_Watch_1_TEACHER/`, each with an FR and an EN subfolder.
- The menu photos still show the earlier firmware.

The MELODY sketches are syntax-checked with avr-gcc for the ATmega328P and tested on the device; the TEACHER sketches are syntax-checked only.

# Firmware update — 20 October 2026

- One language per build: the FR files use a French interface and the EN files an English interface. The Language choice is removed from Settings (seven settings remain). Other languages will be created on request.
- Frees about 6 KB of program storage on the Arduino UNO (TEACHER was at 100 %). Settings stored in EEPROM keep the same layout.
- SHT31 read: the CRC-8 of each 16-bit word is now checked. A corrupted read is ignored and the previous values are kept; a sensor fault is reported after three bad reads in a row. This follows a reader comment on the project page.
- Dice use a lighter random-number generator, saving about 250 bytes.

These changes compile for the four sketches (MELODY and TEACHER, FR and EN). A physical device check of the settings menu, the SHT31 read and the dice is still pending.

# Firmware update — 19 October 2026

- Current MELODY and TEACHER Editions, each with FR/EN-commented sketches.
- TEACHER adds EXAM: configurable countdown and expected papers, immediate collection counting, pause/resume, correction and extension controls, final active elapsed time excluding pauses.
- TEACHER starts on Favorite, menu 1/11, until the dial moves.
- Updated climate layout: clock always visible, measurements/alerts below.
- Eight alarm choices and live melody/volume previews; melody choice previews quietly.
- Counter displays the active timer/stopwatch; M identifies a running timer.
- Counter follows Timer in the menu; dial opens the menu after starting the timer.
- Embedded manual removed; TEACHER user guides provided as files.

The classroom example used the previous separate timer/counter workflow. The new EXAM app has passed compilation and source-derived logic tests and still needs a physical device check.

TEACHER build: Arduino AVR Boards 1.8.8, Grove LCD RGB Backlight 1.0.2, Arduino CLI 1.5.1. Program storage 32,256 bytes, global RAM 486 bytes. Existing Wokwi and menu photographs show earlier firmware.
