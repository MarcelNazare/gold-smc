# gold-smc

A C++20 command-line tool that scans **Gold (XAU/USD)** 5-minute candles from **Twelve Data** for a specific ICT/SMC setup:

> **liquidity sweep → 5-minute Change of Character (CHoCH) → Fair Value Gap (FVG)**

When all three line up within configurable tolerances, it prints a LONG or SHORT signal with entry, stop, target, and the candle indices of the sweep / CHoCH / FVG. It does not place trades — it's a signal generator you point your own execution at.

---

## The idea in one paragraph

Price sweeps an obvious high or low to grab stops (liquidity). Then it sharply reverses, and its first close beyond the prior swing high (or low) proves the reversal is real — that's the **CHoCH**. The impulse leg of that reversal usually leaves a **Fair Value Gap**: a three-candle imbalance where price moved so fast it left a gap. You enter on the retest of the FVG, stop beyond the sweep wick, and target 1:2 (configurable) risk-reward.

Full strategy walkthrough, config reference, and troubleshooting live in **[`USAGE.md`](USAGE.md)**.

---

## Quick start (Windows, MinGW + vcpkg)

```cmd
:: 1. Clone and enter the project
git clone <repo-url> gold-smc
cd gold-smc

:: 2. Get a free Twelve Data API key
::    https://twelvedata.com/pricing

:: 3. Configure your API key
copy .env.example .env
notepad .env                         :: paste the key after TWELVEDATA_API_KEY=

:: 4. Build (first build compiles curl + OpenSSL via vcpkg — ~15 min)
build.bat

:: 5. Run once
build.bat run
```

If the last command prints a signal, you're done. If it prints "No sweep + CHoCH + FVG setup found", that's normal — most runs produce no signal. Set up a loop for continuous scanning (see below).

**Expected output when a signal fires:**

```
=== SIGNAL (2026-11-25T14:20:11Z) ===
LONG  XAU/USD  5min
Entry   : 2655.30
Stop    : 2651.20
Target  : 2663.50
R:R     : 1:2.00
Sweep   : level 2651.50 at candle #213
CHoCH   : candle #217
FVG     : candle #215
Reason  : Bullish sweep + 5m CHoCH + bullish FVG
```

---

## Requirements

- **C++20 compiler** — MinGW-w64 (UCRT) 13+ recommended on Windows. GCC/Clang on Linux/macOS also work.
- **CMake** 3.21+
- **Ninja** (or any generator)
- **vcpkg** — `C:\dev\vcpkg` is the default path; override with `VCPKG_ROOT`.
- A **Twelve Data** API key (free tier is enough).

---

## Platform support

| Platform | Build | Run |
|---|---|---|
| **Windows** (MinGW-w64 UCRT) | `build.bat` | `build\gold_smc.exe` |
| **Windows** (MSYS2 UCRT64 / Git Bash) | `make` | `make run` |
| **Linux / macOS** | `make` | `./build/gold_smc` |

The Windows build produces a **fully static `gold_smc.exe`** (~4–6 MB, no DLLs). You can copy it to any Windows 10+ machine and it just runs.

