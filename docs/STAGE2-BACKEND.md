# STAGE 2 — Backend Foundation

**Scope delivered:** data models, local file storage, password hashing, sessions, user management (Module 1), build system, and a self-test you can run on your MacBook.

Status: **code complete, awaiting your `make` run.** I cannot compile in this environment (no shell, no toolchain), so nothing here is claimed to build until you run the commands in section 6 and send me the output.

---

## 1. Files delivered

| File | Type | Purpose |
|------|------|---------|
| `backend/include/Errors.h` | header | `AppError` hierarchy → HTTP status mapping |
| `backend/include/Entity.h` | header | abstract record base: `id`, `createdAt`, `toRecord` / `fromRecord` / `describe` |
| `backend/include/Models.h` | header | `User`, `Resource`, `HelpRequest`, `Notification` + enums, validation helpers, timestamp |
| `backend/include/FileManager.h` | header | the only `fstream` in the project; atomic save, escaping, corrupt-line recovery |
| `backend/include/PasswordHasher.h` + `src/PasswordHasher.cpp` | class | PBKDF2-HMAC-SHA256 with a unique per-user salt |
| `backend/include/SessionManager.h` | header | token → user id, idle expiry, thread-safe |
| `backend/include/UserService.h` + `src/UserService.cpp` | class | **Module 1**: register, login, logout, profile, edit, auth |
| `backend/include/SelfTest.h` + `src/SelfTest.cpp` | class | 40+ automated checks of everything above |
| `backend/include/ConsoleDemo.h` + `src/ConsoleDemo.cpp` | class | your original menu, rebuilt on the service layer |
| `backend/src/main.cpp` | entry | argument parsing and start-up only |
| `backend/Makefile`, `backend/CMakeLists.txt` | build | one-command build |
| `data/.gitkeep`, `.gitignore` | config | data folder without committing records |

`FileManager.h` and `SessionManager.h` are header-only (templated / inline), so they are not separate `.cpp` files — that is deliberate, not an omission.

## 2. What changed from your original files, and why

### `CommunityPlatform.h` → `Models.h` + `Entity.h` + `UserService.h`
Your single header declared one god class with three fixed arrays. It is now split as your section 12 structure asks. Concrete differences:

| Original | Now | Reason |
|---|---|---|
| `User` had 6 fields, `string password` | 11 fields incl. `email`, `passwordHash`, `city`, `locality`, `contactPhone` | Module 1 field list; plaintext password removed |
| `User.password` plaintext | `passwordHash = "pbkdf2_sha256$120000$salt$hash"` | section 4 forbids plaintext |
| `Resource` had 5 fields | adds `availability`, `dateAdded` | Module 2 field list |
| `HelpRequest` had `message`, `status` | adds `title`, `description`, `requiredSkills`, `category`, `priority`, `acceptOwnerId`, `updatedAt` | Module 3 field list |
| `string status` | `enum class RequestStatus` | a typo cannot invent a state (defect Q5) |
| `using namespace std;` in the header | removed; `std::` everywhere | header no longer pollutes other files (Q1) |
| fixed `users[MAX_USERS]` | `std::vector<User>` | no silent 100-user ceiling (P4) |

### `UserManagement.cpp` → `UserService.cpp`
Every function is preserved in behaviour, but the console is gone and the security is real.

| Original behaviour | Now |
|---|---|
| `registerUser()` read 5 fields with `getline` | `registerUser(...)` takes 9 arguments and **returns the new id** |
| empty-username check only | name, username, email, password, city, phone all validated with specific messages |
| username unique, case-sensitive | email **and** username unique, both **case-insensitive**, via `unordered_map` (**O(1)** instead of a scan) |
| password stored and compared as plain text | PBKDF2 hash, unique salt, constant-time comparison |
| `login()` scanned the array | `login()` hashes once against the indexed record (**O(1)** lookup + one hash) |
| `loggedInUserId` int | `SessionManager` token; one process can serve many sessions |
| `viewProfile()` printed 5 fields | `getOwnProfile()` returns a `UserView` (email + phone included); `getPublicProfile()` **omits** them |
| no edit at all | `updateProfile()` — validate-all-then-commit, so a rejected field cannot half-apply an edit |
| `logout()` reset an int | `logout(token)` invalidates exactly that token |
| nothing saved | every mutation calls `persist()` → `users.txt` is written immediately |

### `main.cpp`
The 106-line menu is now ~120 lines of argument parsing plus wiring. The menu moved to `ConsoleDemo.cpp` (optional), and the menu logic itself is unchanged so you can still demo it.

### Kept from your code, deliberately
- The `-1`/`0` "not logged in" sentinel → `currentUserId == 0` in the demo.
- Duplicate-username rejection and empty-input rejection → generalised.
- Identical error message for "unknown user" and "wrong password" → kept and extended.
- Student-friendly naming and comment style.

## 3. How the file format works

`data/users.txt`:

```
#CC1
1|2026-11-13T09:41:07Z|Aditi Sharma|aditi|aditi.sharma@college.edu|pbkdf2_sha256$120000$k3J...$9fA...|Pune|Kothrud|C++, DSA|Web development, AI|+91 98765 43210
```

