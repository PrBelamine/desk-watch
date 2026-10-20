// Desk Watch 1 — TEACHER Edition: Exam / Temp & Hum / Timer / Counter / Stopwatch / Alarm / Dice / Chess / Melody / Clock
// Hardware : Arduino UNO + Grove Base Shield, LCD RGB V4.0 (I2C),
//            SHT31 (I2C 0x44), buzzer D3, button D4, potentiometer (dial) A1,
//            optional RTC DS1307 / DS3231 (I2C 0x68).
// Required library: "Grove - LCD RGB Backlight" (rgb_lcd.h).
// SHT31 and RTC are read directly over I2C: no other library needed.
//
// All functions run in the background (humidity, alarm, pomodoro, stopwatch).
//
// Navigation:
//   Long press: open the MENU; during an EXAM session, open its controls; inside the MENU, go back to the page
//   Home                  : combined Temp & Hum / RTC time; stable when OK, alternates only for alerts; short press = MENU
//   Menu: Favorite, Exam, Temp & Hum, Timer, Counter, Stopwatch, Alarm, Dice, Chess clock, Melody, Settings
//   Settings: brightness, volume, startup sound, auto sleep, night mode, set time, favorite
//   Night mode   : No / Auto (22:00 to 06:00, with RTC) / Yes : screen dimmed to 8 %, no alert beeps, melody muted ; the alarm still rings and displays normally
//   Dial: turn right = next value / entry
//   Settings             : dial = choose, short press = open / confirm
//   Startup              : DESK WATCH, short melody, PR BELAMINE MAKERLAB, Teacher Edition loading bar, module check
//   MENU                 : dial = scroll, short press = enter
//
// Functions:
//   Exam: 1-240 minutes, 1-99 papers; short press counts a paper, including after time is up.
//         Long press opens controls: back, subtract a paper, pause/resume, +5 min, confirm ending, main menu.
//         Countdown continues in the background; automatic sleep is disabled throughout the session.
//   Temp & Hum   : row 1 RTC time and background badges, always visible; row 2 temperature, humidity and OK.
//                  Alerts replace row 2 every 2 s (measurements / temperature alert / measurements / humidity alert).
//                  RGB colours the whole screen according to the displayed alert.
//                  TOO HIGH (red) / TOO LOW (blue) / OK (green) ; double click = set the 4 thresholds
//                  (T low, T high, H low, H high) with the dial, short press = next then confirm ;
//                  thresholds are saved and reused at startup
//   Counter      : each short press = +1 (immediate) ; dial = ask for a reset (OK = yes, double click or 5 s = no)
//   Dice         : short press = roll (tick for 1 s then result), dial = number of sides (2, 4, 6, 8, 10, 12, 20, 30, 100), double click = menu
//   Music        : dial = choose a melody, short press = play / pause / resume, double click = menu
//   Chess        : two-player clock ; dial = time control (1+0 to 60+0, 5+3...), short press = start then next player, double click = pause / reset / menu
//   Alarm (menu) : short press = set hours, minutes, armed, melody, alarm volume; double click = arm / disarm
//                  Dial chooses each value; click advances and saves after volume. Melody repeats until stopped / snoozed.
//                  Melody and alarm-volume settings play a live preview; leaving either setting stops the preview.
//                  Melody selection previews at minimum volume; volume selection previews at the chosen alarm volume.
//                  Eight alarm choices: the four Music tracks plus Aurora, Cosmic Pulse, Arcade Hero and Epic Rise.
//   Pomodoro     : short press = start / pause, double click = reset (then, if already at zero, back to the menu),
//                  when stopped, the potentiometer sets the work duration (1 min to 4 h)
//   Stopwatch    : short press = start / stop, double click = reset, then (already at zero) double click or dial = menu
//   Set time (Settings) : potentiometer = value, short press = next then confirm, long press = cancel
//   Brightness (Settings) : potentiometer = level (5 to 100 %), short press = confirm, long press = cancel
//   Ringing      : short press or double click = snooze, long press = stop
//   Very long press (more than 2.5 s) : go to sleep (screen off, alarm active) ; while asleep, a press wakes it up
//   Auto sleep   : delay set in Settings (never, 1, 2, 5, 10, 15, 30 min, 1 h) after the last press or dial turn ;
//                        no auto sleep during a chess game in progress
//   UNO LED (pin 13) : on while asleep and when the button is held for more than 2.5 s
//   A short press is confirmed 0.3 s after release (double-click delay).
//   Letters at the top right : ! temperature / humidity alert, A alarm armed, C stopwatch running,
//                              M timer running on every page (last column, blinks off for 1 s every 10 s)

#include <Wire.h>
#include <EEPROM.h>
#include <rgb_lcd.h>

// Repeated flash strings are shared without changing any translation.
static const char TXT_EMPTY_0[] PROGMEM = "";
static const char TXT_12s_OK_1[] PROGMEM = "%-12s OK";
static const char TXT_s_laeuft_2[] PROGMEM = "%s laeuft";
static const char TXT_s_running_3[] PROGMEM = "%s running";
static const char TXT_u_min_4[] PROGMEM = "%u min";
static const char TXT_12s_5[] PROGMEM = "> %-12s";
static const char TXT_Auto_22_6h_6[] PROGMEM = "Auto 22-6h";
static const char TXT_Brightness_7[] PROGMEM = "Brightness";
static const char TXT_Brillo_8[] PROGMEM = "Brillo";
static const char TXT_Capteur_absent_9[] PROGMEM = "Capteur absent";
static const char TXT_D_3u_3u_10[] PROGMEM = "D%-3u        %3u";
static const char TXT_DESK_WATCH_11[] PROGMEM = "DESK WATCH";
static const char TXT_FIN_12[] PROGMEM = "FIN";
static const char TXT_Helligkeit_13[] PROGMEM = "Helligkeit";
static const char TXT_JA_14[] PROGMEM = "JA";
static const char TXT_Luminosite_15[] PROGMEM = "Luminosite";
static const char TXT_Menu_principal_16[] PROGMEM = "Menu principal";
static const char TXT_NEIN_17[] PROGMEM = "NEIN";
static const char TXT_NO_18[] PROGMEM = "NO";
static const char TXT_NON_19[] PROGMEM = "NON";
static const char TXT_OK_snooze_5_min_20[] PROGMEM = "OK=snooze 5 min";
static const char TXT_OK_start_21[] PROGMEM = "OK=start";
static const char TXT_OUI_22[] PROGMEM = "OUI";
static const char TXT_PAUSE_23[] PROGMEM = "PAUSE";
static const char TXT_Pause_24[] PROGMEM = "Pause";
static const char TXT_SI_25[] PROGMEM = "SI";
static const char TXT_Sensor_OK_26[] PROGMEM = "Sensor OK";
static const char TXT_Sensor_ausente_27[] PROGMEM = "Sensor ausente";
static const char TXT_Sensor_fehlt_28[] PROGMEM = "Sensor fehlt";
static const char TXT_Sensor_missing_29[] PROGMEM = "Sensor missing";
static const char TXT_TIMER_d_30[] PROGMEM = "TIMER    #%d";
static const char TXT_Volume_31[] PROGMEM = "Volume";
#define TR_PTR(fr, en, es, de) (en)

// ---------- Settings ----------
const uint8_t PIN_BUZZER = 3;
const uint8_t PIN_BUTTON = 4;
const uint8_t PIN_LED    = 13;         // UNO L LED ; for a real red LED, plug in a Grove LED module and change this number
const unsigned long LED_HOLD_MS = 2500;   // the LED turns on when the button is held long enough for sleep
const uint8_t PIN_POT    = A1;
const uint8_t BTN_ACTIVE = HIGH;          // Grove module: HIGH = pressed
const int  POT_MAX   = 1023;              // 675 if the shield switch is set to 3.3 V

// Alert thresholds (as in the V6 station): value >= high threshold = TOO HIGH, <= low threshold = TOO LOW.
// Default values ; adjustable by double click on the Temp & Hum page and saved.
const uint8_t T_LO_DEFAULT2 = 40, T_HI_DEFAULT2 = 56;   // temperature in half-degrees: 20.0 and 28.0 C
const uint8_t H_LO_DEFAULT  = 40, H_HI_DEFAULT  = 60;   // humidity in %
const uint8_t T_MIN2 = 20, T_MAX2 = 80, T_GAP2 = 2;     // range 10 to 40 C in 0.5 C steps, minimum gap 1 C
const uint8_t H_MIN  = 10, H_MAX  = 90, H_GAP  = 5;     // range 10 to 90 %, minimum gap 5 points
const int T_MARGE10 = 3, H_MARGE10 = 20;                 // anti-flicker margin around a threshold (in tenths: 0.3 C and 2 %)
const int TEMP_OFFSET10 = 0, HUM_OFFSET10 = 0;           // correction in tenths after comparison with a reference

const unsigned long SNOOZE_MS  = 5UL * 60000UL;
const unsigned long RING_MAX_S = 120;     // maximum ringing time before automatic snooze
const uint8_t NIGHT_START = 22, NIGHT_END = 6;   // dimmed screen

const uint8_t SHORT_BREAK_MIN = 5, LONG_BREAK_MIN = 15, LONG_EVERY = 4;
// work durations: 1 to 60 min in 1 min steps, then 65 min to 4 h in 5 min steps
const uint8_t N_WORK = 96;
uint8_t workValue(uint8_t i) { return i < 60 ? i + 1 : 60 + (i - 59) * 5; }

const uint8_t SHT_ADDR = 0x44, RTC_ADDR = 0x68;

const uint8_t BRIGHT_LEVELS[8] = { 5, 10, 20, 30, 50, 70, 85, 100 };   // % of the backlight

// ---------- State ----------
rgb_lcd lcd;

enum { M_TH, M_POMO, M_CHRONO, M_TALLY, M_DIE, M_CHESS, M_MUSIC, M_PARAMS, M_HOME, M_ALARM, M_SETCLOCK, M_BRIGHT, M_MENU, M_CLOCK, M_EXAM };
const uint8_t N_MENU = 11;
const uint8_t N_PAR = 7;   // visible settings (the language choice was removed: one language per edition)
uint8_t mode = M_HOME, prevMode = M_HOME;   // M_* values
uint8_t menuIdx = 0;
uint8_t brightBackup = 100, brightIdx = 7;

enum { EV_NONE, EV_SHORT, EV_LONG, EV_VLONG, EV_DOUBLE, EV_PRESS };

struct Settings { uint8_t magic, aH, aM, armed, workMin, bright, tLo2, tHi2, hLo, hHi, sound, lang, volume, home, night, sleep; } cfg;   // lang: 0 French, 1 English, 2 Spanish, 3 German ; home: page opened by Home (0 auto, 1 Temp & Hum, 2 clock, 3 timer, 4 stopwatch, 5 counter) ; night: night mode (0 no, 1 auto by time of day, 2 yes) ; sleep: auto sleep delay (index into SLEEP_MIN, 0 = never)

// text of this edition (one language per build: the file name gives it), in flash memory
#define TR(fr, en, es, de) PSTR(en)

// clock
bool rtcPresent = false;
unsigned long swSecs = 0, swRefMs = 0;
uint8_t nowH = 0, nowM = 0, nowS = 0;

// sensor
int temp10 = 0, hum10 = 0;      // temperature (tenths of C) and humidity (tenths of %): no floating-point math, to save memory
bool shtOk = false;
uint8_t tState = 0, hState = 0;   // 0 OK, 1 TOO HIGH, 2 TOO LOW

