# noren

noren (暖簾) — the split fabric curtain hung in Japanese shop doorways.
It doesn't stop you. It makes entering a deliberate act.

A screen time tracker for Windows. Tracks which application has focus and for how long. Local-only, no network, no account required.

## What it does

- Tracks active window focus in real time
- Records time spent per application in a local SQLite database
- Shows a live dashboard with daily totals, weekly activity, and per-app breakdown
- Provides a CLI for querying usage data

## What it does not do

- It does not phone home. There is no network code in this project.
- It is not a parental control tool.
- It does not block or restrict anything (yet — enforcement is planned for v0.2).

## Building

Requires Qt 6.5+ and CMake.

```
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/qt
cmake --build build
```

## Usage

Run `noren.exe`. A tray icon will appear. The dashboard opens from the tray menu.

To query usage from the command line:

```
noren-cli report
```

## Privacy

noren records the title of every window you focus. That is a diary of everything you read, wrote, and searched.

- All data is stored locally in `%APPDATA%/noren/noren.db`
- No network connections are made
- Data can be deleted by removing the database file

## Status

Early. v0.1 — tracking only. No enforcement, no limits, no blocking.

## License

MIT
