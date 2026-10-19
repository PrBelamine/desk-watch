# Desk Watch 1 — TEACHER Edition

This edition adds **EXAM** to all existing MELODY Edition features. The `.ino` comments are in English; all four interface languages remain available in Settings. The MELODY files are kept separately.

## Start an exam

After the introduction, the device opens the menu at **Favorite, entry 1/11**, regardless of the dial's initial position. It stays on that entry until the dial moves. Select **Examen**, immediately after Favorite, and click to open exam setup.

1. Turn the dial to choose **1–240 minutes**, then click.
2. Choose **1–99 expected papers**, then click.
3. Check the summary and click to start.

Initial suggestions: **35 minutes and 24 papers**.

```text
EXAMEN     34:59
Papers     00/24
```

Every short press counts one paper immediately. Two quick presses count two papers; a double click never resets the count. The count is capped at the expected total.

## Controls

Hold the button for **0.8–2.5 seconds**, choose an action with the dial, then click:

- **Back**: return to collection.
- **Subtract 1 paper**: undo one count, without going below zero.
- **Pause / Resume**: pause or resume the countdown. Paper collection remains available while paused.
- **+5 min**: add five minutes, capped at four hours remaining. After expiry, this restarts a five-minute countdown.
- **End exam**: requires confirmation. Click to show the final summary; double click to cancel. Another click on the summary returns to setup for the next exam.
- **Main menu**: use another app while the exam countdown continues in the background. Return through the Examen entry to continue collection.

A long press in the action menu returns to collection. During setup, a long press opens the main menu. Turning the dial alone does not leave the collection screen.

## Time up and collection complete

At zero, two short beeps sound once and the display shows **TIME IS UP**. You can continue counting the remaining papers.

At the expected total, the screen turns green and shows **ALL PAPERS IN** and the total. The session remains active until you choose End exam.

After expiry or after the last expected paper is collected, the first row alternates every two seconds between the completion message and **actual elapsed working time, excluding pauses**, for example `TOTAL      35:00`. The paper count stays visible on the second row. Elapsed time freezes at the last expected paper or at expiry; collection after expiry does not increase it.

End exam shows the same elapsed time and paper count in a stable summary. Click to set up the next exam. Extending the countdown adds available time without instantly increasing elapsed working time.

Automatic sleep is disabled throughout an active session, including pause and collection after expiry. Holding for more than 2.5 seconds still enters manual sleep. The remaining time and paper count survive navigation and sleep in RAM, but not power loss or a reset.

## Hardware and validation

Same hardware and libraries as MELODY Edition: Arduino UNO, Grove RGB I2C LCD, D4 button, A1 dial and D3 buzzer. EXAM does not require an RTC; the RTC remains used by the clock and alarm.

Arduino UNO compilation passed. Source-derived logic tests cover 35 minutes / 24 papers, rapid presses, pause/resume, correction, extension, collection after expiry, end confirmation and `millis()` rollover. Physical LCD, button and sound behaviour still require a device check.