// alarm
uint8_t clockSub = 0;       // Set time page: 1 hours, 2 minutes
uint8_t alarmSub = 0;       // 0 view, 1 hours, 2 minutes, 3 armed, 4 melody, 5 volume
// Separate EEPROM extension preserves the existing settings and their addresses.
struct AlarmSettings { uint8_t magic, melody, volume; } alarmCfg;
uint8_t alarmPos = 0;
unsigned long alarmNoteEnd = 0;
bool alarmPreview = false;
uint8_t previewMelody = 0, previewVolume = 0;
uint8_t parSub = 0, parBackup = 0;   // setting open? value before editing
uint8_t parIdx = 1;          // Settings page (internal ids): 1 brightness, 2 volume, 3 startup sound, 4 auto sleep, 5 night mode, 6 set time, 7 favorite
unsigned int tallyCount = 0;  // hand tally counter
uint8_t tallySub = 0;         // 0 counting, 1 reset confirmation
unsigned long tallyT0 = 0;
unsigned long tapMs = 0;      // time of the last short press release
bool tapFlag = false;         // a short press has just been released (counts immediately, without waiting for the double-click delay)
// dice: possible sides (2 = coin flip, 30 = pick a student from the class)
const uint8_t N_DIE = 9;
static const uint8_t DIE_FACES[N_DIE] PROGMEM = { 2, 4, 6, 8, 10, 12, 20, 30, 100 };
uint8_t dieIdx = 2, dieVal = 0, dieShow = 0;   // chosen sides (D6 at start), last result, value displayed during the roll
bool dieRolling = false;
unsigned long dieEnd = 0;
// chess clock: time controls (minutes, increment in seconds)
const uint8_t N_CHESS = 10;
static const uint8_t CHESS_MIN[N_CHESS] PROGMEM = { 1, 3, 3, 5, 5, 10, 10, 15, 30, 60 };
static const uint8_t CHESS_INC[N_CHESS] PROGMEM = { 0, 0, 2, 0, 3, 0, 5, 10, 0, 0 };
uint8_t chessIdx = 4;                 // 5+3 at start
uint8_t chessState = 0;               // 0 waiting, 1 running, 2 paused, 3 time out
uint8_t chessSide = 0;                // player whose clock is running (0 or 1)
long chessBase[2];                    // time left for each player at the start of their turn (ms)
unsigned long chessTurn = 0;          // time the turn started
long chessLeft(uint8_t sd) {
  long v = chessBase[sd];
  if (chessState == 1 && sd == chessSide) v -= (long)(millis() - chessTurn);
  return v < 0 ? 0 : v;
}
// music notes (Hz) and lengths (x 10 ms), kept in flash
const uint8_t N_BDAY = 25;
static const uint16_t BD_NOTE[N_BDAY] PROGMEM = {
  349, 349, 392, 349, 466, 440,
  349, 349, 392, 349, 523, 466,
  349, 349, 698, 587, 466, 440, 392,
  622, 622, 587, 466, 523, 466 };
static const uint8_t BD_LEN[N_BDAY] PROGMEM = {
  35, 20, 60, 60, 60, 120,
  35, 20, 60, 60, 60, 120,
  35, 20, 60, 60, 60, 75, 100,
  35, 20, 60, 60, 60, 120 };
const uint8_t N_ELISE = 34;
static const uint16_t ELISE_NOTE[N_ELISE] PROGMEM = {
  659, 622, 659, 622, 659, 494, 587, 523, 440,
  262, 330, 440, 494, 330, 415, 494, 523,
  659, 622, 659, 622, 659, 494, 587, 523, 440,
  262, 330, 440, 494, 330, 523, 494, 440 };
static const uint8_t ELISE_LEN[N_ELISE] PROGMEM = {
  18, 18, 18, 18, 18, 18, 18, 18, 45,
  18, 18, 18, 45, 18, 18, 18, 45,
  18, 18, 18, 18, 18, 18, 18, 18, 45,
  18, 18, 18, 45, 18, 18, 18, 60 };
const uint8_t N_MARIO = 45;
static const uint16_t MARIO_NOTE[N_MARIO] PROGMEM = { 659, 659, 0, 659, 0, 523, 659, 784, 0, 392, 0, 523, 392, 0, 330, 0, 392, 0, 523, 0, 440, 494, 466, 440, 392, 659, 784, 880, 698, 784, 0, 659, 523, 587, 494, 0, 523, 0, 392, 0, 440, 494, 466, 440, 392 };
static const uint8_t MARIO_LEN[N_MARIO] PROGMEM = { 15, 15, 15, 15, 15, 15, 15, 30, 30, 15, 30, 45, 15, 30, 45, 30, 30, 15, 15, 15, 15, 15, 30, 30, 30, 30, 30, 45, 15, 30, 15, 15, 15, 15, 30, 15, 15, 15, 30, 15, 15, 15, 15, 15, 45 };
const uint8_t N_TETRIS = 36;
static const uint16_t TETRIS_NOTE[N_TETRIS] PROGMEM = { 659, 494, 523, 587, 523, 494, 440, 440, 523, 659, 587, 523, 494, 523, 587, 659, 523, 440, 440, 0, 0, 587, 698, 880, 784, 698, 659, 523, 659, 587, 523, 494, 494, 523, 587, 659 };
static const uint8_t TETRIS_LEN[N_TETRIS] PROGMEM = { 42, 21, 21, 42, 21, 21, 42, 21, 21, 42, 21, 21, 63, 21, 42, 42, 42, 42, 42, 42, 21, 42, 21, 42, 42, 21, 21, 42, 21, 21, 42, 42, 21, 21, 42, 42 };
const uint8_t N_MUSIC = 4;
static const uint16_t* const MUSIC_NOTES[N_MUSIC] PROGMEM = {
  BD_NOTE, ELISE_NOTE, MARIO_NOTE, TETRIS_NOTE };
static const uint8_t* const MUSIC_LENGTHS[N_MUSIC] PROGMEM = {
  BD_LEN, ELISE_LEN, MARIO_LEN, TETRIS_LEN };
static const uint8_t MUSIC_NOTE_COUNTS[N_MUSIC] PROGMEM = {
  N_BDAY, N_ELISE, N_MARIO, N_TETRIS };
static const char MUSIC_NAMES[] PROGMEM =
  "Birthday\0"
  "Fur Elise\0"
  "Mario Bros\0"
  "Tetris\0";
// Original alarm-only themes (Hz / duration in 10 ms units). Zero is a rest.
// Aurora: flowing major arpeggios, then a bright upper-register resolution.
static const uint16_t AURORA_NOTE[] PROGMEM = {
  523, 659, 784, 1047, 988, 784, 659, 0,
  587, 740, 880, 1175, 1047, 880, 740, 0,
  659, 784, 1047, 1319, 1175, 988, 1047, 0 };
static const uint8_t AURORA_LEN[] PROGMEM = {
  18, 18, 18, 36, 18, 18, 36, 12,
  18, 18, 18, 36, 18, 18, 36, 12,
  18, 18, 24, 36, 18, 18, 72, 24 };
// Cosmic Pulse: syncopated minor pulses and a cascading answer.
static const uint16_t COSMIC_NOTE[] PROGMEM = {
  440, 0, 880, 659, 0, 523, 659, 0,
  440, 0, 988, 784, 0, 659, 523, 0,
  587, 0, 1175, 880, 0, 698, 880, 0,
  1047, 988, 880, 784, 659, 523, 440, 0 };
static const uint8_t COSMIC_LEN[] PROGMEM = {
  12, 6, 24, 12, 6, 12, 24, 12,
  12, 6, 24, 12, 6, 12, 24, 12,
  12, 6, 24, 12, 6, 12, 24, 12,
  12, 12, 12, 12, 18, 18, 48, 24 };
// Arcade Hero: short upbeat calls, octave jumps and a victory flourish.
static const uint16_t ARCADE_NOTE[] PROGMEM = {
  784, 0, 784, 988, 1175, 0, 1568, 1175,
  1319, 0, 1175, 988, 1047, 0, 784, 0,
  880, 1047, 1319, 0, 1175, 988, 1568, 0,
  1319, 1175, 988, 1175, 1568, 0, 1568, 0 };
static const uint8_t ARCADE_LEN[] PROGMEM = {
  12, 6, 12, 12, 24, 6, 18, 12,
  24, 6, 12, 12, 24, 6, 24, 12,
  12, 12, 24, 6, 12, 12, 24, 12,
  12, 12, 12, 12, 36, 12, 48, 24 };
// Epic Rise: dotted fanfare, a rising sequence and a sustained final high note.
static const uint16_t EPIC_NOTE[] PROGMEM = {
  392, 0, 392, 523, 659, 0, 784, 659,
  587, 0, 587, 740, 880, 0, 1175, 880,
  659, 784, 988, 1175, 784, 988, 1175, 1397,
  1047, 0, 1319, 0, 1568, 1319, 1568, 0 };
static const uint8_t EPIC_LEN[] PROGMEM = {
  27, 9, 12, 24, 36, 12, 36, 18,
  27, 9, 12, 24, 36, 12, 36, 18,
  12, 12, 12, 24, 12, 12, 12, 24,
  36, 12, 36, 12, 36, 18, 72, 24 };
static_assert(sizeof(AURORA_NOTE) / sizeof(uint16_t) == sizeof(AURORA_LEN), "Aurora note/length mismatch");
static_assert(sizeof(COSMIC_NOTE) / sizeof(uint16_t) == sizeof(COSMIC_LEN), "Cosmic note/length mismatch");
static_assert(sizeof(ARCADE_NOTE) / sizeof(uint16_t) == sizeof(ARCADE_LEN), "Arcade note/length mismatch");
static_assert(sizeof(EPIC_NOTE) / sizeof(uint16_t) == sizeof(EPIC_LEN), "Epic note/length mismatch");
const uint8_t N_ALARM_TUNES = N_MUSIC + 4;
static const uint16_t* const ALARM_NOTES[N_ALARM_TUNES] PROGMEM = {
  BD_NOTE, ELISE_NOTE, MARIO_NOTE, TETRIS_NOTE, AURORA_NOTE, COSMIC_NOTE, ARCADE_NOTE, EPIC_NOTE };
static const uint8_t* const ALARM_LENGTHS[N_ALARM_TUNES] PROGMEM = {
  BD_LEN, ELISE_LEN, MARIO_LEN, TETRIS_LEN, AURORA_LEN, COSMIC_LEN, ARCADE_LEN, EPIC_LEN };
static const uint8_t ALARM_NOTE_COUNTS[N_ALARM_TUNES] PROGMEM = {
  N_BDAY, N_ELISE, N_MARIO, N_TETRIS,
  sizeof(AURORA_LEN), sizeof(COSMIC_LEN), sizeof(ARCADE_LEN), sizeof(EPIC_LEN) };
static const char ALARM_EXTRA_NAMES[4][13] PROGMEM = {
  "Aurora", "Cosmic Pulse", "Arcade Hero", "Epic Rise" };

PGM_P packedText(PGM_P text, uint8_t index);
PGM_P alarmTuneName() {
  return alarmCfg.melody < N_MUSIC ? packedText(MUSIC_NAMES, alarmCfg.melody)
                                 : ALARM_EXTRA_NAMES[alarmCfg.melody - N_MUSIC];
}

uint8_t musicTrack = 0;
uint8_t musicState = 0, musicPos = 0;  // 0 ready, 1 playing, 2 paused ; next note to play
int musicRemain = 0;                  // time left on the current note (ms)
unsigned int musicFreq = 0;
unsigned long musicNoteEnd = 0;
uint8_t thSub = 0;          // threshold setting: 0 view, 1 T low, 2 T high, 3 H low, 4 H high
uint8_t edH = 0, edM = 0;
bool ringing = false, snoozing = false;
bool standby = false;       // sleep: screen off, alarm still active
static const uint8_t SLEEP_MIN[8] PROGMEM = { 0, 1, 2, 5, 10, 15, 30, 60 };   // auto sleep delays (minutes, 0 = never)
unsigned long lastActivity = 0;   // last press or dial movement
int lastPotSample = 0;
unsigned long lastPotCheck = 0;
unsigned long btnHeld = 0;        // duration of the current press (ms), 0 if the button is released
unsigned long ringStart = 0, snoozeUntil = 0;
int lastFiredMin = -1;

