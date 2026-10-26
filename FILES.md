# Files: what each one is

## The current program (project root)
| File | Job | Lines |
|---|---|---|
| `example_plan.csv` | A ready plan file for *Run a plan file ...* (5 tests, about 8 minutes). | |
| `1_start_app.bat` | Double-click to open the app (installs the three packages the first time). | |
| `2_check_last_run.bat` | Double-click to print the data check for the newest run in `runs/`. | |
| `phantom_lab.py` | The window. Runs a job in the background and shows P1, P2, the pump voltage and the data check. | 427 |
| `phantom_core.py` | Everything that touches the board and makes the data: voltage shapes, the serial protocol, zero, counts to mmHg, the run loop, safety stop, saving. Has a demo board for practice. | 288 |
| `phantom_verify.py` | The ten-item data check. Used by the window after every run and runnable alone: `python phantom_verify.py runs/<folder>`. | 235 |
| `matlab/phantom_check.m` | The same ten checks in MATLAB, plus plots, to cross-check the Python. | 209 |
| `firmware/Phantom_HW_executor/` | The Arduino sketch: sets the pump voltage through the DAC, reads P1 and P2. Flashed once. | |
| `firmware/Phantom_HW_executor_legacy/` | The older sketch (no watchdog). Kept for reference. | |
| `CHECKLIST.md`, `README.md`, `FILES.md` | How to check a run, how to start, this table. | |
| `PhantomLab_v2/` (+ `.zip`) | The packaged app of this program: `PhantomLab.exe`, docs, MATLAB script, firmware and a demo example run. Share this. | |
| `PhantomLab_app/` (+ `.zip`) | The previous packaged app (15 Oct build). Superseded by `PhantomLab_v2`. Delete it once its window is closed. | |
| `client_package/` (+ `.zip`) | The source version of the same thing for Python users: the three `phantom_*.py` files, docs, MATLAB, firmware. | |
| `runs/<date>_<tag>/` | One folder per run: `data.csv`, `meta.json`, `report.txt`, `plot.png`. | |
| `logs/` | All older data (October and before). Not written by the current program. | |
| `archive/` | Everything below. It still runs from inside that folder (`cd archive`). | |

Only the three `phantom_*.py` files are needed to run the app.

## The data path in the current program (one number, start to end)
1. `command_samples()` builds the pump voltage: DC + AC x shape, one value per 20 ms.
2. `Board.step()` sends `STEP <volts>`; the board answers `D,t_ms,seq,raw1,ok1,raw2,ok2,pump_v` (raw ADC counts and the voltage it actually set).
3. `measure_zero()` reads 3 s with the pump off; the median counts are the zero.
4. mmHg = (counts - zero) x psi x 51.7149 / 3276 (15 psi: 0.2368 mmHg per count; 5 psi: 0.0789).
5. `save_run()` writes `data.csv` (raw counts and mmHg side by side) and `meta.json` (scale, zero, AC, DC, shape, `feedback: none`).
6. `phantom_verify.py` reads those two files back and checks them.

## `archive/`: the old code (nothing was deleted; a zip backup is in there too)
| File | What it was | Used by |
|---|---|---|
| `morphology.py` (2,609 lines) | The old engine: waveform builder, the **learning controller** (changes the command every beat until P1 matches a target), pump identification, test matrix, reference calibration, command-line tool. | `analysis`, `acdc_sweep`, old `phantom_*` windows |
| `pressure.py` (2,139 lines) | The old serial link, zero and scale code, and the first LOW/HIGH controller. `SENSOR_RANGE_PSI` was changed to 5.0 on 2026-10-15. | `morphology`, `acdc_sweep`, `analysis` (and `morphology` imports it back: a circular import) |
| `acdc_sweep.py` | Holds fixed AC/DC pairs for set times and reports the pressure each gives (the 16-minute runs). Plans: `acdc_*.csv`. | run from the terminal |
| `phantom_validation_sweep.py` (1,592 lines) | The earlier grid of LOW and HIGH voltages against 90/60, 120/80, 150/120. | run from the terminal |
| `analysis.py` | Reads old logs and prints summaries. | terminal |
| `phantom_gui.py`, `phantom_app.py`, `phantom_lab.py` | Three generations of the old windows, all driving `morphology.py`. `phantom_gui.py` also holds the command runner. | each other |
| `executor_backend.py`, `gui/gui.py` | The original lab GUI and its backend for the executor firmware. | each other |
| `new_main.py`, `new_main1.py` | The original lab main program (August). The two files differ only in the serial port (`COM11` against `COM1`). | `shared_logger` |
| `shared_logger.py`, `csv_helper.py` | Small helpers for the two items above. | |
| `util/` | Original lab helpers (voltage-sweep calibration, step-function waveform, file loaders) and sample sweeps. `pressure.py` loads `util/waveform.py`. | `pressure.py` |
| `PhantomSystemInputs/` | Sample heart-rate signal CSVs from the original lab. | `gui/gui.py` |
| `PhantomControl_app/` (+ zip) | The old packaged window (October 9 build). Do not use. | |
| `requirements.txt`, `README.md`, `OPERATING_PROCEDURE.md`, `*.csv` | Old instructions, test plans and example data. | |

## Why there were so many files
Each new idea (a new GUI, a learning controller, a sweep tool, a calibration tool) was added next to the last one instead of replacing it, and they import each other. The current program has one job: apply a voltage you type, record P1 and P2, and prove the recording is sound.
