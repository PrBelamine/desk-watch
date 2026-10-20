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