// pomodoro
uint8_t pomoPhase = 0;      // 0 work, 1 short break, 2 long break
uint8_t pomoCount = 0;
bool pomoRun = false;
unsigned long pomoRemMs = 0, pomoLast = 0;

// stopwatch
bool chronoRun = false;
unsigned long chronoStart = 0, chronoAccum = 0;
unsigned long chronoMs() { return chronoRun ? millis() - chronoStart : chronoAccum; }

// potentiometer / display
int potAnchor = 0; bool potActive = false;
uint8_t lastR = 255, lastG = 255, lastB = 255;
unsigned long lastDraw = 0, lastClockRead = 0, lastSht = 0;

// ---------- Utilities ----------
uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

// smoothed reading: average of 8 samples (the first one, skewed by the multiplexer, is ignored)
int potRead() {
  analogRead(PIN_POT);
  long sum = 0;
  for (uint8_t i = 0; i < 8; i++) sum += analogRead(PIN_POT);
  int v = (int)(sum / 8);
  if (v > POT_MAX) v = POT_MAX;
  return POT_MAX - v;                              // reversed direction: turning right = increasing value
}
// choice among n values from the dial, with hysteresis around the boundaries
uint8_t potSelect(uint8_t n, uint8_t cur, int margin) {
  const int zone = (POT_MAX + 1) / n;
  int raw = potRead();
  int target = raw / zone;
  if (target > n - 1) target = n - 1;
  if (target > cur && raw >= target * zone + margin) return target;
  if (target < cur && raw <= (int)cur * zone - margin) return target;
  return cur;
}
uint8_t workIndex(uint8_t m) {
  if (m < 1) return 0;
  if (m <= 60) return m - 1;
  uint8_t i = 59 + (m - 60 + 2) / 5;
  return i > N_WORK - 1 ? N_WORK - 1 : i;
}
// "25 min" up to 60 minutes, then "1h05"
void fmtDuration(char* b, size_t n, unsigned m) {
  if (m <= 60) snprintf_P(b, n, TXT_u_min_4, m);
  else         snprintf_P(b, n, PSTR("%uh%02u"), m / 60, m % 60);
}
void potReset() { potAnchor = potRead(); potActive = false; }
bool potLive() {
  int d = potRead() - potAnchor;
  if (d < 0) d = -d;
  if (!potActive && d > 30) potActive = true;
  return potActive;
}

// night mode: active if selected (Yes) or, in automatic mode, between NIGHT_START and NIGHT_END (needs the RTC module)
bool nightActive() {
  if (cfg.night == 2) return true;
  return cfg.night == 1 && rtcPresent && (nowH >= NIGHT_START || nowH < NIGHT_END);
}
bool nightPrev = false;

// applies the requested colour, scaled by the chosen brightness (and dimmed at night, except while ringing)
void applyColor() {
  uint8_t k = (nightActive() && !ringing) ? 8 : 100;
  nightPrev = nightActive();
  lcd.setRGB((uint32_t)lastR * cfg.bright * k / 10000,
             (uint32_t)lastG * cfg.bright * k / 10000,
             (uint32_t)lastB * cfg.bright * k / 10000);
}

void color(uint8_t r, uint8_t g, uint8_t b) {
  if (r == lastR && g == lastG && b == lastB) return;
  lastR = r; lastG = g; lastB = b;
  applyColor();
}

// Write sixteen characters from RAM or flash, padding with spaces.
__attribute__((noinline, noclone)) void writeLcdLine(uint8_t row, const char* s, bool flash) {
  lcd.setCursor(0, row);
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t c = flash ? pgm_read_byte(s) : *s;
    if (c) s++; else c = ' ';
    lcd.write(c);
  }
}
void line(uint8_t row, const char* s) { writeLcdLine(row, s, false); }
void lineP(uint8_t row, PGM_P s) { writeLcdLine(row, s, true); }

__attribute__((noinline)) void drawConfirmHint() {
  lineP(1, TR("OK=oui  dbl=non", "OK=yes  dbl=no", "OK=si   dbl=no", "OK=ja  dbl=nein"));
}

// Zero-separated flash strings avoid the padding of fixed-width text tables.
PGM_P packedText(PGM_P text, uint8_t index) {
  while (index--) text += strlen_P(text) + 1;
  return text;
}

PGM_P volumeName(uint8_t volume) {
  return volume == 0 ? TR("Faible", "Low", "Bajo", "Leise") : volume == 1 ? TR("Moyen", "Medium", "Medio", "Mittel") : TR("Fort", "High", "Alto", "Laut");
}

void musicToneStop() {
  TCCR2A = 0;
  TCCR2B = 0;
  PORTD &= ~_BV(PD3);
}

void musicToneStartAtVolume(unsigned int freq, uint8_t volume) {
  static const uint8_t DUTY[3] = { 5, 20, 50 };
  unsigned long count = F_CPU / (256UL * freq);
  if (count < 1) count = 1;
  if (count > 256) count = 256;
  musicToneStop();
  OCR2A = count - 1;
  OCR2B = count * DUTY[volume] / 100;
  TCNT2 = 0;
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS22) | _BV(CS21);
}

void musicToneStart(unsigned int freq) { musicToneStartAtVolume(freq, cfg.volume); }

__attribute__((noinline)) unsigned int playTrackNote(uint8_t track, uint8_t pos, uint8_t volume, unsigned int &freq) {
  const uint16_t* notes = (const uint16_t*)pgm_read_word(&MUSIC_NOTES[track]);
  const uint8_t* lengths = (const uint8_t*)pgm_read_word(&MUSIC_LENGTHS[track]);
  freq = pgm_read_word(&notes[pos]);
  if (freq) musicToneStartAtVolume(freq, volume); else musicToneStop();
  return pgm_read_byte(&lengths[pos]) * 10;
}

void stopAlarmPreview() {
  if (!alarmPreview) return;
  musicToneStop();
  alarmPreview = false;
}

// Shared by the real alarm and its preview; no delay blocks the button or dial.
void alarmMelodyTick(unsigned long now, uint8_t volume) {
  if ((long)(now - alarmNoteEnd) < 0) return;
  musicToneStop();
  uint8_t count = pgm_read_byte(&ALARM_NOTE_COUNTS[alarmCfg.melody]);
  if (alarmPos >= count) { alarmPos = 0; alarmNoteEnd = now + 700; }
  else {
    const uint16_t* notes = (const uint16_t*)pgm_read_word(&ALARM_NOTES[alarmCfg.melody]);
    const uint8_t* lengths = (const uint8_t*)pgm_read_word(&ALARM_LENGTHS[alarmCfg.melody]);
    unsigned int freq = pgm_read_word(&notes[alarmPos]);
    alarmNoteEnd = now + pgm_read_byte(&lengths[alarmPos]) * 10UL;
    if (freq) musicToneStartAtVolume(freq, volume);
    alarmPos++;
  }
}

void updateAlarmPreview() {
  if (alarmSub != 4 && alarmSub != 5) { stopAlarmPreview(); return; }
  uint8_t volume = alarmSub == 4 ? 0 : alarmCfg.volume;
  if (!alarmPreview || previewMelody != alarmCfg.melody || previewVolume != volume) {
    musicToneStop();
    alarmPos = 0; alarmNoteEnd = millis();
    previewMelody = alarmCfg.melody; previewVolume = volume;
    alarmPreview = true;
  }
  alarmMelodyTick(millis(), volume);
}

// Buzzer sound: square wave at the requested frequency ; the volume sets the duty cycle
// (5 %, 20 % or 50 %: the shorter it is, the quieter the piezo buzzer). Blocking for 'ms'.
void buzz(unsigned int freq, unsigned int ms) {
  if (alarmPreview) musicToneStop();   // let notification beeps temporarily take over the pin
  static const uint8_t DUTY[3] = { 5, 20, 50 };
  unsigned long period = 1000000UL / freq;
  unsigned long high = period * DUTY[cfg.volume] / 100;
  unsigned long end = millis() + ms;
  while ((long)(millis() - end) < 0) {
    digitalWrite(PIN_BUZZER, HIGH); delayMicroseconds(high);
    digitalWrite(PIN_BUZZER, LOW);  delayMicroseconds(period - high);
  }
}

void beep(uint8_t n, unsigned int ms) {
  for (uint8_t i = 0; i < n; i++) {
    buzz(2300, ms);
    delay(ms);
  }
}

const unsigned long DOUBLE_MS = 300;       // maximum delay between two presses for a double click

uint8_t readButton() {
  static bool down = false, pending = false, second = false, musicPressHandled = false;
  static unsigned long t0 = 0, tRel = 0;
  bool p = (digitalRead(PIN_BUTTON) == BTN_ACTIVE);
  unsigned long now = millis();
  btnHeld = (p && down) ? now - t0 : 0;

  // an isolated short press is confirmed once the double-click delay has elapsed
  if (pending && !down && !p && now - tRel > DOUBLE_MS) {
    pending = false;
    if (musicPressHandled) { musicPressHandled = false; if (!ringing) return EV_NONE; }
    return EV_SHORT;
  }

  if (p && !down) {
    down = true; t0 = now;
    second = pending && (now - tRel <= DOUBLE_MS);
    if (mode == M_MUSIC && !ringing && !second) { musicPressHandled = true; return EV_PRESS; }
  } else if (!p && down) {
    down = false;
    unsigned long d = now - t0;
    if (d < 30) { second = false; musicPressHandled = false; return EV_NONE; }
    if (d < 800) { tapFlag = true; tapMs = now; }
    if (d < 800) {
      if (second) { pending = false; second = false; musicPressHandled = false; return EV_DOUBLE; }
      pending = true; tRel = now;
      return EV_NONE;
    }
    pending = false; second = false; musicPressHandled = false;
    return d < 2500 ? EV_LONG : EV_VLONG;
  }
  return EV_NONE;
}

// ---------- EEPROM configuration ----------
void loadCfg() {
  EEPROM.get(0, cfg);
  if (cfg.magic != 0xA9 || cfg.aH > 23 || cfg.aM > 59 || cfg.armed > 1 ||
      cfg.workMin < 1 || cfg.workMin > 240) {
    cfg.magic = 0xA9; cfg.aH = 7; cfg.aM = 0; cfg.armed = 0; cfg.workMin = 25;
    cfg.bright = 100; cfg.sound = 1; cfg.volume = 2; cfg.home = 0; cfg.night = 1; cfg.sleep = 0; cfg.tLo2 = 0xFF;       // thresholds are re-checked below
  }
  if (cfg.sleep > 7) cfg.sleep = 0;                     // auto sleep: off by default
  if (cfg.night > 2) cfg.night = 1;                     // night mode: automatic by default
  if (cfg.home > 5) cfg.home = 0;                       // Favorite: automatic by default
  if (cfg.volume > 2) cfg.volume = 2;                   // maximum volume by default
  if (cfg.sound > 1) cfg.sound = 1;                    // startup sound: on by default
  // thresholds: default values if missing or inconsistent
  if (cfg.tLo2 < T_MIN2 || cfg.tHi2 > T_MAX2 || cfg.tHi2 < cfg.tLo2 + T_GAP2 ||
      cfg.hLo < H_MIN || cfg.hHi > H_MAX || cfg.hHi < cfg.hLo + H_GAP) {
    cfg.tLo2 = T_LO_DEFAULT2; cfg.tHi2 = T_HI_DEFAULT2;
    cfg.hLo = H_LO_DEFAULT;   cfg.hHi = H_HI_DEFAULT;
  }
  if (cfg.bright < 1 || cfg.bright > 100) cfg.bright = 100;   // old save or invalid value
  EEPROM.get(sizeof(cfg), alarmCfg);
  if (alarmCfg.magic != 0xB1 || alarmCfg.melody >= N_ALARM_TUNES || alarmCfg.volume > 2) {
    alarmCfg.magic = 0xB1; alarmCfg.melody = 0; alarmCfg.volume = cfg.volume;
  }
}
void saveCfg() { EEPROM.put(0, cfg); EEPROM.put(sizeof(cfg), alarmCfg); }