Detailed install instructions for each platform: see [`USAGE.md` §3](USAGE.md#3-building).

---

## Common commands

```cmd
build.bat              :: configure (once) + build + test
build.bat run          :: build + run once
build.bat test         :: run unit tests only
build.bat debug        :: Debug build into build-debug\
build.bat clean        :: wipe build\ and rebuild everything
build.bat --help       :: list every target
```

```bash
make                   # same, from MSYS2 / Git Bash / WSL
make debug
make dist              # stage a portable exe in dist/
```

Continuous scanning every 5 minutes:

```powershell
.\scripts\run_loop.ps1
```

or in `cmd.exe`:

```cmd
:loop
build.bat run
timeout /t 300 /nobreak
goto loop
```

The default 300-second interval keeps you at ~288 requests/day, well within the free-tier 800/day cap.

---

## Configure

Two files control everything:

**`.env`** — the API key:

```env
TWELVEDATA_API_KEY=your_key_here
```

**`config/config.json`** — the strategy:

```json
{
  "symbol": "XAU/USD",
  "interval": "5min",
  "output_size": 300,
  "swing_lookback": 2,
  "risk_reward": 2.0,
  "min_fvg_size": 0.20,
  "stop_buffer_usd": 0.30,
  "sweep_lookback_candles": 30,
  "max_candles_sweep_to_choch": 15,
  "max_candles_choch_to_fvg": 10,
  "session_filter_enabled": false,
  "allowed_utc_hours": [[7, 11], [12, 16]]
}
```

Every field is documented in [`USAGE.md` §6](USAGE.md#6-configuring-the-strategy-configconfigjson). If you don't touch either file, defaults are sensible.

---

## Project layout

```
gold-smc\
├── build.bat               Windows build script (cmd.exe)
├── Makefile                POSIX build wrapper
├── CMakeLists.txt          CMake build definition
├── vcpkg.json              curl + nlohmann-json manifest
├── USAGE.md                Full user guide
├── README.md               This file
│
├── config\config.json      Strategy parameters
│
├── include\goldsmc\        Public headers (types, config, utils,
│                           twelvedata, indicators, strategy)
├── src\                    Implementations + main.cpp
├── tests\                  Unit tests (deterministic, offline)
└── scripts\                run_once / run_loop wrappers (.bat/.ps1/.sh)
```

Every `.cpp` in `src/` is one module — `twelvedata.cpp` is the HTTP client, `indicators.cpp` is swing detection, `strategy.cpp` is the sweep/CHoCH/FVG logic. They compose into a static library `goldsmc_core`, so the same code can be reused by a backtester or a live executor later without duplication.

---

## Tests

```cmd
build.bat test
```

Runs a small hand-rolled test harness (`tests/test_framework.hpp`) — no external dependency. Cases cover swing detection, bullish and bearish FVG detection, FVG minimum-size filtering, sweep detection, CHoCH triggering, and end-to-end `runStrategy` returning a LONG signal with the correct 1:2 RR.

The tests feed synthetic candles through the strategy, so they run **offline** — no API key, no rate limit.

---

## How it works (short version)

The strategy is three detectors chained together in `src/strategy.cpp`:

1. **`detectLiquiditySweep`** — for each recent swing high/low, checks whether a later candle wicked past it and then closed back inside. If yes, records the sweep level and index.
2. **`detectCHoCH`** — after the sweep, walks forward candle-by-candle looking for the first close that breaks the most recent opposite swing (a swing high for bullish reversal, a swing low for bearish).
3. **`findFVG`** — in the window between the sweep and CHoCH, looks for the 3-candle imbalance that satisfies `c[i].high < c[i+2].low` (bullish) or `c[i].low > c[i+2].high` (bearish), and exceeds `min_fvg_size`.

If all three succeed with the right tolerances, `runStrategy` returns a `Signal` with entry at the FVG edge, stop beyond the sweep wick, and target = entry ± `risk_reward × risk`.

Full explanation, including why the tolerances matter: [`USAGE.md` §1](USAGE.md#1-what-the-program-does).

---

## Rate limits

Free tier: **8 requests/min, 800/day**. A single run = 1 request. At 300-second intervals that's ~288/day. Increase `INTERVAL` if you need to stay under.

The tool does **not** retry on 429 — it exits with code `1` and prints the API error. Wrap your loop with a longer sleep if you're on the free tier and running more often.

---

## Troubleshooting

The two errors you're most likely to hit:

| Symptom | Fix |
|---|---|
| `[ERROR] TWELVEDATA_API_KEY is not set` | The `.env` file is missing, empty, or (most often) actually named `.env.txt`. Run `dir /a .env*` in the project root. |
| `The CXX compiler identification is Clang` after switching to MinGW | `build\CMakeCache.txt` remembers the old compiler. `rmdir /s /q build` and rebuild. |

Everything else — including the `STL1000` Clang/STL mismatch, `.env` BOM issues, and rate-limit strategies — is covered in [`USAGE.md` §11](USAGE.md#11-troubleshooting).

---

## Disclaimers

This is **research and educational software**. It is **not financial advice**, not a trading system, and not something to point at a live account without understanding every line.

- Backtest on your own data before paper trading.
- Paper trade before live.
- The signals are only as good as the data Twelve Data returns. Your broker's feed will differ by small amounts — always confirm the setup on your execution chart.
- Past performance of a liquidity sweep + CHoCH + FVG setup does not guarantee future results.

---

## License

Released under the [MIT License](LICENSE).

You are free to use, modify, and distribute this software for any purpose,
including commercial, provided the copyright notice and license text are
preserved. The software is provided "as is", without warranty of any kind —
see [`LICENSE`](LICENSE) for the full text.

---

## See also

- **[`USAGE.md`](docs/USAGE.md)** — install, config, troubleshooting, trading checklist, rate limits, file reference
- **[`config/config.json`](config/config.json)** — the only file you need to edit to tune the strategy
- **[`tests/`](tests/)** — the strategy's expected behavior, expressed as assertions