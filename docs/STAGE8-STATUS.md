# Stage 8 - Final Verification Status

Community Connect is now packaged as a locally runnable C++17 web application.

## Verified

- `make` build succeeds on the available GNU C++17 toolchain.
- CMake build succeeds.
- Backend self-test: **136 passed, 0 failed**.
- HTTP smoke tests verified health, static frontend serving, registration, login, profile, categories, resource creation, help-request creation, request acceptance, notifications, skill matching and contact privacy.
- Contact details return HTTP 403 before acceptance and become available after acceptance.
- Demo seed is non-destructive: it only seeds an empty data directory and skips when existing data is present.
- All frontend JavaScript files pass `node --check`.

## macOS runtime

From the `backend` directory:

```bash
make
./build/cc-server --seed-demo
./build/cc-server
```

Then open:

`http://127.0.0.1:8080/`

If the backend is started from another working directory, pass explicit paths:

```bash
./build/cc-server --data /path/to/Community-Connect/data --frontend /path/to/Community-Connect/frontend
```

## Demo accounts

Only created by `--seed-demo` when the data directory is empty:

- aman / demo1234
- riya / demo1234
- kabir / demo1234

The seed command never overwrites existing records.

## Storage

Permanent data remains in local text files:

- `data/users.txt`
- `data/resources.txt`
- `data/requests.txt`
- `data/notifications.txt`

Sessions are intentionally memory-only and expire after the configured idle period.

## External dependencies

The final implementation uses POSIX sockets for the local HTTP server and does not require Node.js, Python, a database server, Firebase, Supabase, MySQL or SQLite.

The password implementation uses macOS CommonCrypto on Apple platforms and OpenSSL on non-Apple platforms.