// ---------- Clock ----------
void readClock() {
  if (rtcPresent) {
    Wire.beginTransmission(RTC_ADDR);
    Wire.write((uint8_t)0);
    if (Wire.endTransmission() == 0 && Wire.requestFrom(RTC_ADDR, (uint8_t)3) == 3) {
      uint8_t s = Wire.read();
      uint8_t m = Wire.read();
      uint8_t h = Wire.read();
      nowS = bcd2dec(s & 0x7F);
      nowM = bcd2dec(m & 0x7F);
      nowH = bcd2dec(h & 0x3F);
    }
  } else {
    unsigned long t = (swSecs + (millis() - swRefMs) / 1000UL) % 86400UL;
    nowH = t / 3600; nowM = (t / 60) % 60; nowS = t % 60;
  }
}

void setClock(uint8_t h, uint8_t m) {
  if (rtcPresent) {
    Wire.beginTransmission(RTC_ADDR);
    Wire.write((uint8_t)0);
    Wire.write((uint8_t)0);          // seconds = 0 (also clears the CH bit of the DS1307)
    Wire.write(dec2bcd(m));
    Wire.write(dec2bcd(h));          // 24 h mode
    Wire.endTransmission();
  } else {
    swSecs = (unsigned long)h * 3600UL + (unsigned long)m * 60UL;
    swRefMs = millis();
  }
  readClock();
}

// ---------- SHT31 ----------
// Small 16-bit xorshift generator: replaces random()/randomSeed() and saves about 250 bytes of flash
static uint16_t rngS = 1;
static void seedRng(uint32_t s) { rngS = (uint16_t)(s ^ (s >> 16)); if (!rngS) rngS = 1; }
static uint16_t rollDie(uint8_t faces) {
  rngS ^= rngS << 7; rngS ^= rngS >> 9; rngS ^= rngS << 8;
  return 1 + rngS % faces;
}

// CRC-8 (polynomial 0x31, init 0xFF) used by the SHT31 to protect each 16-bit word
uint8_t crc8(const uint8_t *d, uint8_t n) {
  uint8_t c = 0xFF;
  while (n--) {
    c ^= *d++;
    for (uint8_t i = 0; i < 8; i++) c = (c & 0x80) ? (c << 1) ^ 0x31 : (c << 1);
  }
  return c;
}

bool readSht() {
  Wire.beginTransmission(SHT_ADDR);
  Wire.write((uint8_t)0x24); Wire.write((uint8_t)0x00);   // high repeatability measurement
  if (Wire.endTransmission() != 0) return false;
  delay(20);
  if (Wire.requestFrom(SHT_ADDR, (uint8_t)6) != 6) return false;
  uint8_t b[6];
  for (uint8_t i = 0; i < 6; i++) b[i] = Wire.read();
  static uint8_t crcBad = 0;
  if (crc8(b, 2) != b[2] || crc8(b + 3, 2) != b[5]) {   // corrupted read: keep the previous values
    return ++crcBad < 3 ? shtOk : false;                // report a sensor fault after 3 bad reads in a row
  }
  crcBad = 0;
  uint16_t rt = ((uint16_t)b[0] << 8) | b[1];
  uint16_t rh = ((uint16_t)b[3] << 8) | b[4];
  temp10 = (int)((1750L * rt) / 65535L) - 450 + TEMP_OFFSET10;
  hum10  = (int)((1000L * rh) / 65535L) + HUM_OFFSET10;
  if (hum10 < 0) hum10 = 0;
  if (hum10 > 1000) hum10 = 1000;
  return true;
}

// updates a state with a margin ; returns true if it has just become TOO HIGH or TOO LOW
bool updateState(uint8_t &st, int x, int lo, int hi, int marge) {
  uint8_t old = st;
  if (x >= hi)                                st = 1;
  else if (x <= lo)                           st = 2;
  else if (st == 1 && x >= hi - marge)        { }
  else if (st == 2 && x <= lo + marge)        { }
  else                                        st = 0;
  return st != old && st != 0;
}
void alertBeep(uint8_t st) {
  if (ringing || standby || nightActive()) return;   // silence while asleep and at night
  if (st == 1) beep(2, 120); else beep(1, 700);   // high: two short beeps, low: one long beep
}
void updateAlerts() {
  if (updateState(tState, temp10, cfg.tLo2 * 5, cfg.tHi2 * 5, T_MARGE10))   alertBeep(tState);
  if (updateState(hState, hum10, cfg.hLo * 10, cfg.hHi * 10, H_MARGE10))   alertBeep(hState);
}
bool anyAlert() { return tState != 0 || hState != 0; }

// ---------- Alarm ----------
void startRing() {
  if (alarmPreview) { stopAlarmPreview(); alarmSub = 0; }   // real alarm takes priority
  if (musicState == 1) {
    long remaining = (long)(musicNoteEnd - millis());
    musicRemain = remaining > 0 ? remaining : 0;
    musicState = 2;                     // resume manually after the alarm
  }
  musicToneStop();
  alarmPos = 0; alarmNoteEnd = millis();
  if (standby) { standby = false; lcd.clear(); }   // the alarm turns the screen back on
  ringing = true; snoozing = false;
  ringStart = millis();
  lastR = 255;                          // forces the colour update
}
void stopRing() {
  ringing = false; snoozing = false;
  lastR = 255;
  musicToneStop();
}

void checkAlarm() {
  if (ringing || !rtcPresent) return;   // no alarm without a reliable clock
  int minuteOfDay = nowH * 60 + nowM;
  if (!cfg.armed) snoozing = false;
  if (snoozing) {
    if ((long)(millis() - snoozeUntil) >= 0) startRing();
    return;
  }
  if (cfg.armed && nowH == cfg.aH && nowM == cfg.aM && lastFiredMin != minuteOfDay) {
    lastFiredMin = minuteOfDay;
    startRing();
  }
}

void handleRing(uint8_t e, bool draw) {
  unsigned long now = millis();
  unsigned long el = (now - ringStart) / 1000UL;

  if (e == EV_SHORT || e == EV_DOUBLE || el >= RING_MAX_S) {
    musicToneStop();
    ringing = false; snoozing = true; snoozeUntil = now + SNOOZE_MS;
    lastR = 255;
    return;
  }
  if (e == EV_LONG || e == EV_VLONG) { stopRing(); return; }

  alarmMelodyTick(now, alarmCfg.volume);

  if (draw) {
    color(255, el < 30 ? 90 : 0, 0);
    char b[17];
    snprintf_P(b, sizeof(b), TR("REVEIL  %02d:%02d", "ALARM   %02d:%02d", "ALARMA  %02d:%02d", "WECKER  %02d:%02d"), nowH, nowM);
    line(0, b);
    lineP(1, TR_PTR(TXT_OK_snooze_5_min_20, TXT_OK_snooze_5_min_20, PSTR("OK=repetir 5 min"), PSTR("OK=Snooze 5 min")));
  }
}

// ---------- Pomodoro ----------
unsigned long phaseMs(uint8_t p) {
  if (p == 0) return (unsigned long)cfg.workMin * 60000UL;
  if (p == 1) return (unsigned long)SHORT_BREAK_MIN * 60000UL;
  return (unsigned long)LONG_BREAK_MIN * 60000UL;
}
bool pomoIdle() {
  return !pomoRun && pomoPhase == 0 && pomoCount == 0 && pomoRemMs == phaseMs(0);
}
void pomoReset() {
  pomoPhase = 0; pomoCount = 0; pomoRun = false; pomoRemMs = phaseMs(0);
}
void pomoNextPhase() {
  if (pomoPhase == 0) {
    pomoCount++;
    pomoPhase = (pomoCount % LONG_EVERY == 0) ? 2 : 1;
    beep(3, 150);
  } else {
    pomoPhase = 0;
    beep(2, 300);
  }
  pomoRemMs = phaseMs(pomoPhase);
}
void pomoTick() {
  unsigned long now = millis();
  if (pomoRun) {
    unsigned long dt = now - pomoLast;
    if (dt >= pomoRemMs) pomoNextPhase();
    else pomoRemMs -= dt;
  }
  pomoLast = millis();
}

// ---------- Screens ----------
// ---------- Navigation ----------
// values of the Settings page items (2 volume, 3 sound, 4 auto sleep, 5 night mode, 7 favorite)
uint8_t parValue(uint8_t i) { return i == 2 ? cfg.volume : i == 3 ? cfg.sound : i == 4 ? cfg.sleep : i == 5 ? cfg.night : cfg.home; }
void setParValue(uint8_t i, uint8_t v) { if (i == 2) cfg.volume = v; else if (i == 3) cfg.sound = v; else if (i == 4) cfg.sleep = v; else if (i == 5) cfg.night = v; else cfg.home = v; }

void chessPause() {                    // leaving the page pauses the clock
  if (chessState == 1) { chessBase[chessSide] -= (long)(millis() - chessTurn); chessState = 2; }
}

// EXAM: setup, immediate paper counting and a separate control menu.
enum { EX_DURATION, EX_TARGET, EX_READY, EX_RUNNING, EX_PAUSED, EX_ELAPSED, EX_FINISHED };
uint8_t examState = EX_DURATION, examMinutes = 35, examTarget = 24, examCopies = 0;
uint8_t examSub = 0, examAction = 0;
unsigned long examRemaining = 0, examLast = 0;
unsigned long examElapsed = 0;   // Active working time, excluding pauses; frozen after the last paper or expiry.

bool examSession() { return examState >= EX_RUNNING; }


void enterMode(uint8_t m) {
  stopAlarmPreview();
  if (mode == M_CHESS && m != M_CHESS) chessPause();
  if (mode == M_MUSIC && m != M_MUSIC) musicToneStop();
  mode = m;
  clockSub = 0; alarmSub = 0; thSub = 0; parIdx = 1; parSub = 0; tallySub = 0;
  if (m == M_DIE) dieRolling = false;
  if (m == M_MUSIC) { musicState = 0; musicPos = 0; musicRemain = 0; }
  potReset();
  if (m == M_SETCLOCK) { clockSub = 1; edH = nowH; edM = nowM; }
  if (m == M_BRIGHT) {
    brightBackup = cfg.bright;
    brightIdx = 0;
    for (uint8_t i = 1; i < 8; i++)
      if (abs((int)BRIGHT_LEVELS[i] - cfg.bright) < abs((int)BRIGHT_LEVELS[brightIdx] - cfg.bright)) brightIdx = i;
  }
  potReset();
  lastDraw = 0;
}

// Home: opens the page chosen in Settings (0 = automatic)
void goHome() {
  uint8_t h = cfg.home;
  if (h == 2 && !rtcPresent) h = 1;                  // no clock without an RTC module: Temp & Hum
  enterMode(h == 0 ? M_HOME : h == 1 ? M_TH : h == 2 ? M_CLOCK : h == 3 ? M_POMO : h == 4 ? M_CHRONO : M_TALLY);
}

// back to the Settings page, on setting number i
void backToParams(uint8_t i) { enterMode(M_PARAMS); parIdx = i; }

void openMenu() {
  stopAlarmPreview();
  if (mode == M_MUSIC) musicToneStop();
  if (mode == M_CHESS) chessPause();
  prevMode = mode;
  int t = potRead() / ((POT_MAX + 1) / N_MENU);       // the entry follows the dial position
  menuIdx = (t > N_MENU - 1) ? N_MENU - 1 : t;
  mode = M_MENU;
  clockSub = 0; alarmSub = 0; thSub = 0; parSub = 0; tallySub = 0;
  potReset();
  lastDraw = 0;
}

