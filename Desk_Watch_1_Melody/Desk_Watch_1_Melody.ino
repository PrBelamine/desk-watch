// Desk Watch 1: Temp & Hum / Timer (Pomodoro) / Stopwatch / Counter / Dice / Chess clock / Alarm / Clock
// Hardware : Arduino UNO + Grove Base Shield, LCD RGB V4.0 (I2C),
//            SHT31 (I2C 0x44), buzzer D3, button D4, potentiometer (dial) A1,
//            optional RTC DS1307 / DS3231 (I2C 0x68).
// Required library: "Grove - LCD RGB Backlight" (rgb_lcd.h).
// SHT31 and RTC are read directly over I2C: no other library needed.
//
// All functions run in the background (humidity, alarm, pomodoro, stopwatch).
//
// Navigation:
//   Long press (anywhere) : open the MENU ; inside the MENU, go back to the page
//   Home                  : without RTC, Temp & Hum ; with RTC, alternates time / Temp & Hum ; short press = open the MENU
//   Menu: Favorite, Temp & Hum, Timer, Stopwatch, Alarm, Counter, Dice, Chess clock, Melody, Settings
//   Settings: language, brightness, volume, startup sound, auto sleep, night mode, set time, favorite
//   Night mode   : No / Auto (22:00 to 06:00, with RTC) / Yes : screen dimmed to 8 %, no alert beeps, melody muted ; the alarm still rings and displays normally
//   Dial: turn right = next value / entry
//   Manual               : dial = scroll through the user manual (27 pages), one click, double click or long press = back to the menu
//   Settings             : dial = choose, short press = open / confirm
//   Startup              : DESK WATCH, short melody, PR BELAMINE MAKERLAB, Melody Edition loading bar, module check
//   MENU                 : dial = scroll, short press = enter
//
// Functions:
//   Temp & Hum   : alerts TOO HIGH (red) / TOO LOW (blue) / OK (green) ; double click = set the 4 thresholds
//                  (T low, T high, H low, H high) with the dial, short press = next then confirm ;
//                  thresholds are saved and reused at startup
//   Counter      : each short press = +1 (immediate) ; dial = ask for a reset (OK = yes, double click or 5 s = no)
//   Dice         : short press = roll (tick for 1 s then result), dial = number of sides (2, 4, 6, 8, 10, 12, 20, 30, 100), double click = menu
//   Music        : dial = choose a melody, short press = play / pause / resume, double click = menu
//   Chess        : two-player clock ; dial = time control (1+0 to 60+0, 5+3...), short press = start then next player, double click = pause / reset / menu
//   Alarm (menu) : short press = set the alarm (hours, minutes, armed), double click = arm / disarm
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
//                              P timer running (last column, blinks off for 1 s every 10 s)

#include <Wire.h>
#include <EEPROM.h>
#include <rgb_lcd.h>

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

enum { M_TH, M_POMO, M_CHRONO, M_TALLY, M_DIE, M_CHESS, M_MUSIC, M_PARAMS, M_MANUAL, M_HOME, M_ALARM, M_SETCLOCK, M_BRIGHT, M_MENU, M_CLOCK };
const uint8_t N_MENU = 10;
const uint8_t N_PAR = 8;
const uint8_t N_MANUAL = 27;     // manual pages
uint8_t mode = M_HOME, prevMode = M_HOME;   // M_* values
uint8_t menuIdx = 0;
uint8_t brightBackup = 100, brightIdx = 7;

enum { EV_NONE, EV_SHORT, EV_LONG, EV_VLONG, EV_DOUBLE, EV_PRESS };

struct Settings { uint8_t magic, aH, aM, armed, workMin, bright, tLo2, tHi2, hLo, hHi, sound, lang, volume, home, night, sleep; } cfg;   // lang: 0 French, 1 English, 2 Spanish, 3 German ; home: page opened by Home (0 auto, 1 Temp & Hum, 2 clock, 3 timer, 4 stopwatch, 5 counter) ; night: night mode (0 no, 1 auto by time of day, 2 yes) ; sleep: auto sleep delay (index into SLEEP_MIN, 0 = never)

