# macOS Setup and Execution Guide

## 1. Install Apple's command-line developer tools

```bash
xcode-select --install
```

If they are already installed, the command reports that fact.

## 2. Check the compiler

```bash
clang++ --version
make --version
cmake --version
```

Only `clang++` and `make` are required for the simplest build. CMake is optional.

## 3. Open the project

In Terminal:

```bash
cd /path/to/Community-Connect/backend
```

## 4. Build

```bash
make clean
make
```

The executable is created at `backend/build/cc-server`.

## 5. Optional self-test

```bash
./build/cc-server --selftest
```

A successful run ends with:

```text
Passed: 136   Failed: 0
```

## 6. Optional demo data

On a new/empty data directory:

```bash
./build/cc-server --seed-demo
```

Then start the server:

```bash
./build/cc-server
```

## 7. Open the website

Open `http://127.0.0.1:8080/` in a browser.

## 8. Stop the server

Press `Ctrl+C` in the Terminal running the server.

## 9. Restart

Run the same server command again. Existing `.txt` records are loaded automatically.

## 10. If port 8080 is busy

```bash
./build/cc-server --port 8090
```

Then open `http://127.0.0.1:8090/`.

## 11. If paths are different

Use explicit paths:

```bash
./build/cc-server \
  --data /absolute/path/to/Community-Connect/data \
  --frontend /absolute/path/to/Community-Connect/frontend
```