void backToMenu() {
  if (mode == M_MENU) { enterMode(prevMode); return; }          // in the menu: back to the page
  if (mode == M_BRIGHT) { cfg.bright = brightBackup; applyColor(); }   // cancels the brightness change
  if (mode == M_ALARM && alarmSub != 0) saveCfg();
  if (mode == M_TH && thSub != 0) saveCfg();
  if (mode == M_PARAMS && parSub != 0) setParValue(parIdx, parBackup);   // cancels the setting being edited
  openMenu();
}

// menu entry selected by the potentiometer (anti-flicker hysteresis)
uint8_t menuFromPot() {
  const int zone = (POT_MAX + 1) / N_MENU;
  int raw = potRead();
  int target = raw / zone;
  if (target > N_MENU - 1) target = N_MENU - 1;
  int cur = menuIdx;
  if (target > cur && raw >= target * zone + 15) return target;
  if (target < cur && raw <= cur * zone - 15) return target;
  return menuIdx;
}

// letters of the active background functions, except the one of the displayed page (columns 13 to 15)
void badges(uint8_t self) {
  char all[5];                                       // order: ! A C M (M always stays the rightmost)
  uint8_t n = 0;
  if (self != M_TH     && anyAlert())                all[n++] = '!';
  if (self != M_ALARM  && cfg.armed && rtcPresent)   all[n++] = 'A';
  if (self != M_CHRONO && chronoRun)                 all[n++] = 'C';
  if (self != M_POMO   && pomoRun)                   all[n++] = ((millis() / 1000UL) % 10 == 9) ? ' ' : 'M';   // M for the timer on every page; absent for 1 s every 10 s
  char s[4] = { ' ', ' ', ' ', 0 };                  // columns 13 to 15, right-aligned
  uint8_t keep = n > 3 ? 3 : n;
  for (uint8_t i = 0; i < keep; i++) s[3 - keep + i] = all[n - keep + i];
  lcd.setCursor(13, 0);
  lcd.print(s);
}

// ---------- Pages ----------
uint8_t menuMode(uint8_t i);
void drawMenu(uint8_t e, bool draw) {
  static const char names[] PROGMEM =
  "Favorite\0"
  "Examen\0"
  "Temp & Hum\0"
  "Timer\0"
  "Counter\0"
  "Stopwatch\0"
  "Alarm\0"
  "Dice\0"
  "Chess clock\0"
  "Melody\0"
  "Settings\0"
;
  if (potLive()) menuIdx = menuFromPot();
  if (e == EV_SHORT) { if (menuIdx == 0) goHome(); else enterMode(menuMode(menuIdx)); return; }
  if (e == EV_LONG || e == EV_VLONG) { enterMode(prevMode); return; }
  if (!draw) return;
  char b[17];
  color(40, 80, 160);
  snprintf_P(b, sizeof(b), PSTR("MENU       %2d/%d"), menuIdx + 1, N_MENU);
  line(0, b);
  char nm[13]; strcpy_P(nm, packedText(names, menuIdx));
  snprintf_P(b, sizeof(b), PSTR("> %s"), nm);
  line(1, b);
}

uint8_t menuMode(uint8_t i) {
  static const uint8_t modes[N_MENU] PROGMEM = { M_HOME, M_EXAM, M_TH, M_POMO, M_TALLY, M_CHRONO, M_ALARM, M_DIE, M_CHESS, M_MUSIC, M_PARAMS };
  return pgm_read_byte(&modes[i]);
}

// text "T23.4C  H 45%"
void thText(char* b, size_t n) {
  if (shtOk) {
    int t10 = temp10;
    snprintf_P(b, n, PSTR("T%s%d.%dC  H%3d%%"), t10 < 0 ? "-" : "",
             abs(t10) / 10, abs(t10) % 10, (hum10 + 5) / 10);
  } else {
    snprintf_P(b, n, PSTR("T --.-C  H --%%"));
  }
}

void noRtcScreen() {
  color(60, 60, 120);
  lineP(0, TR("Installer module", "Install RTC", "Instalar modulo", "RTC-Modul noetig"));
  lineP(1, TR("RTC pour l'heure", "module for clock", "RTC para la hora", "fuer die Uhrzeit"));
}

void drawClock(bool draw) {
  if (!draw) return;
  if (!rtcPresent) { noRtcScreen(); return; }
  char b[17];
  color(40, 90, 160);                                 // dimmed by night mode
  snprintf_P(b, sizeof(b), PSTR("    %02d:%02d:%02d"), nowH, nowM, nowS);
  line(0, b);
  badges(M_CLOCK);
  thText(b, sizeof(b));
  line(1, b);
}

void introSound();
// possible pages for Home: 0 auto, 1 Temp & Hum, 2 clock, 3 timer, 4 stopwatch, 5 counter
static const char homeName[] PROGMEM =
  "Auto\0"
  "Temp & Hum\0"
  "Clock\0"
  "Timer\0"
  "Stopwatch\0"
  "Counter\0"
;
// Settings: brightness, volume, startup sound, auto sleep, night mode, set time, favorite.

void drawParams(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { backToMenu(); return; }          // double click: back to the menu (cancels an open setting)
  if (parSub == 0) {
    if (potLive()) parIdx = 1 + potSelect(N_PAR, parIdx - 1, 30);
    if (e == EV_SHORT) {
      if (parIdx == 1)      { enterMode(M_BRIGHT); return; }
      else if (parIdx == 6) { enterMode(M_SETCLOCK); return; }
      parSub = 1; parBackup = parValue(parIdx); potReset();
    }
  } else {
    if (potLive()) {
      uint8_t cur = parValue(parIdx);
      uint8_t count = parIdx == 2 ? 3 : parIdx == 3 ? 2 : parIdx == 4 ? 8 : parIdx == 5 ? 3 : 6;
      uint8_t v = potSelect(count, cur, parIdx == 2 ? 40 : 30);
      if (v != cur) { setParValue(parIdx, v); if (parIdx == 2) beep(1, 100); }
    }
    if (e == EV_SHORT) {
      saveCfg(); parSub = 0; potReset();
      if (parIdx == 3 && cfg.sound) introSound();
    }
  }
  if (!draw) return;
  char b[17], nm[14], val[14];
  color(40, 90, 160);
  strcpy_P(nm, parIdx == 1 ? TR_PTR(TXT_Luminosite_15, TXT_Brightness_7, TXT_Brillo_8, TXT_Helligkeit_13)
             : parIdx == 2 ? TR_PTR(TXT_Volume_31, TXT_Volume_31, PSTR("Volumen"), PSTR("Lautstaerke"))
             : parIdx == 3 ? TR("Son demarr.", "Boot sound", "Sonido inic.", "Startton")
             : parIdx == 4 ? TR("Veille auto", "Auto sleep", "Reposo auto", "Auto-Standby")
             : parIdx == 5 ? TR("Mode nuit", "Night mode", "Modo noche", "Nachtmodus")
             : parIdx == 6 ? TR("Regler heure", "Set time", "Ajustar hora", "Uhr stellen")
                           : TR("Favori", "Favorite", "Favorito", "Favorit"));
  snprintf_P(b, sizeof(b), PSTR("%-12s %d/%d"), nm, parIdx, N_PAR);
  line(0, b);
  if (parIdx == 6) {
    if (rtcPresent) snprintf_P(b, sizeof(b), PSTR("%02d:%02d        OK"), nowH, nowM);
    else            snprintf_P(b, sizeof(b), PSTR("--:--        OK"));
    line(1, b);
    return;
  }
  if (parIdx == 1) {
    snprintf_P(b, sizeof(b), PSTR("%3d%%         OK"), cfg.bright);
    line(1, b);
    return;
  }
  if (parIdx == 7) {
    strcpy_P(val, packedText(homeName, cfg.home));
    snprintf_P(b, sizeof(b), parSub ? TXT_12s_5 : TXT_12s_OK_1, val);
    line(1, b);
    return;
  }
  if (parIdx == 4) {
    uint8_t m = pgm_read_byte(&SLEEP_MIN[cfg.sleep]);
    if (m == 0)       strcpy_P(val, TR("Jamais", "Never", "Nunca", "Nie"));
    else if (m == 60) strcpy_P(val, PSTR("1 h"));
    else              snprintf_P(val, sizeof(val), TXT_u_min_4, m);
    snprintf_P(b, sizeof(b), parSub ? TXT_12s_5 : TXT_12s_OK_1, val);
    line(1, b);
    return;
  }
  if (parIdx == 5) {
    strcpy_P(val, cfg.night == 0 ? TR("Non", "Off", "No", "Aus") : cfg.night == 1 ? TR_PTR(TXT_Auto_22_6h_6, TXT_Auto_22_6h_6, TXT_Auto_22_6h_6, TXT_Auto_22_6h_6) : TR("Oui", "On", "Si", "An"));
    snprintf_P(b, sizeof(b), parSub ? TXT_12s_5 : TXT_12s_OK_1, val);
    line(1, b);
    return;
  }
  if (parIdx == 3) strcpy_P(val, cfg.sound ? TR_PTR(TXT_OUI_22, PSTR("ON"), TXT_SI_25, TXT_JA_14) : TR_PTR(TXT_NON_19, PSTR("OFF"), TXT_NO_18, TXT_NEIN_17));
  else {
    char bar[4] = "   ";
    for (uint8_t i = 0; i <= cfg.volume; i++) bar[i] = '#';                            // ### = loud
    strcpy_P(val, volumeName(cfg.volume));
    snprintf_P(b, sizeof(b), parSub ? PSTR("> %-6s %-3s OK") : PSTR("%-6s %-3s  OK"), val, bar);
    line(1, b);
    return;
  }
  snprintf_P(b, sizeof(b), parSub ? PSTR("> %-9s   OK") : PSTR("%-9s OK=chg"), val);
  line(1, b);
}

void drawTempHum(uint8_t e, bool draw);
// Home uses the combined measurements / clock page, without automatic page changes.
void drawHome(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { enterMode(M_TH); thSub = 1; potReset(); return; }   // set the thresholds
  if (e != EV_NONE || potLive()) { openMenu(); return; }   // button or dial: open the menu
  drawTempHum(EV_NONE, draw);
}

// Clock page alone (chosen in Settings > Home): button or dial = menu
void drawClockPage(uint8_t e, bool draw) {
  if (e != EV_NONE || potLive()) { openMenu(); return; }
  drawClock(draw);
}

uint8_t* thPtr(uint8_t sub) { return sub == 1 ? &cfg.tLo2 : sub == 2 ? &cfg.tHi2 : sub == 3 ? &cfg.hLo : &cfg.hHi; }

// sets threshold number 'sub' with the dial, keeping the minimum gap between low and high
void thEdit(uint8_t sub) {
  bool isT = (sub <= 2);
  uint8_t mn = isT ? T_MIN2 : H_MIN, mx = isT ? T_MAX2 : H_MAX, gap = isT ? T_GAP2 : H_GAP;
  uint8_t* p = thPtr(sub);
  uint8_t v = mn + potSelect(mx - mn + 1, *p - mn, 4);
  if (sub == 1 && v > cfg.tHi2 - gap) v = cfg.tHi2 - gap;
  if (sub == 2 && v < cfg.tLo2 + gap) v = cfg.tLo2 + gap;
  if (sub == 3 && v > cfg.hHi - gap)  v = cfg.hHi - gap;
  if (sub == 4 && v < cfg.hLo + gap)  v = cfg.hLo + gap;
  *p = v;
}