// text in the selected language (French, English, Spanish, German), in flash memory
#define TR(fr, en, es, de) (cfg.lang == 1 ? PSTR(en) : cfg.lang == 2 ? PSTR(es) : cfg.lang == 3 ? PSTR(de) : PSTR(fr))

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
uint8_t alarmSub = 0;       // alarm setting: 0 view, 1 hours, 2 minutes, 3 armed
uint8_t manPage = 0;        // manual page displayed
uint8_t parSub = 0, parBackup = 0;   // setting open? value before editing
uint8_t parIdx = 0;          // Settings page: 0 sound, 1 volume, 2 language, 3 home, 4 night, 5 auto sleep, 6 alarm, 7 time, 8 brightness
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
static const char MUSIC_NAMES[4][N_MUSIC][13] PROGMEM = {
  { "Anniversaire", "Pour Elise", "Mario Bros", "Tetris" },
  { "Birthday", "Fur Elise", "Mario Bros", "Tetris" },
  { "Cumpleanos", "Para Elisa", "Mario Bros", "Tetris" },
  { "Geburtstag", "Fuer Elise", "Mario Bros", "Tetris" } };
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
  if (m <= 60) snprintf_P(b, n, PSTR("%u min"), m);
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

void line(uint8_t row, const char* s) {
  char b[17];
  snprintf_P(b, sizeof(b), PSTR("%-16s"), s);
  lcd.setCursor(0, row);
  lcd.print(b);
}

// text in flash memory (saves Arduino UNO RAM)
void lineP(uint8_t row, PGM_P s) {
  char b[17];
  strncpy_P(b, s, 16); b[16] = 0;
  line(row, b);
}

void musicToneStop() {
  TCCR2A = 0;
  TCCR2B = 0;
  PORTD &= ~_BV(PD3);
}

void musicToneStart(unsigned int freq) {
  static const uint8_t DUTY[3] = { 5, 20, 50 };
  unsigned long count = F_CPU / (256UL * freq);
  if (count < 1) count = 1;
  if (count > 256) count = 256;
  musicToneStop();
  OCR2A = count - 1;
  OCR2B = count * DUTY[cfg.volume] / 100;
  TCNT2 = 0;
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS22) | _BV(CS21);
}

