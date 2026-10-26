# Phantom checklist

Tick these in order. A run is only used if the DATA CHECK in the window shows **no FAIL**.

## A. Before the run
- [ ] **Board:** Portenta flashed once with `firmware/Phantom_HW_executor`, connected by USB, Arduino Serial Monitor closed.
- [ ] Open the window, press **Check board**. The top right must say `Connected`.
- [ ] **Scale:** choose the sensor range. `ABPDANT015` is 15 psi, `ABPDANT005` is 5 psi. The label on your sensors reads `ABPDANT005`. Whichever you choose is written into `meta.json`; the raw counts are always saved, so a wrong choice can be corrected later (multiply mmHg by 5/15 or 15/5).
- [ ] **Pump OFF and the loop at rest.** No pressure trapped in the phantom. The sensors are zeroed from this moment.
- [ ] Air in the loop? First press **Flush air** (AC 0, DC 3.0, 60 s) and START. Repeat until no bubbles come out.

## B. During the run
- [ ] The bottom graph shows the voltage you typed (DC up to DC + AC; a flat line if AC is 0).
- [ ] P1 and P2 (top graph) start moving about 0.2 s after the voltage does.
- [ ] STOP at any time switches the pump off. It also stops by itself above 300 mmHg (15 psi scale) or 200 mmHg (5 psi scale).

## C. After the run: read the DATA CHECK
- [ ] 1 Sampling: no gaps, one sample every 20 ms.
- [ ] 2 Sensors valid: no bad samples, nothing stuck at 0 or 4095.
- [ ] 3 Zero: spread under 5 counts (the pump-off reading was steady).
- [ ] 4 Command applied: the board echoes the voltage it was sent.
- [ ] 5 Counts to mmHg: recomputed from raw counts equals the saved mmHg.
- [ ] 6 Response: P1 follows the command after a delay of roughly 0.1 to 0.3 s (flat run: P1 settled).
- [ ] 7 Repeatability: beat-to-beat spread under 2 mmHg.
- [ ] 8 P1 and P2: P1 above P2 and they move together.
- [ ] Any **FAIL**: do not use the data. Read the detail, fix the cause, run again. **WARN**: read it and decide.

## D. Is the pressure really produced by the pump? (once per setup)
- [ ] Press **Waveform test (60 s)** with the AC and DC you care about. It plays sine, two-sine and human for 20 s each.
- [ ] Check 10 must be **PASS**: a model fitted on the sine alone predicts the other two shapes (R-squared 0.9 or more). That means P1 follows whatever shape is commanded, so the pressure shape is the pump's response and not something added by the program.
- [ ] There is **no feedback** in this program: it never changes the voltage because of what the sensors read. `meta.json` says `"feedback": "none"`.
- [ ] Note: the 99% shape matches from October used a learning mode that changed the command every beat. Those show a shape can be reached; this test shows what the pump does by itself.

## E. Are the pressure numbers right? (once per setup, manual)
- [ ] Pump OFF. Connect P1 to a vertical water column. 50 cm of water = 36.8 mmHg (1 cm = 0.7356 mmHg).
- [ ] Do a run (any, short), then **Known-pressure check...**: enter the known mmHg and what P1 showed. Repeat at 25 cm, 50 cm and 75 cm.
- [ ] Check 9 must be **PASS** (error 3 mmHg or less). If P1 shows 3 times too much, the scale is set to 15 psi but the sensor is 5 psi.

## F. MATLAB
- [ ] In MATLAB: `cd matlab`, then `phantom_check('../runs/<run folder>')`.
- [ ] It prints the same ten lines. The two reports must agree. A difference means a bug in one of them.
- [ ] To plot or analyse yourself: `T = readtable('data.csv')` (columns `pc_time, board_ms, seq, cmd_V, pump_V, p1_raw, p2_raw, p1_mmHg, p2_mmHg, ok1, ok2`, 50 samples/s).

## F2. Automatic tests (plan file)
- [ ] Make or edit a plan CSV: `shape, ac, dc, hr, seconds, rest_s` (see `example_plan.csv`). Use `rest_s` of 30 or more after high-DC tests so the pressure drains before the next zero.
- [ ] **Run a plan file ...**, choose it, confirm the pump is off. Every test is zeroed, run, saved and checked.
- [ ] Open `runs/<time>_plan/plan_summary.csv`: no row with `fail` above 0.

## G. Keep with each result
- [ ] The whole run folder (`data.csv`, `meta.json`, `report.txt`, `plot.png`), the scale used, and a note of the phantom's age and any leak or tear.