void drawTempHum(uint8_t e, bool draw) {
  if (thSub == 0) {
    if (e == EV_DOUBLE)    { thSub = 1; potReset(); }     // set the four thresholds
    else if (e != EV_NONE) { openMenu(); return; }
  } else if (e == EV_SHORT) {
    if (thSub < 4) { thSub++; potReset(); }
    else           { thSub = 0; saveCfg(); potReset(); }
  }
  if (thSub != 0 && potLive()) thEdit(thSub);
  if (thSub == 0 && potLive()) { openMenu(); return; }   // turning the dial opens the menu

  if (!draw) return;
  char b[17];
  if (thSub != 0) {
    static const char titles[] PROGMEM =
      "Temp too low\0"
      "Temp too high\0"
      "Humid too low\0"
      "Humid too high\0";
    color(150, 100, 0);
    lineP(0, packedText(titles, thSub - 1));
    uint8_t v = *thPtr(thSub);
    if (thSub <= 2) snprintf_P(b, sizeof(b), PSTR("T%s%d.%dC  OK"), thSub == 1 ? "<=" : ">=", v / 2, (v % 2) * 5);
    else            snprintf_P(b, sizeof(b), PSTR("H%s%d%%  OK"),   thSub == 3 ? "<=" : ">=", v);
    line(1, b);
    return;
  }
  // The clock and background badges remain visible even during a sensor alert.
  if (rtcPresent) snprintf_P(b, sizeof(b), PSTR("%02d:%02d:%02d"), nowH, nowM, nowS);
  else strcpy_P(b, PSTR("--:--:--"));
  line(0, b); badges(M_TH);
  if (!shtOk) {
    color(60, 60, 60);
    lineP(1, TR_PTR(TXT_Capteur_absent_9, TXT_Sensor_missing_29, TXT_Sensor_ausente_27, TXT_Sensor_fehlt_28));
    return;
  }

  // Second row: measurements / full alert, with measurements before each alert.
  PGM_P msg[2]; uint8_t states[2], n = 0;
  if (tState) {
    states[n] = tState;
    msg[n++] = tState == 1 ? TR("TEMP TROP HAUTE", "TEMP TOO HIGH", "TEMP MUY ALTA", "TEMP ZU HOCH") : TR("TEMP TROP BASSE", "TEMP TOO LOW", "TEMP MUY BAJA", "TEMP ZU TIEF");
  }
  if (hState) {
    states[n] = hState;
    msg[n++] = hState == 1 ? TR("HUM TROP HAUTE", "HUM TOO HIGH", "HUM MUY ALTA", "FEUCHTE ZU HOCH") : TR("HUM TROP BASSE", "HUM TOO LOW", "HUM MUY BAJA", "FEUCHTE ZU TIEF");
  }
  uint8_t slot = n ? (millis() / 2000UL) % (2 * n) : 0;
  uint8_t state = n ? states[slot / 2] : 0;
  if (state == 1) color(255, 0, 0);
  else if (state == 2) color(0, 80, 255);
  else color(0, 160, 40);
  if (slot & 1) { lineP(1, msg[slot / 2]); return; }
  // Degrees use LCD character 0xDF; measurements occupy at most 13 columns.
  snprintf_P(b, sizeof(b), PSTR("T%s%d.%d%c H%d%%"), temp10 < 0 ? "-" : "",
             abs(temp10) / 10, abs(temp10) % 10, 0xDF, (hum10 + 5) / 10);
  line(1, b);
  if (!n) { lcd.setCursor(14, 1); lcd.print("OK"); }
}

void drawSetClock(uint8_t e, bool draw) {
  if (!rtcPresent) {
    if (e != EV_NONE || potLive()) backToParams(6);
    else if (draw) noRtcScreen();
    return;
  }
  if (e == EV_SHORT) {
    if (clockSub == 1) { clockSub = 2; potReset(); }
    else { setClock(edH, edM); backToParams(6); return; }
  }
  if (clockSub == 1 && potLive()) edH = potSelect(24, edH, 4);
  if (clockSub == 2 && potLive()) edM = potSelect(60, edM, 4);

  if (!draw) return;
  char b[17];
  color(150, 100, 0);
  lineP(0, clockSub == 1 ? TR("Heure : heures", "Time: hours", "Hora: horas", "Zeit: Stunden") : TR("Heure : minutes", "Time: minutes", "Hora: minutos", "Zeit: Minuten"));
  snprintf_P(b, sizeof(b), clockSub == 1 ? TR("%02d:%02d OK=suite", "%02d:%02d OK=next", "%02d:%02d OK=sigue", "%02d:%02d OK=weiter") : TR("%02d:%02d OK=valider", "%02d:%02d OK=confirm", "%02d:%02d OK=acepta", "%02d:%02d OK=fertig"), edH, edM);
  line(1, b);
}

void drawAlarm(uint8_t e, bool draw) {
  if (!rtcPresent) {
    if (e != EV_NONE || potLive()) { openMenu(); return; }
    if (draw) { color(60, 60, 120); lineP(0, TR("Alarme : il faut", "Alarm: needs the", "Alarma: requiere", "Wecker: braucht")); lineP(1, TR("le module RTC", "RTC module", "modulo RTC", "RTC-Modul")); }
    return;
  }
  if (alarmSub == 0) {
    if (e == EV_DOUBLE)     { cfg.armed = !cfg.armed; saveCfg(); }   // arm / disarm
    else if (e == EV_SHORT) { alarmSub = 1; potReset(); }
    else if (potLive())     { openMenu(); return; }
  } else if (e == EV_SHORT) {
    stopAlarmPreview();
    alarmSub++;
    potReset();
    if (alarmSub == 6) { alarmSub = 0; saveCfg(); }
  }

  if (alarmSub == 1 && potLive()) cfg.aH = potSelect(24, cfg.aH, 4);
  if (alarmSub == 2 && potLive()) cfg.aM = potSelect(60, cfg.aM, 4);
  if (alarmSub == 3 && potLive()) cfg.armed = potSelect(2, cfg.armed, 20);
  if (alarmSub == 4 && potLive()) alarmCfg.melody = potSelect(N_ALARM_TUNES, alarmCfg.melody, 15);
  if (alarmSub == 5 && potLive()) alarmCfg.volume = potSelect(3, alarmCfg.volume, 20);
  updateAlarmPreview();

  if (!draw) return;
  char b[17];
  if (alarmSub == 0) {
    color(150, 70, 0);
    snprintf_P(b, sizeof(b), TR("Alarme  %02d:%02d", "Alarm   %02d:%02d", "Alarma  %02d:%02d", "Wecker  %02d:%02d"), cfg.aH, cfg.aM);
    line(0, b);
    badges(M_ALARM);
    if (snoozing) lineP(1, TR("Snooze en cours", "Snoozing", "Repitiendo", "Schlummern"));
    else          lineP(1, cfg.armed ? TR("ON   OK=regler", "ON   OK=set", "ON   OK=ajustar", "AN   OK=stellen") : TR("OFF  OK=regler", "OFF  OK=set", "OFF  OK=ajustar", "AUS  OK=stellen"));
    return;
  }
  color(150, 100, 0);
  switch (alarmSub) {
    case 1:  lineP(0, TR("Alarme : heures", "Alarm: hours", "Alarma: horas", "Wecker: Stunden"));  snprintf_P(b, sizeof(b), PSTR("%02d h"), cfg.aH); break;
    case 2:  lineP(0, TR("Alarme : minutes", "Alarm: minutes", "Alarma: minutos", "Wecker: Minuten")); snprintf_P(b, sizeof(b), PSTR("%02d min"), cfg.aM); break;
    case 4:  lineP(0, TR("Alarme: melodie", "Alarm: melody", "Alarma: melodia", "Wecker: Melodie")); strcpy_P(b, alarmTuneName()); break;
    case 5:  lineP(0, TR("Alarme: volume", "Alarm: volume", "Alarma: volumen", "Wecker: Lautst."));
             strcpy_P(b, volumeName(alarmCfg.volume)); break;
    default: lineP(0, TR("Alarme armee ?", "Alarm armed?", "Alarma activa?", "Wecker aktiv?")); strcpy_P(b, cfg.armed ? TR_PTR(TXT_OUI_22, PSTR("YES"), TXT_SI_25, TXT_JA_14) : TR_PTR(TXT_NON_19, TXT_NO_18, TXT_NO_18, TXT_NEIN_17)); break;
  }
  line(1, b);
}

void drawPomo(uint8_t e, bool draw) {
  if (e == EV_DOUBLE && pomoIdle()) { openMenu(); return; }   // already at zero: double click = back to the menu
  if (e == EV_DOUBLE) { pomoReset(); potReset(); }   // reset ; otherwise the timer keeps running in the background
  else if (e == EV_SHORT) {
    if (pomoIdle()) saveCfg();                       // remembers the chosen duration
    pomoRun = !pomoRun;
    pomoLast = millis();
    potReset();                                      // the dial only counts from now on
  }

  // Once started, the dial opens the menu without stopping the countdown.
  if (!pomoIdle() && potLive()) { openMenu(); return; }
  if (pomoIdle() && potLive()) {                     // when stopped: the potentiometer sets the duration
    uint8_t cur = workIndex(cfg.workMin);
    uint8_t idx = potSelect(N_WORK, cur, 3);
    if (idx != cur) { cfg.workMin = workValue(idx); pomoRemMs = phaseMs(0); }
  }

  if (!draw) return;
  char b[17];
  unsigned long rs = (pomoRemMs + 999UL) / 1000UL;
  if (pomoPhase == 0)      { color(220, 30, 0);  snprintf_P(b, sizeof(b), TR_PTR(PSTR("MINUTEUR #%d"), TXT_TIMER_d_30, PSTR("TEMPORIZ. #%d"), TXT_TIMER_d_30), pomoCount + 1); }
  else if (pomoPhase == 1) { color(0, 170, 50);  snprintf_P(b, sizeof(b), TR("PAUSE COURTE", "SHORT BREAK", "PAUSA CORTA", "KURZE PAUSE")); }
  else                     { color(0, 80, 220);  snprintf_P(b, sizeof(b), TR("PAUSE LONGUE", "LONG BREAK", "PAUSA LARGA", "LANGE PAUSE")); }
  line(0, b);
  badges(M_POMO);
  char t[12], d[12];
  if (pomoIdle()) {
    fmtDuration(d, sizeof(d), cfg.workMin);
    snprintf_P(b, sizeof(b), PSTR("%s  OK=start"), d);
  } else {
    if (rs >= 3600) snprintf_P(t, sizeof(t), PSTR("%d:%02d:%02d"), (int)(rs / 3600), (int)((rs / 60) % 60), (int)(rs % 60));
    else            snprintf_P(t, sizeof(t), PSTR("%02d:%02d"), (int)(rs / 60), (int)(rs % 60));
    snprintf_P(b, sizeof(b), pomoRun ? TR_PTR(PSTR("%s en cours"), TXT_s_running_3, PSTR("%s en curso"), TXT_s_laeuft_2) : TR("%s pause", "%s paused", "%s pausa", "%s Pause"), t);
  }
  line(1, b);
}

void drawChrono(uint8_t e, bool draw) {
  bool idle = !chronoRun && chronoMs() == 0;       // stopwatch at zero, waiting for a short press
  if (e == EV_DOUBLE && idle) { openMenu(); return; }   // already at zero: double click = back to the menu
  if (e == EV_DOUBLE) { chronoRun = false; chronoAccum = 0; potReset(); }   // reset ; otherwise the stopwatch keeps running in the background
  else if (e == EV_SHORT) {
    if (chronoRun) { chronoAccum = millis() - chronoStart; chronoRun = false; }
    else           { chronoStart = millis() - chronoAccum; chronoRun = true; }
    potReset();                                      // the dial only counts from now on
  }
  if (potLive()) { openMenu(); return; }            // turning the dial opens the menu (the stopwatch keeps running in the background)

  if (!draw) return;
  color(110, 40, 170);
  unsigned long ms = chronoMs();
  unsigned long h = ms / 3600000UL;
  unsigned long m = (ms / 60000UL) % 60;
  unsigned long sec = (ms / 1000UL) % 60;
  char t[12], b[17];
  if (h > 0) snprintf_P(t, sizeof(t), PSTR("%lu:%02lu:%02lu"), h % 100, m, sec);
  else       snprintf_P(t, sizeof(t), PSTR("%02lu:%02lu.%lu"), m, sec, (ms / 100UL) % 10);
  lineP(0, TR("CHRONO", "STOPWATCH", "CRONOMETRO", "STOPPUHR"));
  badges(M_CHRONO);
  snprintf_P(b, sizeof(b), chronoRun ? TR_PTR(PSTR("%s marche"), TXT_s_running_3, PSTR("%s marcha"), TXT_s_laeuft_2) : (ms == 0 ? PSTR("%s OK=start") : TR("%s arret", "%s stopped", "%s parado", "%s gestoppt")), t);
  line(1, b);
}