- Line 1 is `#CC1` — format marker and version. Lines starting with `#` are skipped on load.
- Fields are separated by `|` and escaped by `FileManager`: `\` → `\\`, `|` → `\p`, newline → `\n`, CR → `\r`. A description can therefore contain pipes, backslashes and newlines without corrupting the file. This is the "proper escaping" section 7 asks for, instead of fragile comma splitting.
- **Atomic writes:** `saveAll()` writes `users.txt.tmp`, flushes, closes, then `rename()`s it over `users.txt`. `rename` is atomic on the same filesystem, so an interrupted save leaves the *old* file intact — never a half-written one.
- **Corrupt records:** a short or malformed line is caught per-line, reported on stderr with its line number, and skipped. The rest of the file still loads. Tested in self-test section 7.
- **Missing file / empty file:** returns an empty vector and the file is created on first save. No crash on a fresh clone.

## 4. Security notes (what to say in the viva)

- **PBKDF2-HMAC-SHA256, 120 000 iterations, 16-byte random salt, 32-byte key.** Salt from the OS CSPRNG. The same password produces different hashes for two users — self-test checks this.
- **No plaintext password anywhere:** not in the file, not in a `UserView`, not in an API response. `UserView` has no `passwordHash` member at all, so it *cannot* leak by accident.
- **Session tokens:** 32 random bytes (256 bits), hex. Not derived from the user id and not sequential.
- **Idle expiry:** 120 minutes, refreshed on use. Sessions are in memory only, so restarting the server logs everyone out while the data stays on disk.
- **Timing:** wrong password vs unknown account return the same message; the derived keys are compared without an early exit.
- **Path safety:** no user-supplied string ever becomes a file path. `FileManager` only ever receives constant names (`users.txt`, …) with the data directory prefixed.
- **Honest limitations:** this is a local single-machine app. No TLS, no rate limiting, no account lockout, no CSRF token, no per-device session list. All four are listed as future improvements rather than pretended.

## 5. DSA and OOPS in this stage

| Concept | Where | Why | Complexity |
|---|---|---|---|
| `std::vector` | every store | grows past your old `MAX_*` caps | append O(1) amortised |
| `unordered_map<int, size_t>` | `UserService::idIndex_` | replaces the `O(n)` `findUserById` scan | O(1) average |
| `unordered_map<string,int>` | `emailIndex_`, `usernameIndex_` | duplicate check + login lookup | O(1) average |
| `std::string` sort order | `createdAt` ISO-8601 | timestamps sort lexicographically (used by the heap tie-break in Stage 3) | – |
| Inheritance | `Entity` → `User`/`Resource`/`HelpRequest`/`Notification` | share `id`/`createdAt` once | – |
| Polymorphism | pure virtual `toRecord`/`fromRecord`/`describe`; `AppError::code()` | one `FileManager` template stores every type | – |
| Encapsulation | all vectors/indexes private; only methods mutate | invariants hold (indexes cannot drift) | – |
| Abstraction | `FileManager` hides `fstream`, temp files, escaping | services never see a stream | – |
| Composition | `UserService` **has-a** `FileManager` and a `SessionManager` | testable, no globals | – |
| Templates | `FileManager::loadAll<T>`, `saveAll<T>` | one storage implementation, four record types | – |
| Exception hierarchy | `AppError` → 6 subclasses | services stay HTTP-agnostic | – |

## 6. How to verify on your MacBook

```bash
# 1. Check the compiler (Apple Command Line Tools)
clang++ --version          # if missing: xcode-select --install

# 2. Build
cd backend
make

# 3. Run the Stage 2 verification
./build/cc-server --selftest
```

Expected: a list of `[PASS]` lines per section (escaping, hashing, validation, persistence, authentication, profile editing, corrupt records) ending with `Passed: N   Failed: 0` and `Stage 2 foundation verified.`

Then try the console version, which uses the same services:

```bash
./build/cc-server --console
```

Register a user, exit with `0`, run it again, log in with the same credentials — that is the "data survives a restart" requirement working. Look at `data/users.txt`: no plaintext password.

If anything fails, send me the **exact** command and the full output.

## 7. Deliberately deferred to later stages

Not bugs, just not this stage's scope: `ResourceService`, `PriorityQueue`, `RequestService`, `SearchService`, `NotificationService` (Stages 3–4); `ApiRouter`/`Server` (Stage 5); the entire frontend (Stage 6); sample data seeding (Stage 8); `docs/DSA-OOPS-REPORT.md` (filled in as stages land).

## 8. Review checklist I applied before handing this over

- every declared method has a definition (listed at the end of this section);
- no `cout` outside `ConsoleDemo.cpp`; no `fstream` outside `FileManager.h` and the self-test;
- no header includes another header that would create a cycle (`Errors → Entity → Models → {FileManager, SessionManager, UserService}`);
- no `using namespace std;` in any header;
- all compiler-warning risks addressed (`-Wall -Wextra -Wpedantic` clean by inspection: no unused parameters, signed/unsigned casts explicit);
- `updateProfile` cannot half-apply a change;
- `nextUserId_` is derived from the maximum stored id, so a restart cannot reissue an id;
- the self-test runs in `build/selftest-data/` and deletes it afterwards — your real `data/` is never touched;
- `--selftest` runs **before** `FileManager` is constructed, so it cannot even create a stray `data/` folder.

**Known follow-up I will fix if the compiler disagrees:** this code has not been through a real compiler yet. Send me the first error output and I will correct it in one pass rather than guessing.
