# Phantom Lab

An arterial pressure phantom: a pump drives water through a silicone phantom; two Honeywell sensors read the pressure at the inlet (P1) and the outlet (P2). You type the pump voltage, the program records P1, P2 and the voltage, and checks that the recording is sound.

## How to run, one step after another
1. **Install Python once** (python.org, version 3.10 or newer, tick "Add Python to PATH"). Skip this if you use `PhantomLab.exe`.
2. **Flash the board once:** Arduino IDE, open `firmware/Phantom_HW_executor/Phantom_HW_executor.ino`, choose the Portenta H7 and its port, Upload. Then close the Serial Monitor.
3. **Open the app.** Easiest: double-click **`1_start_app.bat`** (the first time it installs three packages, then opens the window). Or double-click `PhantomLab_v2/PhantomLab.exe` (no Python needed).
4. **Or from a terminal.** In VS Code: menu *Terminal > New Terminal* (shortcut Ctrl+`). It opens at the project folder. Type:
   ```
   pip install -r requirements.txt      (first time only)
   python phantom_lab.py
   ```
5. **In the window:** press *Check board* (top right must say Connected), choose the scale, type AC and DC (or press *Flush air*), pump OFF, press *START*.
6. **After the run** read the DATA CHECK box. Then double-click **`2_check_last_run.bat`** to print the same check for the newest run, or in a terminal: `python phantom_verify.py runs\<folder>`.
7. **MATLAB:** `cd matlab`, then `phantom_check('../runs/<folder>')`.
8. **Automatic tests:** press *Run a plan file ...* and choose `example_plan.csv` (or your own: columns `shape, ac, dc, hr, seconds, rest_s`). It runs the tests one after another and writes `plan_summary.csv`.
No board connected? Tick *Demo mode* and every step above still works with simulated data.

## Start
1. **Once:** flash `firmware/Phantom_HW_executor/Phantom_HW_executor.ino` to the Arduino Portenta H7 (Arduino IDE, Portenta H7 boards installed).
2. Connect the board by USB. Close the Arduino Serial Monitor (only one program can hold the port).
3. Either double-click `PhantomLab.exe`, or install Python 3.10+ and run:
   ```
   pip install -r requirements.txt
   python phantom_lab.py
   ```
4. Press **Check board**. Choose the sensor scale. Type **AC** (height of each beat) and **DC** (where the curve sits), or press **Flush air** (flat 3.0 V, no pulse, 60 s). Press **START**.

No board? Tick **Demo mode** to practise with a simulated rig. `python phantom_lab.py --selftest` runs a 6-second demo through the whole chain without a window.

## What you get
`runs/<date>_<tag>/` with `data.csv` (50 rows per second), `meta.json` (scale, zero, settings, `feedback: none`), `report.txt` (the data check) and `plot.png`. Open `data.csv` in MATLAB with `phantom_check` (see `matlab/`) or in Excel.

## Read next
- `CHECKLIST.md`: the step-by-step check before, during and after a run, including the known-pressure test.
- `FILES.md`: what every file is and how a number gets from the sensor to the CSV.

## Limits to know
- The DAC is powered from 3.3 V, so the pump never gets more than 3.3 V; below about 1.3 V it barely moves fluid.
- There is no feedback or learning in this program. (The old learning mode is in `archive/`.)
- The pressure scale is the datasheet value for the range you choose. It is confirmed only by the known-pressure check in the checklist.