void drawTally(uint8_t e, bool draw, bool tap) {
  if (tallySub == 0) {
    if (tap && tallyCount < 9999) { tallyCount++; lastDraw = 0; }   // each press counts immediately
    if (potLive()) { tallySub = 1; tallyT0 = millis(); }            // dial: reset request
  } else {
    if (e == EV_DOUBLE || millis() - tallyT0 > 5000) { tallySub = 0; potReset(); }
    else if (e == EV_SHORT && millis() - tallyT0 > 600) { tallyCount = 0; tallySub = 0; potReset(); }
  }
  if (!draw) return;
  char b[17];
  if (tallySub != 0) {
    color(150, 100, 0);
    lineP(0, TR("Remise a zero ?", "Reset counter?", "Poner a cero?", "Zuruecksetzen?"));
    drawConfirmHint();
    return;
  }
  color(0, 130, 130);
  lineP(0, TR("COMPTEUR", "COUNTER", "CONTADOR", "ZAEHLER"));
  badges(M_TALLY);
  snprintf_P(b, sizeof(b), PSTR("%16u"), tallyCount);
  line(1, b);
  // Active time at the bottom left; the counter stays in the four rightmost columns.
  // When both run, alternate timer and stopwatch every two seconds.
  if (pomoRun || chronoRun) {
    bool timer = pomoRun && (!chronoRun || (millis() / 2000UL) % 2 == 0);
    unsigned long secs = timer ? (pomoRemMs + 999UL) / 1000UL : chronoMs() / 1000UL;
    char t[12];
    if (secs >= 3600UL)
      snprintf_P(t, sizeof(t), PSTR("%c%lu:%02lu:%02lu"), timer ? 'M' : 'C', secs / 3600UL, (secs / 60UL) % 60, secs % 60);
    else
      snprintf_P(t, sizeof(t), PSTR("%c%02lu:%02lu"), timer ? 'M' : 'C', secs / 60UL, secs % 60);
    lcd.setCursor(0, 1);
    lcd.print(t);
  }
}

void drawDie(uint8_t e, bool draw, bool tap) {
  if (e == EV_DOUBLE) { openMenu(); return; }          // double click: back to the menu
  uint8_t faces = pgm_read_byte(&DIE_FACES[dieIdx]);
  if (dieRolling) {
    if ((long)(millis() - dieEnd) >= 0) {              // end of the roll: final result
      seedRng(micros() + analogRead(PIN_POT));
      dieVal = rollDie(faces);
      dieRolling = false;
      beep(1, 60);
      lastDraw = 0;
    }
  } else {
    if (potLive()) {                                   // the dial chooses the number of sides
      uint8_t n = potSelect(N_DIE, dieIdx, 20);
      if (n != dieIdx) { dieIdx = n; dieVal = 0; faces = pgm_read_byte(&DIE_FACES[dieIdx]); }
    }
    if (tap) {                                         // each press rolls the die immediately
      seedRng(micros() + analogRead(PIN_POT));
      dieRolling = true; dieEnd = millis() + 900; dieVal = 0;
    }
  }
  if (!draw) return;
  char b[17], h[12];
  color(170, 60, 130);
  lineP(0, TR("DE", "DICE", "DADO", "WUERFEL"));
  badges(M_DIE);
  if (dieRolling) {
    dieShow = rollDie(faces);
    buzz(1800, 5);                                     // small tick during the roll
    snprintf_P(b, sizeof(b), TXT_D_3u_3u_10, faces, dieShow);
  } else if (dieVal != 0) {
    snprintf_P(b, sizeof(b), TXT_D_3u_3u_10, faces, dieVal);
  } else {
    strcpy_P(h, TR("OK=lancer", "OK=roll", "OK=tirar", "OK=wuerfeln"));
    snprintf_P(b, sizeof(b), PSTR("D%-3u  %s"), faces, h);
  }
  line(1, b);
}

// Music: dial = choose a track, short press = play / pause / resume, double click = back to the menu.
void drawMusic(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { openMenu(); return; }
  if (e == EV_PRESS || e == EV_SHORT) {
    unsigned long now = millis();
    if (musicState == 1) {
      long remaining = (long)(musicNoteEnd - now);
      musicRemain = remaining > 0 ? remaining : 0;
      musicToneStop();
      musicState = 2;
    } else {
      musicState = 1;
      if (musicRemain > 0) {
        musicNoteEnd = now + musicRemain;
        if (musicFreq) musicToneStart(musicFreq); else musicToneStop();
      }
    }
    lastDraw = 0;
  }
  if (musicState != 1 && potLive()) {
    uint8_t selected = potSelect(N_MUSIC, musicTrack, 15);
    if (selected != musicTrack) {
      musicToneStop();
      musicTrack = selected; musicState = 0; musicPos = 0; musicRemain = 0; lastDraw = 0;
    }
  }
  if (musicState == 1) {
    unsigned long now = millis();
    if (musicRemain > 0 && (long)(now - musicNoteEnd) >= 0) musicRemain = 0;
    if (musicRemain <= 0) {
      uint8_t noteCount = pgm_read_byte(&MUSIC_NOTE_COUNTS[musicTrack]);
      if (musicPos >= noteCount) { musicToneStop(); musicState = 0; musicPos = 0; lastDraw = 0; }
      else {
        musicRemain = playTrackNote(musicTrack, musicPos, cfg.volume, musicFreq);
        musicPos++;
        musicNoteEnd = now + musicRemain;
      }
    }
  }
  if (!draw) return;
  color(120, 60, 180);
  char b[17], title[13];
  strcpy_P(title, packedText(MUSIC_NAMES, musicTrack));
  snprintf_P(b, sizeof(b), PSTR("%u/%u %s"), musicTrack + 1, N_MUSIC, title);
  line(0, b);
  badges(M_MUSIC);
  if (musicState == 1)      lineP(1, TR("Lecture OK=pause", "Playing OK=pause", "Suena OK=pausa", "Spielt OK=Pause"));
  else if (musicState == 2) lineP(1, TR("Pause OK=lire", "Paused OK=play", "Pausa OK=tocar", "Pause OK=Start"));
  else                   lineP(1, TR("Tourner=choix OK", "Dial=choose OK", "Dial=gira OK", "Dreh=wahl OK"));
}

// Chess clock: a short press hands over to the other player, the dial chooses the time control (when stopped)
void drawChess(uint8_t e, bool draw) {
  if (chessState == 0) {
    if (e == EV_DOUBLE) { openMenu(); return; }
    if (potLive()) chessIdx = potSelect(N_CHESS, chessIdx, 20);
    if (e == EV_SHORT) {                               // start: player 1 begins
      long t = (long)pgm_read_byte(&CHESS_MIN[chessIdx]) * 60000L;
      chessBase[0] = t; chessBase[1] = t;
      chessSide = 0; chessState = 1; chessTurn = millis();
      beep(1, 60); lastR = 255;
    }
  } else if (chessState == 1) {
    if (e == EV_SHORT) {                               // player change, at the instant of the press
      unsigned long at = tapMs;
      if ((long)(at - chessTurn) < 0) at = chessTurn;
      chessBase[chessSide] += (long)pgm_read_byte(&CHESS_INC[chessIdx]) * 1000L - (long)(at - chessTurn);
      chessSide ^= 1; chessTurn = at;
      buzz(2000, 10);
    } else if (e == EV_DOUBLE) chessPause();
    if (chessState == 1 && chessLeft(chessSide) == 0) {   // flag fallen
      chessBase[chessSide] = 0; chessState = 3;
      beep(3, 200); lastR = 255;
    }
  } else if (chessState == 2) {
    if (e == EV_SHORT)       { chessState = 1; chessTurn = millis(); lastR = 255; }   // resume
    else if (e == EV_DOUBLE) { chessState = 0; lastR = 255; }                          // reset
  } else {
    if (e == EV_SHORT || e == EV_DOUBLE) { chessState = 0; lastR = 255; }
  }
  if (!draw) return;
  char b[17], tag[10];
  if (chessState == 0) {
    color(40, 90, 160);
    lineP(0, TR("PEND. ECHECS", "CHESS CLOCK", "AJEDREZ", "SCHACHUHR"));
    badges(M_CHESS);
    strcpy_P(tag, TR_PTR(TXT_OK_start_21, TXT_OK_start_21, PSTR("OK=inicio"), PSTR("OK=Start")));
    snprintf_P(b, sizeof(b), PSTR("%u+%u  %s"), pgm_read_byte(&CHESS_MIN[chessIdx]), pgm_read_byte(&CHESS_INC[chessIdx]), tag);
    line(1, b);
    return;
  }
  if (chessState == 3) color(255, 0, 0);
  else if (chessState == 2) color(150, 100, 0);
  else if (chessSide == 0) color(0, 110, 200);
  else color(200, 110, 0);
  for (uint8_t sd = 0; sd < 2; sd++) {
    long sec = (chessLeft(sd) + 999) / 1000;
    tag[0] = 0;
    if (chessState == 3 && sd == chessSide) strcpy_P(tag, TR_PTR(TXT_FIN_12, PSTR("END"), TXT_FIN_12, PSTR("ENDE")));
    else if (chessState == 2 && sd == chessSide) strcpy_P(tag, TR_PTR(TXT_PAUSE_23, TXT_PAUSE_23, PSTR("PAUSA"), TXT_PAUSE_23));
    snprintf_P(b, sizeof(b), PSTR("%c%d  %02d:%02d  %s"), (chessState != 3 && sd == chessSide) ? '>' : ' ', sd + 1, (int)(sec / 60), (int)(sec % 60), tag);
    line(sd, b);
  }
}

void drawBright(uint8_t e, bool draw) {
  if (e == EV_SHORT) { saveCfg(); backToParams(1); return; }

  if (potLive()) {
    brightIdx = potSelect(8, brightIdx, 10);
    cfg.bright = BRIGHT_LEVELS[brightIdx];
    applyColor();                                    // immediate preview
  }

  if (!draw) return;
  char bar[9], b[17];
  for (uint8_t i = 0; i < 8; i++) bar[i] = (i <= brightIdx) ? '#' : '-';
  bar[8] = 0;
  color(40, 90, 160);
  lineP(0, TR_PTR(TXT_Luminosite_15, TXT_Brightness_7, TXT_Brillo_8, TXT_Helligkeit_13));
  snprintf_P(b, sizeof(b), PSTR("%3d%% %s"), cfg.bright, bar);
  line(1, b);
}

// ---------- Startup intro ----------
const char BRAND1[] PROGMEM = "PR BELAMINE";     // 2nd intro screen, line 1
const char BRAND2[] PROGMEM = "MAKERLAB";        // 2nd intro screen, line 2