// Buzzer sound: square wave at the requested frequency ; the volume sets the duty cycle
// (5 %, 20 % or 50 %: the shorter it is, the quieter the piezo buzzer). Blocking for 'ms'.
void buzz(unsigned int freq, unsigned int ms) {
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
    if (musicPressHandled) { musicPressHandled = false; return EV_NONE; }
    return EV_SHORT;
  }

  if (p && !down) {
    down = true; t0 = now;
    second = pending && (now - tRel <= DOUBLE_MS);
    if (mode == M_MUSIC && !second) { musicPressHandled = true; return EV_PRESS; }
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
    cfg.bright = 100; cfg.sound = 1; cfg.lang = 1; cfg.volume = 2; cfg.home = 0; cfg.night = 1; cfg.sleep = 0; cfg.tLo2 = 0xFF;       // thresholds are re-checked below
  }
  if (cfg.sleep > 7) cfg.sleep = 0;                     // auto sleep: off by default
  if (cfg.night > 2) cfg.night = 1;                     // night mode: automatic by default
  if (cfg.home > 6) cfg.home = 0;                       // Favorite: automatic by default
  if (cfg.volume > 2) cfg.volume = 2;                   // maximum volume by default
  if (cfg.lang > 3) cfg.lang = 1;                       // English by default
  if (cfg.sound > 1) cfg.sound = 1;                    // startup sound: on by default
  // thresholds: default values if missing or inconsistent
  if (cfg.tLo2 < T_MIN2 || cfg.tHi2 > T_MAX2 || cfg.tHi2 < cfg.tLo2 + T_GAP2 ||
      cfg.hLo < H_MIN || cfg.hHi > H_MAX || cfg.hHi < cfg.hLo + H_GAP) {
    cfg.tLo2 = T_LO_DEFAULT2; cfg.tHi2 = T_HI_DEFAULT2;
    cfg.hLo = H_LO_DEFAULT;   cfg.hHi = H_HI_DEFAULT;
  }
  if (cfg.bright < 1 || cfg.bright > 100) cfg.bright = 100;   // old save or invalid value
}
void saveCfg() { EEPROM.put(0, cfg); }

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
bool readSht() {
  Wire.beginTransmission(SHT_ADDR);
  Wire.write((uint8_t)0x24); Wire.write((uint8_t)0x00);   // high repeatability measurement
  if (Wire.endTransmission() != 0) return false;
  delay(20);
  if (Wire.requestFrom(SHT_ADDR, (uint8_t)6) != 6) return false;
  uint16_t rt = (uint16_t)Wire.read() << 8; rt |= Wire.read(); Wire.read();
  uint16_t rh = (uint16_t)Wire.read() << 8; rh |= Wire.read(); Wire.read();
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
  if (standby) { standby = false; lcd.clear(); }   // the alarm turns the screen back on
  ringing = true; snoozing = false;
  ringStart = millis();
  lastR = 255;                          // forces the colour update
}
void stopRing() {
  ringing = false; snoozing = false;
  lastR = 255;
  digitalWrite(PIN_BUZZER, LOW);
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

  // beeps getting closer together: 1000 ms -> 400 ms
  unsigned long period = (el < 60) ? 1000UL - el * 10UL : 400UL;
  if (((now - ringStart) % period) < 120) buzz(2300, 8);   // ringing: 8 ms bursts while the beep is active
  else digitalWrite(PIN_BUZZER, LOW);

  if (e == EV_SHORT || e == EV_DOUBLE || el >= RING_MAX_S) {
    digitalWrite(PIN_BUZZER, LOW);
    ringing = false; snoozing = true; snoozeUntil = now + SNOOZE_MS;
    lastR = 255;
    return;
  }
  if (e == EV_LONG || e == EV_VLONG) { stopRing(); return; }

  if (draw) {
    color(255, el < 30 ? 90 : 0, 0);
    char b[17];
    snprintf_P(b, sizeof(b), TR("REVEIL  %02d:%02d", "ALARM   %02d:%02d", "ALARMA  %02d:%02d", "WECKER  %02d:%02d"), nowH, nowM);
    line(0, b);
    lineP(1, TR("OK=snooze 5 min", "OK=snooze 5 min", "OK=repetir 5 min", "OK=Snooze 5 min"));
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
// values of the Settings page items (0 sound, 1 volume, 2 language)
uint8_t parValue(uint8_t i) { return i == 0 ? cfg.lang : i == 2 ? cfg.volume : i == 3 ? cfg.sound : i == 4 ? cfg.sleep : i == 5 ? cfg.night : cfg.home; }
void setParValue(uint8_t i, uint8_t v) { if (i == 0) cfg.lang = v; else if (i == 2) cfg.volume = v; else if (i == 3) cfg.sound = v; else if (i == 4) cfg.sleep = v; else if (i == 5) cfg.night = v; else cfg.home = v; }

void chessPause() {                    // leaving the page pauses the clock
  if (chessState == 1) { chessBase[chessSide] -= (long)(millis() - chessTurn); chessState = 2; }
}

void enterMode(uint8_t m) {
  if (mode == M_CHESS && m != M_CHESS) chessPause();
  if (mode == M_MUSIC && m != M_MUSIC) musicToneStop();
  mode = m;
  clockSub = 0; alarmSub = 0; thSub = 0; parIdx = 0; parSub = 0; tallySub = 0;
  if (m == M_DIE) dieRolling = false;
  if (m == M_MUSIC) { musicState = 0; musicPos = 0; musicRemain = 0; }
  potReset();
  if (m == M_SETCLOCK) { clockSub = 1; edH = nowH; edM = nowM; }
  if (m == M_MANUAL) { int pg = potRead() / (1024 / N_MANUAL); manPage = pg > N_MANUAL - 1 ? N_MANUAL - 1 : pg; }
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
  enterMode(h == 0 ? M_HOME : h == 1 ? M_TH : h == 2 ? M_CLOCK : h == 3 ? M_POMO : h == 4 ? M_CHRONO : h == 5 ? M_TALLY : M_MANUAL);
}

// back to the Settings page, on setting number i
void backToParams(uint8_t i) { enterMode(M_PARAMS); parIdx = i; }

void openMenu() {
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
  char all[5];                                       // order: ! A C P (P always stays the rightmost)
  uint8_t n = 0;
  if (self != M_TH     && anyAlert())                all[n++] = '!';
  if (self != M_ALARM  && cfg.armed && rtcPresent)   all[n++] = 'A';
  if (self != M_CHRONO && chronoRun)                 all[n++] = 'C';
  if (self != M_POMO   && pomoRun)                   all[n++] = ((millis() / 1000UL) % 10 == 9) ? ' ' : 'P';   // P blinks: absent for 1 s every 10 s
  char s[4] = { ' ', ' ', ' ', 0 };                  // columns 13 to 15, right-aligned
  uint8_t keep = n > 3 ? 3 : n;
  for (uint8_t i = 0; i < keep; i++) s[3 - keep + i] = all[n - keep + i];
  lcd.setCursor(13, 0);
  lcd.print(s);
}

// ---------- Pages ----------
uint8_t menuMode(uint8_t i);
void drawMenu(uint8_t e, bool draw) {
  static const char names[4][N_MENU][13] PROGMEM = {
    { "Favori", "Temp et Hum", "Minuteur", "Chrono", "Reveil", "Compteur", "De", "Pendule", "Melodie", "Parametres" },
    { "Favorite", "Temp & Hum", "Timer", "Stopwatch", "Alarm", "Counter", "Dice", "Chess clock", "Melody", "Settings" },
    { "Favorito", "Temp y Hum", "Temporizador", "Cronometro", "Alarma", "Contador", "Dado", "Ajedrez", "Melody", "Ajustes" },
    { "Favorit", "Temp/Feuchte", "Timer", "Stoppuhr", "Wecker", "Zaehler", "Wuerfel", "Schachuhr", "Melody", "Einstellung" } };
  menuIdx = menuFromPot();
  if (e == EV_SHORT) { if (menuIdx == 0) goHome(); else enterMode(menuMode(menuIdx)); return; }
  if (e == EV_LONG || e == EV_VLONG) { enterMode(prevMode); return; }
  if (!draw) return;
  char b[17];
  color(40, 80, 160);
  snprintf_P(b, sizeof(b), PSTR("MENU       %2d/%d"), menuIdx + 1, N_MENU);
  line(0, b);
  char nm[13]; strcpy_P(nm, names[cfg.lang][menuIdx]);
  snprintf_P(b, sizeof(b), PSTR("> %s"), nm);
  line(1, b);
}

uint8_t menuMode(uint8_t i) {
  static const uint8_t modes[N_MENU] PROGMEM = { M_HOME, M_TH, M_POMO, M_CHRONO, M_ALARM, M_TALLY, M_DIE, M_CHESS, M_MUSIC, M_PARAMS };
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
// User manual: 27 pages of 2 lines, per language (French, English, Spanish, German)
static const char manual[4][N_MANUAL][2][17] PROGMEM = {
  {
    { "Bienvenue sur", "DESK WATCH" },
    { "Appui court:", "ouvre le menu" },
    { "Dans le menu:", "molette + OK" },
    { "Appui long:", "retour au menu" },
    { "Appui >2,5 s:", "veille / reveil" },
    { "Double clic:", "action de page" },
    { "Temp et Hum:", "dbl clic=seuils" },
    { "Compteur: clic", "= +1" },
    { "Compteur:", "molette, OK=RAZ" },
    { "De: clic=lancer", "molette = faces" },
    { "Echecs: molette", "= cadence (5+3)" },
    { "Echecs: clic =", "joueur suivant" },
    { "Echecs: double", "clic = pause" },
    { "Parametres:", "son,volume,lang." },
    { "Parametres:", "reveil,heure,lum" },
    { "Accueil: choisir", "dans Parametres" },
    { "Mode nuit: ecran","tamise, sans bip" },
    { "Veille auto:", "delai en Param." },
    { "Reveil: double", "clic = ON / OFF" },
    { "Minuteur: duree", "molette 1min-4h" },
    { "Minuteur: court", "start / pause" },
    { "Min. et Chrono:", "double = a zero" },
    { "Chrono: court", "start / arret" },
    { "Sonnerie: court", "snooze 5 min" },
    { "Sonnerie: long", "arret" },
    { "En haut a droite", "! A P C actifs" },
    { "Manuel: clic", "= sortir" },
  },
  {
    { "Welcome to", "DESK WATCH" },
    { "Short press:", "opens the menu" },
    { "In the menu:", "knob + OK" },
    { "Long press:", "back to menu" },
    { "Press > 2.5 s:", "sleep / wake" },
    { "Double click:", "page action" },
    { "Temp & Hum:", "dbl click=limits" },
    { "Counter: click", "= +1" },
    { "Counter: knob,", "then OK = reset" },
    { "Dice: click=roll", "knob = sides" },
    { "Chess: knob =", "time (5+3)" },
    { "Chess: click =", "next player" },
    { "Chess: double", "click = pause" },
    { "Settings: sound,", "volume, language" },
    { "Settings: alarm,", "time, brightness" },
    { "Home: choose the", "page in Settings" },
    { "Night mode: dim","screen, no beeps" },
    { "Auto sleep: set", "time in Settings" },
    { "Alarm: double", "click = ON / OFF" },
    { "Timer: length", "knob 1 min-4 h" },
    { "Timer: short", "start / pause" },
    { "Timer/Stopwatch:", "double = reset" },
    { "Stopwatch: short", "start / stop" },
    { "Ringing: short", "snooze 5 min" },
    { "Ringing: long", "stop" },
    { "Top right corner", "! A P C active" },
    { "Manual: click", "= exit" },
  },
  {
    { "Bienvenido a", "DESK WATCH" },
    { "Pulsacion corta", "abre el menu" },
    { "En el menu:", "rueda + OK" },
    { "Pulsacion larga:", "vuelve al menu" },
    { "Pulsar >2,5 s:", "reposo / activar" },
    { "Doble pulsacion:", "accion de pagina" },
    { "Temp y Hum:", "doble=limites" },
    { "Contador: pulsa", "= +1" },
    { "Contador: rueda,", "luego OK = cero" },
    { "Dado: pulsa=tira", "rueda = caras" },
    { "Ajedrez: rueda", "= ritmo (5+3)" },
    { "Ajedrez: pulsa", "= otro jugador" },
    { "Ajedrez: doble", "pulsa = pausa" },
    { "Ajustes: sonido,", "volumen, idioma" },
    { "Ajustes: alarma,", "hora, brillo" },
    { "Inicio: elegir", "en Ajustes" },
    { "Modo noche: poca","luz, sin avisos" },
    { "Reposo auto:", "plazo en Ajustes" },
    { "Alarma: doble", "pulsa = ON/OFF" },
    { "Temporiz.: dur.", "rueda 1 min-4 h" },
    { "Temporiz.: corta", "inicio / pausa" },
    { "Temp./Cronom.:", "doble = a cero" },
    { "Cronom.: corta", "inicio / parada" },
    { "Sonando: corta", "repetir 5 min" },
    { "Sonando: larga", "parar" },
    { "Arriba derecha", "! A P C activos" },
    { "Manual: pulsa", "= salir" },
  },
  {
    { "Willkommen bei", "DESK WATCH" },
    { "Kurzer Druck:", "Menue oeffnen" },
    { "Im Menue:", "Drehknopf + OK" },
    { "Langer Druck:", "Zurueck: Menue" },
    { "Druck >2,5 s:", "Standby / Start" },
    { "Doppelklick:", "Seitenaktion" },
    { "Doppelklick:", "Grenzwerte" },
    { "Zaehler: Klick", "= +1" },
    { "Zaehler: Knopf,", "dann OK = null" },
    { "Wuerfel: Klick", "Knopf = Seiten" },
    { "Schach: Knopf", "= Zeit (5+3)" },
    { "Schach: Klick", "= naechster" },
    { "Schach: Doppel-", "klick = Pause" },
    { "Einstellung:", "Ton,Lautst.,Spr." },
    { "Einstellung:", "Wecker,Uhr,Licht" },
    { "Start: Seite in", "Einstellung" },
    { "Nachtmodus:", "dunkel, kein Ton" },
    { "Auto-Standby:", "Zeit: Einstell." },
    { "Wecker: Doppel-", "klick = AN/AUS" },
    { "Timer: Dauer", "Knopf 1 Min-4 h" },
    { "Timer: kurz", "Start / Pause" },
    { "Timer/Stoppuhr:", "Doppel = null" },
    { "Stoppuhr: kurz", "Start / Stopp" },
    { "Klingeln: kurz", "Snooze 5 Min" },
    { "Klingeln: lang", "Stopp" },
    { "Oben rechts", "! A P C aktiv" },
    { "Anleitung: Klick", "= Ende" },
  },
};

void drawManual(uint8_t e, bool draw) {
  if (e == EV_SHORT || e == EV_DOUBLE) { openMenu(); return; }   // one click: leave the manual
  manPage = potSelect(N_MANUAL, manPage, 8);        // the dial scrolls through the pages
  if (!draw) return;
  color(90, 60, 150);
  lineP(0, manual[cfg.lang][manPage][0]);
  lineP(1, manual[cfg.lang][manPage][1]);
}

static const char langName[4][9] PROGMEM = { "Francais", "English", "Espanol", "Deutsch" };
// possible pages for Home: 0 auto, 1 Temp & Hum, 2 clock, 3 timer, 4 stopwatch, 5 counter
static const char homeName[4][7][13] PROGMEM = {
  { "Auto", "Temp et Hum", "Horloge", "Minuteur", "Chrono", "Compteur", "Manuel" },
  { "Auto", "Temp & Hum", "Clock", "Timer", "Stopwatch", "Counter", "Manual" },
  { "Auto", "Temp y Hum", "Reloj", "Temporizador", "Cronometro", "Contador", "Manual" },
  { "Auto", "Temp/Feuchte", "Uhr", "Timer", "Stoppuhr", "Zaehler", "Anleitung" } };
// Settings: language, brightness, volume, startup sound, auto sleep, night mode, set time, favorite.

void drawParams(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { backToMenu(); return; }          // double click: back to the menu (cancels an open setting)
  if (parSub == 0) {
    if (potLive()) parIdx = potSelect(N_PAR, parIdx, 30);
    if (e == EV_SHORT) {
      if (parIdx == 1)      { enterMode(M_BRIGHT); return; }
      else if (parIdx == 6) { enterMode(M_SETCLOCK); return; }
      parSub = 1; parBackup = parValue(parIdx); potReset();
    }
  } else {
    if (potLive()) {
      uint8_t cur = parValue(parIdx);
      uint8_t count = parIdx == 0 ? 4 : parIdx == 2 ? 3 : parIdx == 3 ? 2 : parIdx == 4 ? 8 : parIdx == 5 ? 3 : 7;
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
  strcpy_P(nm, parIdx == 0 ? TR("Langue", "Language", "Idioma", "Sprache")
             : parIdx == 1 ? TR("Luminosite", "Brightness", "Brillo", "Helligkeit")
             : parIdx == 2 ? TR("Volume", "Volume", "Volumen", "Lautstaerke")
             : parIdx == 3 ? TR("Son demarr.", "Boot sound", "Sonido inic.", "Startton")
             : parIdx == 4 ? TR("Veille auto", "Auto sleep", "Reposo auto", "Auto-Standby")
             : parIdx == 5 ? TR("Mode nuit", "Night mode", "Modo noche", "Nachtmodus")
             : parIdx == 6 ? TR("Regler heure", "Set time", "Ajustar hora", "Uhr stellen")
                           : TR("Favori", "Favorite", "Favorito", "Favorit"));
  snprintf_P(b, sizeof(b), PSTR("%-12s %d/%d"), nm, parIdx + 1, N_PAR);
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
    strcpy_P(val, homeName[cfg.lang][cfg.home]);
    snprintf_P(b, sizeof(b), parSub ? PSTR("> %-12s") : PSTR("%-12s OK"), val);
    line(1, b);
    return;
  }
  if (parIdx == 4) {
    uint8_t m = pgm_read_byte(&SLEEP_MIN[cfg.sleep]);
    if (m == 0)       strcpy_P(val, TR("Jamais", "Never", "Nunca", "Nie"));
    else if (m == 60) strcpy_P(val, PSTR("1 h"));
    else              snprintf_P(val, sizeof(val), PSTR("%u min"), m);
    snprintf_P(b, sizeof(b), parSub ? PSTR("> %-12s") : PSTR("%-12s OK"), val);
    line(1, b);
    return;
  }
  if (parIdx == 5) {
    strcpy_P(val, cfg.night == 0 ? TR("Non", "Off", "No", "Aus") : cfg.night == 1 ? TR("Auto 22-6h", "Auto 22-6h", "Auto 22-6h", "Auto 22-6h") : TR("Oui", "On", "Si", "An"));
    snprintf_P(b, sizeof(b), parSub ? PSTR("> %-12s") : PSTR("%-12s OK"), val);
    line(1, b);
    return;
  }
  if (parIdx == 3) strcpy_P(val, cfg.sound ? TR("OUI", "ON", "SI", "JA") : TR("NON", "OFF", "NO", "NEIN"));
  else if (parIdx == 2) {
    char bar[4] = "   ";
    for (uint8_t i = 0; i <= cfg.volume; i++) bar[i] = '#';                            // ### = loud
    strcpy_P(val, cfg.volume == 0 ? TR("Faible", "Low", "Bajo", "Leise") : cfg.volume == 1 ? TR("Moyen", "Medium", "Medio", "Mittel") : TR("Fort", "High", "Alto", "Laut"));
    snprintf_P(b, sizeof(b), parSub ? PSTR("> %-6s %-3s OK") : PSTR("%-6s %-3s  OK"), val, bar);
    line(1, b);
    return;
  }
  else strcpy_P(val, langName[cfg.lang]);
  snprintf_P(b, sizeof(b), parSub ? PSTR("> %-9s   OK") : PSTR("%-9s OK=chg"), val);
  line(1, b);
}

void drawTempHum(uint8_t e, bool draw);
// Home: without an RTC clock, Temp & Hum ; with RTC, alternates time / Temp & Hum
const unsigned long HOME_CLOCK_MS = 5000, HOME_TH_MS = 4000;
void drawHome(uint8_t e, bool draw) {
  bool showClock = rtcPresent && (millis() % (HOME_CLOCK_MS + HOME_TH_MS)) < HOME_CLOCK_MS;
  if (e == EV_DOUBLE && !showClock) { enterMode(M_TH); thSub = 1; potReset(); return; }   // set the thresholds
  if (e != EV_NONE || potLive()) { openMenu(); return; }   // button or dial: open the menu
  static bool lastShown = false;
  if (showClock != lastShown) { lastShown = showClock; draw = true; lastR = 255; }
  if (showClock) drawClock(draw); else drawTempHum(EV_NONE, draw);
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
    static const char titles[4][4][17] PROGMEM = {
      { "Temp. trop basse", "Temp. trop haute", "Humid trop basse", "Humid trop haute" },
      { "Temp too low", "Temp too high", "Humid too low", "Humid too high" },
      { "Temp muy baja", "Temp muy alta", "Humed muy baja", "Humed muy alta" },
      { "Temp zu tief", "Temp zu hoch", "Feuchte zu tief", "Feuchte zu hoch" } };
    color(150, 100, 0);
    lineP(0, titles[cfg.lang][thSub - 1]);
    uint8_t v = *thPtr(thSub);
    if (thSub <= 2) snprintf_P(b, sizeof(b), PSTR("T%s%d.%dC  OK"), thSub == 1 ? "<=" : ">=", v / 2, (v % 2) * 5);
    else            snprintf_P(b, sizeof(b), PSTR("H%s%d%%  OK"),   thSub == 3 ? "<=" : ">=", v);
    line(1, b);
    return;
  }
  thText(b, sizeof(b));
  line(0, b);
  badges(M_TH);
  if (!shtOk) { color(60, 60, 60); lineP(1, TR("Capteur absent", "Sensor missing", "Sensor ausente", "Sensor fehlt")); return; }
  // alerts: red if a value is too high, otherwise blue if too low, otherwise green
  PGM_P msg[2]; uint8_t n = 0;
  if (hState) msg[n++] = hState == 1 ? TR("HUM TROP HAUTE", "HUM TOO HIGH", "HUM MUY ALTA", "FEUCHTE ZU HOCH") : TR("HUM TROP BASSE", "HUM TOO LOW", "HUM MUY BAJA", "FEUCHTE ZU TIEF");
  if (tState) msg[n++] = tState == 1 ? TR("TEMP TROP HAUTE", "TEMP TOO HIGH", "TEMP MUY ALTA", "TEMP ZU HOCH") : TR("TEMP TROP BASSE", "TEMP TOO LOW", "TEMP MUY BAJA", "TEMP ZU TIEF");
  if (n == 0)                            { color(0, 160, 40); lineP(1, TR("Tout est OK", "All OK", "Todo OK", "Alles OK")); }
  else {
    if (tState == 1 || hState == 1) color(255, 0, 0); else color(0, 80, 255);
    lineP(1, msg[(millis() / 2000UL) % n]);                // two alerts: they alternate
  }
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
    alarmSub++;
    potReset();
    if (alarmSub == 4) { alarmSub = 0; saveCfg(); }
  }

  if (alarmSub == 1 && potLive()) cfg.aH = potSelect(24, cfg.aH, 4);
  if (alarmSub == 2 && potLive()) cfg.aM = potSelect(60, cfg.aM, 4);
  if (alarmSub == 3 && potLive()) cfg.armed = potSelect(2, cfg.armed, 20);

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
    default: lineP(0, TR("Alarme armee ?", "Alarm armed?", "Alarma activa?", "Wecker aktiv?")); strcpy_P(b, cfg.armed ? TR("OUI", "YES", "SI", "JA") : TR("NON", "NO", "NO", "NEIN")); break;
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

  if (pomoIdle() && potLive()) {                     // when stopped: the potentiometer sets the duration
    uint8_t cur = workIndex(cfg.workMin);
    uint8_t idx = potSelect(N_WORK, cur, 3);
    if (idx != cur) { cfg.workMin = workValue(idx); pomoRemMs = phaseMs(0); }
  }

  if (!draw) return;
  char b[17];
  unsigned long rs = (pomoRemMs + 999UL) / 1000UL;
  if (pomoPhase == 0)      { color(220, 30, 0);  snprintf_P(b, sizeof(b), TR("MINUTEUR #%d", "TIMER    #%d", "TEMPORIZ. #%d", "TIMER    #%d"), pomoCount + 1); }
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
    snprintf_P(b, sizeof(b), pomoRun ? TR("%s en cours", "%s running", "%s en curso", "%s laeuft") : TR("%s pause", "%s paused", "%s pausa", "%s Pause"), t);
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
  snprintf_P(b, sizeof(b), chronoRun ? TR("%s marche", "%s running", "%s marcha", "%s laeuft") : (ms == 0 ? PSTR("%s OK=start") : TR("%s arret", "%s stopped", "%s parado", "%s gestoppt")), t);
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
    lineP(1, TR("OK=oui  dbl=non", "OK=yes  dbl=no", "OK=si   dbl=no", "OK=ja  dbl=nein"));
    return;
  }
  color(0, 130, 130);
  lineP(0, TR("COMPTEUR", "COUNTER", "CONTADOR", "ZAEHLER"));
  badges(M_TALLY);
  snprintf_P(b, sizeof(b), PSTR("%16u"), tallyCount);
  line(1, b);
}

void drawDie(uint8_t e, bool draw, bool tap) {
  if (e == EV_DOUBLE) { openMenu(); return; }          // double click: back to the menu
  uint8_t faces = pgm_read_byte(&DIE_FACES[dieIdx]);
  if (dieRolling) {
    if ((long)(millis() - dieEnd) >= 0) {              // end of the roll: final result
      randomSeed(micros() + analogRead(PIN_POT));
      dieVal = random(1, faces + 1);
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
      randomSeed(micros() + analogRead(PIN_POT));
      dieRolling = true; dieEnd = millis() + 900; dieVal = 0;
    }
  }
  if (!draw) return;
  char b[17], h[12];
  color(170, 60, 130);
  lineP(0, TR("DE", "DICE", "DADO", "WUERFEL"));
  badges(M_DIE);
  if (dieRolling) {
    dieShow = random(1, faces + 1);
    buzz(1800, 5);                                     // small tick during the roll
    snprintf_P(b, sizeof(b), PSTR("D%-3u        %3u"), faces, dieShow);
  } else if (dieVal != 0) {
    snprintf_P(b, sizeof(b), PSTR("D%-3u        %3u"), faces, dieVal);
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
        const uint16_t* notes = (const uint16_t*)pgm_read_word(&MUSIC_NOTES[musicTrack]);
        const uint8_t* lengths = (const uint8_t*)pgm_read_word(&MUSIC_LENGTHS[musicTrack]);
        musicFreq = pgm_read_word(&notes[musicPos]);
        musicRemain = pgm_read_byte(&lengths[musicPos]) * 10;
        musicPos++;
        musicNoteEnd = now + musicRemain;
        if (musicFreq) musicToneStart(musicFreq); else musicToneStop();
      }
    }
  }
  if (!draw) return;
  color(120, 60, 180);
  char b[17], title[13];
  strcpy_P(title, MUSIC_NAMES[cfg.lang][musicTrack]);
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
    lineP(0, TR("PENDULE", "CHESS CLOCK", "AJEDREZ", "SCHACHUHR"));
    badges(M_CHESS);
    strcpy_P(tag, TR("OK=start", "OK=start", "OK=inicio", "OK=Start"));
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
    if (chessState == 3 && sd == chessSide) strcpy_P(tag, TR("FIN", "END", "FIN", "ENDE"));
    else if (chessState == 2 && sd == chessSide) strcpy_P(tag, TR("PAUSE", "PAUSE", "PAUSA", "PAUSE"));
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
  lineP(0, TR("Luminosite", "Brightness", "Brillo", "Helligkeit"));
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
    if (i == 0) { centeredP(0, PSTR("DESK WATCH")); centeredP(1, PSTR("")); }
    delay(25);
  }
  centeredP(0, PSTR("DESK WATCH")); centeredP(1, PSTR(""));   // rewritten: in case the screen was not ready
  introSound();
  delay(1000);
  centeredP(0, BRAND1); centeredP(1, BRAND2);      // second screen: the brand
  delay(1500);
  centeredP(0, TR("MELODIE EDITION", "MELODY EDITION", "MELODIA EDITION", "MELODIE EDITION"));
  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < 16; ++i) { lcd.write((uint8_t)255); delay(60); }
}

