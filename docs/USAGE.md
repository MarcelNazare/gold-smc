# gold-smc — Usage Guide

Practical guide to installing, configuring, and running the **Liquidity Sweep → 5m CHoCH → FVG** strategy on Gold (XAU/USD) 5-minute candles from Twelve Data.

> **Not financial advice.** This is research tooling. Paper-trade or backtest before any live use.

---

## Table of Contents

1. [What the program does](#1-what-the-program-does)
2. [Prerequisites](#2-prerequisites)
3. [Building](#3-building)
4. [Getting a Twelve Data API key](#4-getting-a-twelve-data-api-key)
5. [Configuring the API key (`.env`)](#5-configuring-the-api-key-env)
6. [Configuring the strategy (`config/config.json`)](#6-configuring-the-strategy-configconfigjson)
7. [Running the program](#7-running-the-program)
8. [Reading the output](#8-reading-the-output)
9. [Trading the signal — checklist](#9-trading-the-signal--checklist)
10. [Tuning the strategy](#10-tuning-the-strategy)
11. [Troubleshooting](#11-troubleshooting)
12. [Rate limits and cost](#12-rate-limits-and-cost)
13. [File reference](#13-file-reference)

---

## 1. What the program does

For each invocation it:

1. Fetches the last N 5-minute candles for **XAU/USD** from Twelve Data.
2. Scans for a **liquidity sweep** — a wick that pierces a recent swing high/low but closes back inside.
3. Waits for a **5-minute Change of Character (CHoCH)** — a close that breaks the most recent opposite swing after the sweep.
4. Looks for a **Fair Value Gap (FVG)** left by the impulse leg that caused the CHoCH.
5. If all three line up within the configured tolerances, prints a **LONG** or **SHORT** signal with entry, stop, target, and the candle indices of the sweep/CHoCH/FVG.

If any condition fails, it prints `No sweep + CHoCH + FVG setup found` and exits with code `0`. The program does not place trades — it is a signal generator.

---

## 2. Prerequisites

| Tool | Version | Notes |
|---|---|---|
| **MinGW-w64 (UCRT)** | GCC 13+ | Required. `C:\mingw64\bin\g++.exe` is the default path. |
| **CMake** | 3.21+ | Must be on `PATH`. |
| **Ninja** | any | Must be on `PATH`. Or drop `-G Ninja` and use the default generator. |
| **vcpkg** | latest | The script expects it at `C:\dev\vcpkg` unless overridden. |
| **Twelve Data key** | free tier OK | See §4. |

Verify:

```cmd
g++   --version
cmake --version
ninja --version
```

If any of those fail, install it before continuing.

---

## 3. Building

### Quick build (from project root)

```cmd
cd C:\Users\marcel\Documents\Projects\gold-smc
build.bat
```

That runs **configure (first time only) → build → tests** and leaves the binary at `build\gold_smc.exe`.

### Common variants

```cmd
build.bat clean              :: wipe build\, reconfigure, build, test
build.bat debug              :: Debug build into build-debug\
build.bat build              :: build only, skip tests
build.bat test               :: run tests only
build.bat run                :: build then execute once
build.bat --help             :: list everything
```

### Overriding paths (one-shot)

```cmd
set MINGW_BIN=C:\msys64\ucrt64\bin
set VCPKG_ROOT=C:\tools\vcpkg
set JOBS=8
build.bat clean
```

### Overriding paths (permanent, per-user)

```cmd
setx MINGW_BIN  "C:\msys64\ucrt64\bin"
setx VCPKG_ROOT "C:\tools\vcpkg"
```

Reopen the terminal afterwards — `build.bat` picks them up automatically.

### Makefile alternative (MSYS2 / Git Bash / WSL)

```bash
make            # configure + build + test
make rebuild    # clean + build + test
make debug      # Debug into build-debug/
make dist       # stage a portable exe into dist/
```

Both `build.bat` and `Makefile` wrap the same CMake build — pick whichever fits your shell.

### Manual CMake build

```cmd
cmake -B build -S . -G Ninja ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_CXX_COMPILER="C:/mingw64/bin/g++.exe" ^
      -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake ^
      -DVCPKG_TARGET_TRIPLET=x64-mingw-static ^
      -DVCPKG_HOST_TRIPLET=x64-mingw-static

cmake --build build -j
```

> The **first** build compiles `libcurl` and OpenSSL from source via vcpkg. It can take 15–25 minutes. Subsequent builds reuse the vcpkg cache and take a few seconds.

---

## 4. Getting a Twelve Data API key

1. Sign up: <https://twelvedata.com/pricing>
2. Open your dashboard → **API Keys**
3. Copy the key (a 32-character string)

The free tier is enough for personal use: **8 requests/minute, 800 requests/day**.

---

## 5. Configuring the API key (`.env`)

The program looks for `.env` in the **current working directory**, then one level up (`../.env`). The file must contain:

```env
TWELVEDATA_API_KEY=your_key_here
```

Create it:

```cmd
cd C:\Users\marcel\Documents\Projects\gold-smc
copy .env.example .env
notepad .env
```

Paste the real key after the `=`, save, exit.

### ⚠️ Avoid the `.env.txt` trap

Windows hides file extensions by default, so if you save from Notepad with "File type: Text Documents" you will silently get `.env.txt`. The C++ loader opens exactly `.env` — nothing else.

Check:

```cmd
dir /a .env
```

If you see `.env.txt`:

```cmd
ren .env.txt .env
```

And once, permanently, tell Explorer to show extensions:

```cmd
reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced" ^
    /v HideFileExt /t REG_DWORD /d 0 /f
```

### Verify the key loads

```cmd
build\gold_smc.exe
```

Expected first log line:

```
[INFO] Loaded 1 env var(s) from .env
```

If you see `Loaded N env var(s) from ../.env` it means you launched from `build\` — that's fine, the fallback found it. If you see `No .env file at .env or ../.env`, see §11.

---

## 6. Configuring the strategy (`config/config.json`)

All strategy knobs live here. Edit as needed; changes take effect on the next run.

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

| Field | Meaning |
|---|---|
| `symbol` | Twelve Data symbol. `XAU/USD` is gold spot. |
| `interval` | Candle interval. The strategy is designed for `5min`. |
| `output_size` | How many candles to fetch. 300 = ~25 hours. |
| `swing_lookback` | Candle window on each side to declare a swing high/low. 2 is standard. |
| `risk_reward` | Target multiplier over risk. `2.0` = 1:2 RR. |
| `min_fvg_size` | Minimum FVG height in USD. Filters tiny imbalances. `0.20` = $0.20 on gold. |
| `stop_buffer_usd` | Extra space beyond the sweep wick for the stop. |
| `sweep_lookback_candles` | Only consider sweeps that happened within this many candles of "now". |
| `max_candles_sweep_to_choch` | Reject if the CHoCH takes too long to appear after the sweep. |
| `max_candles_choch_to_fvg` | Reject if the FVG is too far from the CHoCH. |
| `session_filter_enabled` | Currently parsed but not enforced — leave `false`. |
| `allowed_utc_hours` | Reserved for the session filter. |

Run the program with a different config file:

```cmd
build\gold_smc.exe config\my-config.json
```

---

## 7. Running the program

### One-shot (recommended)

```cmd
cd C:\Users\marcel\Documents\Projects\gold-smc
build.bat run
```

Or from a shell where `.env` is already exported:

```cmd
set TWELVEDATA_API_KEY=your_key_here
build\gold_smc.exe
```

### Continuous — every 5 minutes

**Windows (PowerShell):**

```powershell
.\scripts\run_loop.ps1
```

**Windows (cmd.exe):**

```cmd
:loop
build.bat run
timeout /t 300 /nobreak
goto loop
```

**MSYS2 / Git Bash:**

```bash
INTERVAL=300 ./scripts/run_loop.sh
```

300 seconds between runs is safe for the free tier (8 req/min → 1 req every 8s max; 5min interval = 12 req/hour).

### Redirect output to a log

```cmd
build\gold_smc.exe > logs\run.log 2>&1
```

---

## 8. Reading the output

### When a signal is found

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

Field meanings:

| Line | Meaning |
|---|---|
| `LONG` / `SHORT` | Direction of the setup. |
| `Entry` | Price level of the FVG retest — where you'd place a limit order. |
| `Stop` | Sweep level ± `stop_buffer_usd`. |
| `Target` | Entry ± `risk_reward × risk`. |
| `R:R` | The configured risk-reward ratio. |
| `Sweep` | Price level swept + the candle index in the fetched window (0 = oldest). |
| `CHoCH` | Index of the candle that confirmed the reversal. |
| `FVG` | Index of the first candle of the 3-candle FVG pattern. |
| `Reason` | Human-readable summary. |

If price has **already** passed through the entry (e.g. a LONG whose last close is below `Entry`), you'll see a warning:

```
[WARN] Price is already below the FVG entry — retest may have passed.
```

That means the setup is stale — skip it.

### When no signal is found

```
No sweep + CHoCH + FVG setup found on the latest data.
```

Exit code `0`. Normal. Most runs will produce no signal — that's the point.

### Exit codes

| Code | Meaning |
|---|---|
| `0` | Ran successfully (signal **or** no signal). |
| `1` | Fatal error (missing API key, HTTP failure, parse error). |
| `2` | Too few candles returned by the API. |

Use them in scripts: `build\gold_smc.exe && echo "ran ok"`.

---

## 9. Trading the signal — checklist

Before acting on any signal the tool emits:

- [ ] **Confirm direction on a higher timeframe.** A bullish 5m setup inside a 4H downtrend is far less reliable.
- [ ] **Verify the FVG on your broker's chart.** Twelve Data aggregates differently from your broker; a $0.10–0.20 difference is normal, but the gap must still exist.
- [ ] **Check the session.** Signals inside the London open (07:00–11:00 UTC) or New York open (12:00–16:00 UTC) are usually higher quality.
- [ ] **Avoid the 5 minutes before and after a red-folder news release.** The sweep will just be news noise.
- [ ] **Respect the stop.** The stop is at the sweep wick — if that level breaks, the whole thesis is invalid.
- [ ] **Size the position from the stop distance**, not from a fixed lot size.

---

## 10. Tuning the strategy

| Symptom | Tuning |
|---|---|
| Too many signals, most losing | Increase `min_fvg_size` (e.g. `0.30`) and reduce `max_candles_sweep_to_choch` (e.g. `8`). |
| Too few signals | Decrease `min_fvg_size` (e.g. `0.10`) and increase `max_candles_sweep_to_choch` (e.g. `20`). |
| Signals look stale by the time they print | The FVG was filled before you read the output. Run more frequently (`INTERVAL=60`), or set `sweep_lookback_candles` to `10`. |
| Signals on the wrong side of a bigger trend | Add a 4H filter (out of scope of the current code) or manually check the 4H chart before acting. |

Always re-run the unit tests after changes to `src/strategy.cpp`:

```cmd
build.bat test
```

---

## 11. Troubleshooting

### `[ERROR] TWELVEDATA_API_KEY is not set (env or .env)`

The `.env` file wasn't found. Diagnose in this order:

```cmd
:: 1. Are you in the project root?
cd

:: 2. Is the file really named .env, not .env.txt?
dir /a .env*

:: 3. What's inside?
type .env
```

Fixes:
- `ren .env.txt .env` if #2 shows `.env.txt`.
- `cd C:\Users\marcel\Documents\Projects\gold-smc` if #1 shows a different folder.
- If `type .env` shows nothing → the file is empty; re-create it.
- If you see `ï»¿TWELVEDATA...` at the start → BOM saved by Notepad; the loader strips it now, but you can also `notepad .env` → **Save As** → **Encoding: UTF-8** (not "UTF-8 with BOM").

### `[WARN] No .env file at .env or ../.env`

Both paths were tried and neither existed. Usually means the file is `.env.txt` — see above.

### `The CXX compiler identification is Clang` after switching to MinGW

`build\CMakeCache.txt` still remembers the old compiler. Delete and reconfigure:

```cmd
rmdir /s /q build
build.bat
```

### `undefined reference to curl_easy_init`

You're linking against a DLL import lib (wrong vcpkg triplet). Ensure `-DVCPKG_TARGET_TRIPLET=x64-mingw-static` is passed. `build.bat` does this automatically.

### `libstdc++-6.dll was not found` when running the exe

You built without `-static-libstdc++`. Rebuild with `build.bat clean`. The CMakeLists already sets `-static -static-libgcc -static-libstdc++` in the MinGW branch.

### HTTP 429 from Twelve Data

You hit the free-tier rate limit. Increase `INTERVAL` (e.g. 600s) or upgrade your Twelve Data plan.

### `no matching function for call to 'std::getline(...)'` or similar C++ errors

`include/goldsmc/utils.hpp` has been overwritten with implementations. It must contain **only** declarations. See the "utils.hpp reminder" in the previous fix — the header is ~13 lines, the `.cpp` is ~130.

### `error STL1000: Unexpected compiler version, expected Clang 19.0.0 or newer`

You're using an old Clang against a newer MSVC STL. Switch to MinGW as documented in §3.

---

## 12. Rate limits and cost

| Plan | Requests / minute | Requests / day | Cost |
|---|---|---|---|
| Free | 8 | 800 | $0 |
| Grow | 55 | 8,000 | paid |
| Pro | 610 | unlimited | paid |

- A single `gold_smc.exe` run = **1 request**.
- At 300s intervals that's **288 requests/day** — comfortably inside the free tier.
- At 60s intervals that's **1,440/day** — you'll hit the 800 cap. Space runs accordingly.

The tool does **not** retry on 429 — it exits with code `1` and prints the API error message.

---

## 13. File reference

```
gold-smc\
├── build.bat               Windows build script (cmd.exe)
├── Makefile                POSIX wrapper (MSYS2 / Git Bash / WSL)
├── CMakeLists.txt          CMake build definition
├── vcpkg.json              vcpkg manifest (curl, nlohmann-json)
├── .env                    YOUR API KEY (gitignored)
├── .env.example            Template
├── USAGE.md                This file
├── README.md               Project overview
│
├── config\
│   └── config.json         Strategy parameters
│
├── include\goldsmc\
│   ├── types.hpp           Candle, Swing, FVG, Signal
│   ├── config.hpp          StrategyConfig
│   ├── utils.hpp           getEnv, loadDotEnv, logging, urlEncode, nowIsoUtc
│   ├── twelvedata.hpp      fetchTimeSeries
│   ├── indicators.hpp      findSwings
│   └── strategy.hpp        detectLiquiditySweep, detectCHoCH, findFVG, runStrategy
│
├── src\
│   ├── main.cpp            Entry point
│   ├── config.cpp          config.json loader
│   ├── utils.cpp           Implementations of utils.hpp
│   ├── twelvedata.cpp      HTTP + JSON parsing
│   ├── indicators.cpp      Implementations of indicators.hpp
│   └── strategy.cpp        Implementations of strategy.hpp
│
├── scripts\
│   ├── run_once.sh/.ps1/.bat     Run once from project root
│   └── run_loop.sh/.ps1          Loop every INTERVAL seconds
│
├── tests\
│   ├── test_framework.hpp        Tiny test harness
│   ├── test_main.cpp             Runner
│   ├── test_indicators.cpp       Swing & FVG unit tests
│   └── test_strategy.cpp         Sweep/CHoCH/runStrategy tests
│
└── build\                  (gitignored) CMake output + gold_smc.exe
```

---

## Quick reference card

```cmd
:: First-time build
cd C:\Users\marcel\Documents\Projects\gold-smc
build.bat

:: Set up the API key (once)
copy .env.example .env
notepad .env                     :: paste your key, save

:: Run once
build.bat run

:: Rebuild after code change
build.bat build

:: Run tests
build.bat test

:: Clean rebuild
build.bat clean

:: Continuous every 5 minutes (PowerShell)
.\scripts\run_loop.ps1
```