void centered(uint8_t row, const char* t) {
  char b[17];
  uint8_t len = strlen(t);
  if (len > 16) len = 16;
  uint8_t pad = (16 - len) / 2;
  memset(b, ' ', 16);                            // manual centering: "%*s" does not exist on the UNO
  memcpy(b + pad, t, len);
  b[16] = 0;
  line(row, b);
}
void centeredP(uint8_t row, PGM_P s) {
  char t[17];
  strncpy_P(t, s, 16); t[16] = 0;
  centered(row, t);
}
void introSound() {                              // short three-note melody
  if (!cfg.sound || nightActive()) return;
  buzz(1500, 90); delay(40);
  buzz(2000, 90); delay(40);
  buzz(2600, 160);
  digitalWrite(PIN_BUZZER, LOW);
}
void intro() {
  lastR = 40; lastG = 90; lastB = 160;
  for (uint8_t i = 0; i <= 100; i += 5) {        // the backlight fades in slowly
    lcd.setRGB(40 * i / 100 * cfg.bright / 100, 90 * i / 100 * cfg.bright / 100, 160 * i / 100 * cfg.bright / 100);
    if (i == 0) { centeredP(0, TXT_DESK_WATCH_11); centeredP(1, TXT_EMPTY_0); }
    delay(25);
  }
  centeredP(0, TXT_DESK_WATCH_11); centeredP(1, TXT_EMPTY_0);   // rewritten: in case the screen was not ready
  introSound();
  delay(1000);
  centeredP(0, BRAND1); centeredP(1, BRAND2);      // second screen: the brand
  delay(1500);
  centeredP(0, PSTR("TEACHER EDITION"));
  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < 16; ++i) { lcd.write((uint8_t)255); delay(60); }
}

// ---------- Sleep (very long press) ----------
void sleepNow() {
  stopAlarmPreview();
  centeredP(0, TR("A bientot", "Goodbye", "Hasta pronto", "Tschuess"));
  centeredP(1, TXT_EMPTY_0);
  delay(700);
  lcd.clear();
  lcd.setRGB(0, 0, 0);
  standby = true;
  lastR = 255;
}
void wakeUp() {
  standby = false;
  lastActivity = millis();
  lcd.clear();
  goHome();
  lastR = 255; lastDraw = 0;
  potReset();
  if (cfg.sound) introSound();
}

// ---------- Program ----------
void examTick() {
  unsigned long now = millis();
  if (examState == EX_RUNNING) {
    unsigned long dt = now - examLast;
    if (examCopies < examTarget) examElapsed += min(dt, examRemaining);
    if (dt >= examRemaining) {
      examRemaining = 0; examState = EX_ELAPSED;
      if (!ringing) beep(2, 100);
    } else examRemaining -= dt;
  }
  examLast = millis();
}

void drawExam(uint8_t e, bool draw, bool tap) {
  if (examState == EX_FINISHED && examSub == 0 && e == EV_SHORT) {
    examState = EX_DURATION; potReset(); return;
  }
  if (e == EV_LONG) {
    if (!examSession()) { openMenu(); return; }
    examSub = examSub ? 0 : 1; examAction = 0; potReset();
  }
  if (examSub == 2) {
    if (e == EV_SHORT) { examState = EX_FINISHED; examSub = 0; potReset(); return; }
    if (e == EV_DOUBLE) { examSub = 0; potReset(); return; }
    if (draw) {
      color(150, 100, 0);
      lineP(0, TR("Terminer examen?", "End exam?", "Terminar examen?", "Pruefung enden?"));
      drawConfirmHint();
    }
    return;
  }
  if (examSub == 1) {
    if (potLive()) examAction = potSelect(6, examAction, 15);
    if (e == EV_DOUBLE) { examSub = 0; potReset(); return; }
    if (e == EV_SHORT) {
      examSub = 0;
      if (examAction == 1 && examCopies) examCopies--;
      if (examAction == 2) {
        if (examState == EX_RUNNING) examState = EX_PAUSED;
        else if (examRemaining) { examState = EX_RUNNING; examLast = millis(); }
      }
      if (examAction == 3) {
        examRemaining = min(examRemaining + 300000UL, 14400000UL);
        if (examState == EX_ELAPSED) examState = EX_RUNNING;
        examLast = millis();
      }
      if (examAction == 4) examSub = 2;
      if (examAction == 5) { openMenu(); return; }
      potReset(); return;
    }
    if (draw) {
      color(150, 100, 0);
      lineP(0, TR("EXAMEN: actions", "EXAM: actions", "EXAMEN: acciones", "PRUEFUNG: Menue"));
      PGM_P label = examAction == 0 ? TR("Retour", "Back", "Volver", "Zurueck")
        : examAction == 1 ? TR("Retirer 1 copie", "Subtract 1 paper", "Restar 1 copia", "1 Blatt weniger")
        : examAction == 2 ? (examState == EX_RUNNING ? TR_PTR(TXT_Pause_24, TXT_Pause_24, PSTR("Pausa"), TXT_Pause_24) : TR("Reprendre", "Resume", "Continuar", "Fortsetzen"))
        : examAction == 3 ? PSTR("+5 min")
        : examAction == 4 ? TR("Terminer", "End exam", "Terminar", "Beenden")
        : TR_PTR(TXT_Menu_principal_16, PSTR("Main menu"), TXT_Menu_principal_16, PSTR("Hauptmenue"));
      lineP(1, label);
    }
    return;
  }
  if (!examSession()) {
    if (e == EV_SHORT) {
      if (examState == EX_READY) {
        examCopies = 0; examElapsed = 0; examRemaining = (unsigned long)examMinutes * 60000UL;
        examState = EX_RUNNING; examLast = millis();
      } else examState++;
      potReset();
    }
    if (examState == EX_DURATION && potLive()) examMinutes = 1 + potSelect(240, examMinutes - 1, 2);
    if (examState == EX_TARGET && potLive()) examTarget = 1 + potSelect(99, examTarget - 1, 4);
    if (draw && !examSession()) {
      char b[17]; color(40, 90, 160);
      lineP(0, examState == EX_DURATION ? TR("Duree examen", "Exam duration", "Duracion examen", "Pruefungsdauer")
        : examState == EX_TARGET ? TR("Copies attendues", "Expected papers", "Copias previstas", "Anzahl Blaetter")
        : TR("EXAMEN PRET", "EXAM READY", "EXAMEN LISTO", "PRUEFUNG BEREIT"));
      if (examState == EX_DURATION) snprintf_P(b, sizeof(b), PSTR("%u min   OK"), examMinutes);
      else if (examState == EX_TARGET) snprintf_P(b, sizeof(b), PSTR("%u   OK"), examTarget);
      else snprintf_P(b, sizeof(b), PSTR("%umin %u  OK"), examMinutes, examTarget);
      line(1, b); return;
    }
    if (!examSession()) return;
  }
  // Each short release counts, including both presses of a double click.
  if (tap && examState != EX_FINISHED && examCopies < examTarget) { examCopies++; draw = true; }
  if (!draw) return;
  char b[17]; unsigned long secs = (examRemaining + 999UL) / 1000UL;
  color(examCopies == examTarget ? 0 : examState == EX_ELAPSED ? 255 : 40,
        examCopies == examTarget ? 160 : examState == EX_ELAPSED ? 0 : 90, 40);
  if (examState == EX_FINISHED || ((examState == EX_ELAPSED || examCopies == examTarget) && (millis() / 2000UL) % 2)) {
    unsigned long elapsed = examElapsed / 1000UL;
    snprintf_P(b, sizeof(b), PSTR("TOTAL     %3lu:%02lu"), elapsed / 60UL, elapsed % 60UL);
    line(0, b);
  }
  else if (examCopies == examTarget) lineP(0, TR("COPIES RENDUES", "ALL PAPERS IN", "TODAS ENTREGADAS", "ALLE ABGEGEBEN"));
  else if (examState == EX_ELAPSED) lineP(0, TR("TEMPS ECOULE", "TIME IS UP", "TIEMPO AGOTADO", "ZEIT ABGELAUFEN"));
  else {
    snprintf_P(b, sizeof(b), examState == EX_PAUSED ? PSTR("PAUSE     %3lu:%02lu") : PSTR("EXAMEN    %3lu:%02lu"), secs / 60UL, secs % 60UL);
    line(0, b);
  }
  snprintf_P(b, sizeof(b), TR("Copies     %02u/%02u", "Papers     %02u/%02u", "Copias     %02u/%02u", "Blaetter   %02u/%02u"), examCopies, examTarget);
  line(1, b);
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  Wire.begin();
  loadCfg();
  lcd.begin(16, 2);
  delay(150);                                    // lets the screen finish its initialization

  intro();

  Wire.beginTransmission(RTC_ADDR);
  rtcPresent = (Wire.endTransmission() == 0);
  pomoReset();
  readClock();
  shtOk = readSht();
  if (shtOk) updateAlerts();

  color(40, 90, 160);                            // module check
  centeredP(0, rtcPresent ? TR("Horloge RTC OK", "RTC clock OK", "Reloj RTC OK", "RTC-Uhr OK") : TR("Sans horloge RTC", "No RTC clock", "Sin reloj RTC", "Keine RTC-Uhr"));
  centeredP(1, shtOk ? TR_PTR(PSTR("Capteur OK"), TXT_Sensor_OK_26, TXT_Sensor_OK_26, TXT_Sensor_OK_26) : TR_PTR(TXT_Capteur_absent_9, TXT_Sensor_missing_29, TXT_Sensor_ausente_27, TXT_Sensor_fehlt_28));
  delay(1200);
  mode = M_MENU;
  potReset();
  lastPotSample = potRead();
  lastActivity = millis();
}

void loop() {
  uint8_t e = readButton();
  bool tap = tapFlag; tapFlag = false;
  unsigned long now = millis();

  if (now - lastClockRead >= 200) { lastClockRead = now; readClock(); }
  if (now - lastSht >= 2000)      { lastSht = now; shtOk = readSht(); if (shtOk) updateAlerts(); }

  pomoTick();
  examTick();
  checkAlarm();
  if (!standby && nightActive() != nightPrev) applyColor();      // automatic night mode starts or stops

  digitalWrite(PIN_LED, (standby || btnHeld >= LED_HOLD_MS) ? HIGH : LOW);   // LED on while asleep and during the very long press

  // activity: press or dial movement (sampled every 200 ms)
  if (e != EV_NONE || tap) lastActivity = now;
  if (now - lastPotCheck >= 200) {
    lastPotCheck = now;
    int pv = potRead();
    if (abs(pv - lastPotSample) > 25) { lastPotSample = pv; lastActivity = now; }
  }

  if (standby && !ringing && e != EV_NONE) { if (e != EV_VLONG) lastActivity = now; wakeUp(); e = EV_NONE; }   // a press wakes it up
  if (!ringing && e == EV_VLONG) { sleepNow(); e = EV_NONE; }   // very long press: sleep
  if (!ringing && !standby && cfg.sleep != 0 && !examSession() && !(mode == M_CHESS && chessState == 1) &&
      now - lastActivity >= (unsigned long)pgm_read_byte(&SLEEP_MIN[cfg.sleep]) * 60000UL) { sleepNow(); e = EV_NONE; }   // auto sleep
  if (standby && !ringing) return;                    // screen off: only the alarm and the background functions keep running
  if (!ringing && e == EV_LONG && mode != M_EXAM) { backToMenu(); e = EV_NONE; }

  unsigned long period = (mode == M_CHRONO || mode == M_TALLY || mode == M_DIE || mode == M_CHESS) ? 100UL : 200UL;
  bool draw = (now - lastDraw >= period) || (e != EV_NONE);
  if (draw) lastDraw = now;

  if (ringing) { handleRing(e, draw); return; }

  switch (mode) {
    case M_EXAM:     drawExam(e, draw, tap); break;
    case M_PARAMS:   drawParams(e, draw);   break;
    case M_HOME:     drawHome(e, draw);     break;
    case M_CLOCK:    drawClockPage(e, draw); break;
    case M_TH:       drawTempHum(e, draw);  break;
    case M_ALARM:    drawAlarm(e, draw);    break;
    case M_POMO:     drawPomo(e, draw);     break;
    case M_CHRONO:   drawChrono(e, draw);   break;
    case M_TALLY:    drawTally(e, draw, tap); break;
    case M_DIE:      drawDie(e, draw, tap);   break;
    case M_CHESS:    drawChess(e, draw);      break;
    case M_MUSIC:    drawMusic(e, draw);      break;
    case M_SETCLOCK: drawSetClock(e, draw); break;
    case M_BRIGHT:   drawBright(e, draw);   break;
    case M_MENU:     drawMenu(e, draw);     break;
  }
}
