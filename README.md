# Desk Watch


![Desk Watch](docs/photos/desk_watch_0_cover.jpg)


![Wiring diagram](docs/wiring.png)


## What it does


- Temperature and humidity on a 16x2 RGB LCD (SHT31 sensor), with adjustable HIGH / LOW thresholds: the screen turns red, blue or green and the buzzer warns you.
- FR/EN menu (9 entries): Favorite, Temp & Hum, Timer, Stopwatch, Alarm, Counter, Dice, Chess clock and Settings. The Melody edition adds Melody after Chess clock (10 entries total).
- Timer: Pomodoro from 1 minute to 4 hours.
- Dice: D2 (coin flip), D4, D6, D8, D10, D12, D20, D30 (pick a student in class) and D100.
- Chess clock: 10 time controls from 1+0 to 60+0, with increment (default 5+3).
- Settings: language, brightness, volume, startup sound, automatic sleep, night mode, set time and favorite, in that order.
- Optional RTC module (DS1307 or DS3231): clock and alarm, with an automatic night mode.
- A 27-page manual built into the firmware, readable on the LCD.


## Hardware


![Components: Grove modules, Base Shield, LCD and Arduino UNO R3](docs/photos/desk_watch_components.jpg)


| Part | Connection |
|---|---|
| Arduino UNO R3 + Grove Base Shield V2.0 | USB-B for power and programming |
| Grove LCD RGB Backlight V4.0 (16x2) | I2C |
| Grove SHT31 temperature and humidity sensor (0x44) | I2C |
| Grove RTC, DS1307 or DS3231 (0x68), optional | I2C |
| Grove Buzzer | D3 |
| Grove Button | D4 |
| Grove Rotary Angle sensor (dial) | A1 |


The LCD, the SHT31 and the RTC share the same I2C bus (different addresses), so the order of the I2C ports does not matter.


## Software


1. Choose one of the maintained firmware editions below and open its `.ino` file in the Arduino IDE.
2. Install the **"Grove - LCD RGB Backlight"** library (Library Manager). No other library is needed: the SHT31 and the RTC are read directly over I2C.
3. Select *Arduino Uno* and upload.

## Firmware editions

The repository also contains three firmware editions. **FR and EN identify the language of the source-code comments**, not separate display languages. Both editions have the same nine-entry menu and omit the Melody app so flash space remains available for future sensor and actuator apps. The menu and settings themselves still support the configured display languages.

- **French code comments:** [`Desk_Watch_1_FR/Desk_Watch_1_FR.ino`](Desk_Watch_1_FR/Desk_Watch_1_FR.ino).
- **English code comments:** [`Desk_Watch_1_EN/Desk_Watch_1_EN.ino`](Desk_Watch_1_EN/Desk_Watch_1_EN.ino).
- **Melody jukebox:** [`Desk_Watch_1_Melody/Desk_Watch_1_Melody.ino`](Desk_Watch_1_Melody/Desk_Watch_1_Melody.ino). Its ten-entry menu places Melody after Chess clock and includes Happy Birthday, Für Elise, Super Mario Bros and Tetris. This edition is intended to remain fixed.

All three editions use the same Settings order: Language, Brightness, Volume, Boot sound, Auto sleep, Night mode, Set time and Favorite. Open the matching `.ino` file in Arduino IDE, install the Grove LCD library listed above, select *Arduino Uno* and upload. The FR and EN editions each use 29,974 / 32,256 bytes of flash (92%); Melody uses 31,946 / 32,256 bytes (99%).

## Wokwi simulation

[Open the Desk Watch simulator](https://wokwi.com/projects/478193958373775361)

![Desk Watch simulator on Wokwi](https://thumbs.wokwi.com/projects/478193958373775361/social/1792303170253.png)

Wokwi does not provide an SHT31 model, so the simulation uses a DHT22 connected to D2 to test temperature and humidity readings and alerts. This is only a simulator substitute: the physical Desk Watch uses the Grove SHT31 sensor on I2C.




### Melody jukebox

The Melody firmware is available directly in [`Desk_Watch_1_Melody/Desk_Watch_1_Melody.ino`](Desk_Watch_1_Melody/Desk_Watch_1_Melody.ino). The earlier [Melody release](https://github.com/PrBelamine/desk-watch/releases/tag/v1.1.0-jukebox-beta) remains available as a previous download.

![Happy Birthday app on the Desk Watch](docs/photos/Birthday.jpg)

The Melody build uses 31,946 / 32,256 bytes of flash (99 %) and 455 / 2,048 bytes of RAM (22 %), leaving little room for additional features. The code is commented in English for reading and modification, including in a classroom.


## Photos


| | |
|---|---|
| ![Base with UNO](docs/photos/desk_watch_2.jpg) | ![UNO and shield on the base](docs/photos/desk_watch_1.jpg) |
| ![Assembled case](docs/photos/desk_watch_3.jpg) | ![Top tray, top view](docs/photos/desk_watch_4.jpg) |
| ![Top tray with the four modules](docs/photos/desk_watch_5.jpg) | ![Menu, app 1](docs/photos/desk_watch_m1.jpg) |


Menu apps in use:


![Menu 2/9 Timer](docs/photos/desk_watch_m2.jpg) ![Menu 3/9 Stopwatch](docs/photos/desk_watch_m3.jpg)
![Menu 4/9 Counter](docs/photos/desk_watch_m4.jpg) ![Menu 5/9 Dice](docs/photos/desk_watch_m5.jpg) ![Menu 6/9 Chess clock](docs/photos/desk_watch_m6.jpg)


## Documentation


[User guide (PDF)](docs/Desk_Watch_1_User_Guide.pdf)


## 3D-printed case


The case (base, shell, tray) is designed for a Bambu Lab printer and printed in PLA. The 3D files, print profiles and photos are only on MakerWorld:


https://makerworld.com/models/3451026


## Also published on


- Hackster: https://www.hackster.io/belaminelfay/desk-watch-arduino-uno-multi-app-desk-console-0709c7
- Arduino Forum (Showcase): https://forum.arduino.cc/t/desk-watch-an-arduino-uno-desk-console-with-9-apps-climate-alerts-pomodoro-dice-chess-clock/1464130
- Arduino Project Hub (submitted, waiting for review)


## License


The sketch is released under the **GNU General Public License v3.0** (see `LICENSE`).