// ---------- Sleep (very long press) ----------
void sleepNow() {
  centeredP(0, TR("A bientot", "Goodbye", "Hasta pronto", "Tschuess"));
  centeredP(1, PSTR(""));
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
  centeredP(1, shtOk ? TR("Capteur OK", "Sensor OK", "Sensor OK", "Sensor OK") : TR("Capteur absent", "Sensor missing", "Sensor ausente", "Sensor fehlt"));
  delay(1200);
  goHome();
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
  if (!ringing && !standby && cfg.sleep != 0 && !(mode == M_CHESS && chessState == 1) &&
      now - lastActivity >= (unsigned long)pgm_read_byte(&SLEEP_MIN[cfg.sleep]) * 60000UL) { sleepNow(); e = EV_NONE; }   // auto sleep
  if (standby && !ringing) return;                    // screen off: only the alarm and the background functions keep running
  if (!ringing && e == EV_LONG) { backToMenu(); e = EV_NONE; }

  unsigned long period = (mode == M_CHRONO || mode == M_TALLY || mode == M_DIE || mode == M_CHESS) ? 100UL : 200UL;
  bool draw = (now - lastDraw >= period) || (e != EV_NONE);
  if (draw) lastDraw = now;

  if (ringing) { handleRing(e, draw); return; }

  switch (mode) {
    case M_PARAMS:   drawParams(e, draw);   break;
    case M_MANUAL:   drawManual(e, draw);   break;
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
