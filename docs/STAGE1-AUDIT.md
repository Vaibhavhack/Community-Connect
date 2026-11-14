# STAGE 1 — Existing Code Audit and Architecture

**Project:** Community Resource and Help Exchange Platform
**Brand:** Community Connect
**Delivery approach:** C — existing code kept as the reference, useful logic preserved, restructured into a clean integrated architecture.
**Date:** 13 November 2026

---

## 0. What was inspected

All six uploaded files were downloaded in full (not the truncated chat preview) and read line by line. They are preserved unmodified in `audit/uploaded/` so every later stage can be diffed against the original.

| # | File | Size | Lines | Role in original |
|---|------|------|-------|------------------|
| 1 | `CommunityPlatform.h` | 1,585 B | 87 | All structs + one god class declaration |
| 2 | `main.cpp` | 2,645 B | 106 | Console menu loop + `main()` |
| 3 | `UserManagement.cpp` | 3,059 B | 135 | Constructor, register, login, profile, logout |
| 4 | `ResourceManagement.cpp` | 1,834 B | 75 | find/show/add/list resources |
| 5 | `HelpRequests.cpp` | 4,625 B | 200 | submit / view / manage requests |
| 6 | `SearchMatching.cpp` | 1,516 B | 70 | `containsText` + `searchResources` |
| | **Total** | **15,264 B** | **~673** | |

Verdict: this is a **working console prototype**, roughly 25 % of the required feature set. It compiles as a single-target program in principle, has one clear class, and its search and authorization *ideas* are worth keeping. It has **no persistence, no HTTP server, no JSON, no notifications, no priority queue, no password hashing, and no edit/delete anywhere.** It cannot become the required application by patching; it has to be re-layered — but the good parts survive (Section 3).

---

## 1. What the existing code actually does (behaviour audit)

### 1.1 `CommunityPlatform.h` — one class, five modules
`CommunityPlatform` holds everything: three fixed C arrays (`users[100]`, `resources[200]`, `requests[300]`), three counters, three ID counters and `loggedInUserId`. Three structs (`User`, `Resource`, `HelpRequest`) live at global scope.

**Structure verdict.** It is a *god class*: user logic, resource logic, request logic and search all hang off one type. The five required modules (user / resource / request / search+match / notification) are not separated, so no module can be tested, reviewed or marked independently — and Module 5 does not exist at all.

The three private `findXById()` helpers are the only cross-cutting utility, and every one of them is a **linear scan** over the full array: `O(n)` per lookup.

### 1.2 `main.cpp` — menu only
`run()` prints a menu that changes shape depending on `loggedInUserId`, reads an `int`, and dispatches through two `switch` blocks. `main()` constructs the object and calls `run()`.

It works, and the input-failure handling (`cin.clear()`, `cin.ignore()`) is correct. But **all decisions are `cout`/`cin`**: business logic and console UI are fused inside the same functions. That is the root architectural problem — none of these functions can be called by an HTTP handler, because each one prints prompts and blocks on `getline`. Splitting them is the core of this restructure.

`main()` also never loads or saves anything, so **every record dies when the program exits**.

### 1.3 `UserManagement.cpp`
- Constructor zeroes counters and sets `loggedInUserId = -1`. No file load.
- `registerUser()`: reads name, username, password, skills, interests. Rejects an empty username and rejects a duplicate username (exact, case-sensitive match). Assigns `nextUserId++`.
- `login()`: linear scan comparing `username` **and plaintext `password`**.
- `viewProfile()`: prints ID, name, username, skills, interests. That is all — **there is no edit path**, although the spec requires "view and edit personal profile".
- `logout()`: resets to `-1`.

**Preserved ideas:** the login/duplicate/empty-input guards and the `-1` sentinel convention are sound and are kept.

### 1.4 `ResourceManagement.cpp`
- `findResourceById()` — linear scan.
- `showResource(index)` — prints ID, title, category, description, and resolves the owner's *name* through `findUserById`. Note it prints only the name, never an email or phone, so the original is accidentally safe on contact privacy.
- `addResource()` — title (non-empty check only), free-text category, description. `ownerId = loggedInUserId`.
- `listResources()` — prints all.

**Gap:** no edit, no delete, no availability status, no date added, no category validation, no filter.

### 1.5 `HelpRequests.cpp`
- `submitRequest()` — requires login, rejects a full array, lists resources, reads a resource ID, rejects your own resource, takes a free-text `message`, sets `status = "Pending"`.
- `viewMyRequests()` — prints requests where `requesterId == loggedInUserId`.
- `manageRequests()` — prints requests whose resource is owned by the logged-in user, then lets them set status to Pending / Accepted / Rejected / Completed.

**This file contains the single best piece of authorisation logic in the whole codebase:** before writing a status, it re-resolves the request's resource and verifies `resources[resourceIndex].ownerId == loggedInUserId`. That "verify ownership at the point of mutation, not at the point of display" pattern is exactly right and is carried forward into the new services.

**Defects here:**
- No priority, no priority queue (the compulsory DSA requirement is entirely absent).
- No title, description, required skills, category, or creation date on a request.
- The owner may move a request **backwards** (Completed → Pending) and may self-assign any status.
- The requester cannot edit, cancel, or complete anything — only the owner can act.
- Nothing generates a notification.
- Deleting a resource would leave requests pointing at a resource ID that no longer exists (dangling reference); there is no cascade or guard.

### 1.6 `SearchMatching.cpp`
- `containsText(text, keyword)` — hand-rolled case-insensitive substring search, `O(n·m)` worst case, with manual `'A'`–`'Z'` folding. Both arguments are passed **by value** (a full copy each call).
- `searchResources()` — scans every resource, matches keyword against title / category / description, prints hits.

**Preserved:** the manual folding approach is genuinely useful for a viva ("why not `strcasestr`?") and is kept as a documented naive-search algorithm with the same semantics, refactored into a reusable utility with `const string&` parameters.

**Gap:** no search over a resource's *owner skills*, no relevance scoring, no descending-order ranking, no user-by-skill search, no "who can help with this request" matching. Module 4's most important features are missing.

---

## 2. Defect register

Severity: **S1** blocks a required feature or loses data · **S2** requirement not met · **S3** quality/correctness risk.

### 2.1 Data loss and persistence (all S1)
| ID | File | Defect |
|----|------|--------|
| P1 | all | **No file I/O whatsoever.** No `fstream` anywhere in the six files. Requirement §7 is 0 % implemented. |
| P2 | `main.cpp` | No load at startup, no save at exit, no save after any mutation. |
| P3 | `UserManagement.cpp` | Constructor initialises counters to 0 instead of loading `users.txt`. |
| P4 | `CommunityPlatform.h` | Fixed arrays `[100]/[200]/[300]` — records beyond the cap are silently refused (fails "handle growth" and conflicts with a 300-request demo dataset). |

### 2.2 Missing modules and features (S1/S2)
| ID | Requirement | Status in existing code |
|----|-------------|-------------------------|
| M1 | Email, password hash, salt, city, locality on User | **Absent** — `User` has 6 fields, one of which is a plaintext password |
| M2 | Edit own resource / delete own resource | **Absent** |
| M3 | Availability status, date added on Resource | **Absent** |
| M4 | Category must be one of 9 educational categories | **Absent** — free text |
| M5 | Request title / description / required skills / category / priority / created date | **Absent** — only `message` |
| M6 | Priority queue (High > Medium > Low, FIFO within a level) | **Absent** |
| M7 | Requester edits/cancels own pending request | **Absent** |
| M8 | Contact details revealed only after acceptance | **Absent** (no contact fields exist at all) |
| M9 | Notification module (5 notification types) | **Absent** — no struct, no file, no code |
| M10 | Skill-based user matching and ranked results | **Absent** |
| M11 | Local HTTP server + JSON API | **Absent** |
| M12 | Frontend (14 pages) | **Absent** — no HTML/CSS/JS at all |
| M13 | Profile edit | **Absent** |

### 2.3 Security (S1/S2)
| ID | Defect |
|----|--------|
| S1-1 | Passwords stored and compared **in plaintext**. Requirement §4 explicitly forbids this. |
| S1-2 | No salt, no hash function, no iteration count. |
| S1-3 | No session token; auth state is a process-global `int`. One process = one user, so it can never serve a browser. |
| S1-4 | No backend input validation beyond empty-string checks (no email shape, no length limits, no control-character rejection). |
| S1-5 | Identical error path for "no such user" and "wrong password" is good practice — the only security nicety present. |

### 2.4 Correctness and quality (S3)
| ID | File | Defect |
|----|------|--------|
| Q1 | `CommunityPlatform.h` | `using namespace std;` at global scope in a header — pollutes every translation unit that includes it. |
| Q2 | `CommunityPlatform.h` | No destructor, no copy control; `run()` is public so anyone can launch a second menu loop. |
| Q3 | `ResourceManagement.cpp` | `showResource(int index)` trusts the index; negative or `>= count` reads out of bounds. |
| Q4 | `SearchMatching.cpp` | `containsText(string, string)` copies both strings on every call. |
| Q5 | `HelpRequests.cpp` | Status is a raw `string` written by hand — a typo (`"Accepted "`) silently creates a new state. Should be an `enum class`. |
| Q6 | `HelpRequests.cpp` | No state machine: Completed→Pending is allowed. |
| Q7 | `UserManagement.cpp` | Case-sensitive username comparison — "Aditi" and "aditi" are two accounts. |
| Q8 | `UserManagement.cpp` | No validation on name/username/password length or character set. |
| Q9 | all | `findXById` is `O(n)` and called inside loops → request listing is `O(n·m)`. |
| Q10 | all | No `CMakeLists.txt` / `Makefile`, no README, no build instructions. |
| Q11 | `ResourceManagement.cpp` | Owner name resolution at display time means a deleted user leaves a resource printing nothing ("Shared by: "). |
| Q12 | all | Zero comments explaining the DSA choices — weak for evaluation. |

---

## 3. What is being kept (approach C, explicitly)

| Kept | Where it goes |
|------|---------------|
| `-1` "not logged in" sentinel convention | `SessionManager` uses `userId = 0` = anonymous; the sentinel idea survives as `Session::userId` validity check |
| Duplicate-username guard, empty-input guard | `UserService::registerUser()` — extended to email + normalised username |
| "Reject requesting your own resource" | `RequestService::createRequest()` — kept verbatim in spirit |
| **Ownership re-verification at mutation time** (`manageRequests`) | `ResourceService::update/remove`, `RequestService::updateStatus/accept/reject` — the exact same guard, generalised |
| Manual case-insensitive `containsText` | `SearchService::containsCaseInsensitive()` — same algorithm, `const string&`, documented `O(n·m)` |
| `showResource()` presentation idea | `Resource::describe()` + JSON serialiser |
| Comment style, student-friendly naming | Kept throughout |
| Menu numbering / wording as a mental model | Now drives the frontend nav order |

## 4. What is deliberately dropped, and why

| Dropped | Reason |
|---------|--------|
| `cout`/`cin` inside business functions | Cannot coexist with an HTTP API. Logic moves to services returning DTOs / throwing typed errors; presentation moves to the frontend. **A thin console demo (`ConsoleDemo.cpp`) is kept optional** so you can still show the old look in a viva — but it calls the same services. |
| One `CommunityPlatform` god class | Replaced by one service per module + a routing layer. |
| Fixed `MAX_*` arrays as the primary store | Replaced by `std::vector` (dynamic) + `unordered_map` indexes. Arrays remain meaningful inside the custom heap, which is an honest array-backed structure. |
| `string status` | `enum class RequestStatus` with `toString`/`parse`. |
| Plaintext password | `PasswordHasher` (PBKDF2-HMAC-SHA256, unique salt). |

---

## 5. Requirement conflicts — flagged, with recommended solutions

You asked me to surface technical problems rather than silently work around them. Five matter.

### C1 — `file://` frontend + `fetch()` to `localhost` will fail
If you open `frontend/index.html` by double-clicking it, the page origin is `null` and every `fetch("http://localhost:8080/api/...")` is a cross-origin request. Browsers send `Origin: null`, and you then need CORS headers on every route, correct preflight handling, and cookie behaviour that is inconsistent across browsers. Students usually lose a day here.

**Recommendation (adopted):** the C++ server serves the frontend itself on `http://localhost:8080/`. Same origin → no CORS, no preflight, cookies just work, and stopping the server stops the whole app. `frontend/` stays a separate top-level folder (requirement §12 satisfied) — `Server.cpp` simply mounts it as the static root. A `--allow-cors` flag is added for anyone who insists on opening files directly.

### C2 — "prefer standard libraries / avoid dependencies" vs "use an HTTP library and JSON"
C++17 has **no socket API and no HTTP server**, and no JSON type. Something must be added.

**Recommendation (adopted):** exactly **two header-only** libraries, vendored into `backend/third_party/`:
- `httplib.h` — [github.com/yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib) (MIT). You named it in the brief. One header, no `.cpp` to add to the link line.
- `json.hpp` — [github.com/nlohmann/json](https://github.com/nlohmann/json) (MIT). One header, used **only** for HTTP bodies. Permanent data stays in `.txt` (requirement §7/§10 both hold).

Neither needs Homebrew, CMake `find_package`, or a build-system change: `Third-party install on macOS` = copy the two files in. A Makefile target `make deps` can `curl` them.

### C3 — "do not invent your own crypto" on a MacBook
Hand-rolling SHA-256 would violate the brief. OpenSSL via Homebrew works but adds an install step and a link flag.

**Recommendation (adopted):** **Apple CommonCrypto**, which ships inside macOS — `#include <CommonCrypto/CommonCrypto.h>`, `CCKeyDerivationPBKDF`, `CCRandomGenerateBytes` for the salt. **Zero installation, zero link flags**, PBKDF2-HMAC-SHA256 is RFC 2898 / NIST-approved, and "we used the platform's certified crypto instead of inventing one" is a strong viva answer. Stored format: `pbkdf2_sha256$120000$<base64 salt>$<base64 hash>`.

*Trade-off, stated honestly:* this makes the backend macOS-specific. `PasswordHasher.h` is isolated behind one interface, so switching to OpenSSL is a 20-line change (Stage 8 documents the alternative). If you need Linux CI, say so and I will ship the OpenSSL backend as the default instead.

### C4 — "Local .txt storage" vs this delivery environment
Your real deliverable is a program that runs on **your MacBook**, compiled with `clang++`. This workspace has no shell and no C++ toolchain, so I cannot compile or execute the backend here, and I will not claim I have.

**Recommendation (adopted), in two parts:**
1. **The real application** is written into this repository at the exact paths in §6. You download the repo as a folder/zip, run `make`, and it runs. Every stage includes a `make check` target plus an integration review; Stage 7 gives you a `test_api.sh` that exercises all endpoints so verification is one command on your machine, not a claim from mine.
2. **A static browser demo** of the Community Connect UI is also built (Stage 6) so the frontend can be *visually verified* here and screenshotted for your report/presentation before the backend is wired. The demo is clearly labelled and is **not** the data layer — it never pretends to replace `.txt` storage.

Being explicit: I will not say "it compiles" until you paste me the `make` output.

### C5 — Request is both "about a resource" and "standalone"
§4 Module 3 lists request fields with **no** `resourceId`, but §5 describes "a user sends a request for **a resource**", and your existing `HelpRequest` is resource-bound.

**Recommendation (adopted):** `resourceId` is a field on `HelpRequest` where **`0` means "general help request, no specific resource"**. This preserves your existing flow exactly while satisfying Module 3's field list. UI offers "Request this resource" (pre-filled) and "Post a general request".

Minor points, also resolved: `message` → `description` (spec name), with `message` accepted as an alias for one release so nothing you demo breaks; duplicate email **and** duplicate username are both rejected case-insensitively; a resource with live requests cannot be hard-deleted (it is marked unavailable, or deleted only after its requests are Cancelled) — this is the dangling-reference bug Q11/§1.5.

---

## 6. Final architecture

### 6.1 Layering

```
┌──────────────────────────────────────────────────────────┐
│  FRONTEND (frontend/)  HTML + CSS + vanilla JS            │
│  api.js is the ONLY module that calls fetch()             │
└───────────────────────────┬──────────────────────────────┘
                            │ same-origin HTTP + JSON
┌───────────────────────────▼──────────────────────────────┐
│  Server.cpp      httplib::Server, static mount, threads   │
│  ApiRouter.cpp   route table → handler → JSON / status    │
│  SessionManager  token ⇄ userId, expiry                   │
└───────────────────────────┬──────────────────────────────┘
                            │ calls services, never touches files
┌───────────────────────────▼──────────────────────────────┐
│  SERVICE LAYER  (all business rules + authorisation)      │
│  UserService  ResourceService  RequestService             │
│  SearchService  NotificationService                       │
└───────────────────────────┬──────────────────────────────┘
                            │ typed entities only
┌───────────────────────────▼──────────────────────────────┐
│  DSA LAYER                                                │
│  PriorityQueue<T> (custom array binary heap)              │
│  unordered_map indexes, std::sort relevance ranking       │
└───────────────────────────┬──────────────────────────────┘
                            │
┌───────────────────────────▼──────────────────────────────┐
│  PERSISTENCE  FileManager (atomic rewrite, escaping)      │
│  data/users.txt resources.txt requests.txt notifications.txt│
└──────────────────────────────────────────────────────────┘
```

**Rules that hold every file in every stage:**
1. No `cout` outside `ConsoleDemo.cpp`. No `fstream` outside `FileManager`. No `fetch()` outside `api.js`. No business rule in JavaScript.
2. Only the router knows HTTP status codes; services throw `ValidationError` / `AuthError` / `ForbiddenError` / `NotFoundError` / `ConflictError`, and the router maps type → status. That is the inheritance/polymorphism story, and it is genuinely useful rather than decorative.
3. Every mutation writes through `FileManager` immediately (atomic temp-file + `rename`), so a crash mid-write leaves the old file intact.
4. Authorisation is checked inside the service, against the session's `userId` — never by trusting a `userId` in the request body, and never by hiding a field in the UI.

### 6.2 Folder structure (final)

```
Community-Connect/
├── backend/
│   ├── include/
│   │   ├── Errors.h                 # exception hierarchy (inheritance/polymorphism)
│   │   ├── Entity.h                 # abstract base: id, createdAt, toRecord/fromRecord
│   │   ├── Models.h                 # User, Resource, HelpRequest, Notification + enums
│   │   ├── FileManager.h            # escape/unescape, atomic load/save, , auto-create
│   │   ├── PasswordHasher.h         # CommonCrypto PBKDF2-HMAC-SHA256
│   │   ├── PriorityQueue.h          # template custom binary heap
│   │   ├── SessionManager.h
│   │   ├── UserService.h
│   │   ├── ResourceService.h
│   │   ├── RequestService.h
│   │   ├── SearchService.h
│   │   ├── NotificationService.h
│   │   ├── ApiRouter.h
│   │   └── Server.h
│   ├── src/
│   │   ├── main.cpp                 # arg parsing, boot, graceful shutdown
│   │   ├── FileManager.cpp
│   │   ├── PasswordHasher.cpp
│   │   ├── SessionManager.cpp
│   │   ├── UserService.cpp
│   │   ├── ResourceService.cpp
│   │   ├── RequestService.cpp
│   │   ├── SearchService.cpp
│   │   ├── NotificationService.cpp
│   │   ├── ApiRouter.cpp
│   │   ├── Server.cpp
│   │   └── ConsoleDemo.cpp          # optional: old menu, new services
│   ├── third_party/
│   │   ├── httplib.h
│   │   └── json.hpp
│   └── Makefile                     # + CMakeLists.txt for VS Code users
├── frontend/
│   ├── index.html  login.html  register.html  dashboard.html
│   ├── profile.html  resources.html  resource-detail.html  add-resource.html
│   ├── requests.html  pending-requests.html  create-request.html
│   ├── search.html  notifications.html  exchanges.html  about.html
│   ├── css/  style.css  components.css
│   └── js/  api.js  auth.js  dashboard.js  resources.js  requests.js
│            search.js  notifications.js  profile.js  ui.js
├── data/
│   ├── .gitkeep                      # real files auto-created on first run
│   └── sample/sample_data.txt        # idempotent seed source
├── docs/
│   ├── STAGE1-AUDIT.md               # this file
│   ├── DSA-OOPS-REPORT.md            # Stages 2–4 fill in
│   └── API.md                        # Stage 5
├── audit/uploaded/                   # your original six files, untouched
├── tests/test_api.sh                 # Stage 7
├── README.md
└── .gitignore                        # data/*.txt except sample, build/, .DS_Store
```

Differences from your §12 sketch, all deliberate: `NotificationManager` → `NotificationService` (naming consistency); added `PriorityQueue.h`, `Errors.h`, `Entity.h`, `PasswordHasher.h`, `SessionManager.h`, `ApiRouter.h`, `third_party/`; `main.cpp` holds only boot, not the menu. Nothing from §12 is missing.

### 6.3 Data models

```cpp
enum class RequestStatus { Pending, Accepted, Rejected, Completed, Cancelled };
enum class Priority      { High = 3, Medium = 2, Low = 1 };
enum class Availability  { Available, Reserved, Unavailable };

struct User : Entity {           // id, createdAt
    std::string name, username, email;   // email + username unique (case-insensitive)
    std::string passwordHash;            // "pbkdf2_sha256$iter$salt$hash" - never serialised to JSON
    std::string city, locality, skills, interests;   // skills: comma-separated, normalised
    std::string contactPhone;            // private: only after accepted request
};

struct Resource : Entity {
    int ownerId; std::string title, description, category;
    Availability availability; std::string dateAdded;   // dateAdded = createdAt
};

struct HelpRequest : Entity {    // Entity::id == Request ID
    int requesterId, resourceId;      // resourceId == 0 → general request
    std::string title, description, requiredSkills, category;
    Priority priority; RequestStatus status;
    int acceptOwnerId;                // who accepted (resource owner or matcher)
    std::string updatedAt;
};

struct Notification : Entity {
    int receiverId; std::string message, type;  // type: request/accept/reject/priority/completed
    bool isRead; std::string timestamp;
};
```

Nine categories exactly as specified (Books and Textbooks … Other Educational Resources); `isValidCategory()` gates the write, so the API and the frontend's `<select>` cannot drift.

### 6.4 On-disk format
One record per line, `|`-delimited, with `\` escaping (`\\`, `\|`, `\n`, `\r`) so descriptions may contain pipes and newlines without breaking parsing. Line 1 is a version header (`#CC1`). Unknown/short lines are skipped with a warning rather than aborting the load (requirement §7 items 7–9). Every save writes `name.txt.tmp` then `rename()`s over the original.

### 6.5 API contract (Stage 5 implements exactly this)

| Method | Path | Auth | Purpose |
|--------|------|------|---------|
| POST | `/api/auth/register` | – | name, username, email, password, city, locality, skills, interests |
| POST | `/api/auth/login` | – | username-or-email + password → token |
| POST | `/api/auth/logout` | ✔ | invalidate token |
| GET | `/api/profile` | ✔ | own profile (incl. email/phone) |
| PUT | `/api/profile` | ✔ | edit name, city, locality, skills, interests, phone, password |
| GET | `/api/users/:id` | ✔ | public profile — **no email/phone** |
| GET | `/api/resources` | – | list; `?category=&available=1&q=` |
| POST | `/api/resources` | ✔ | create (category validated) |
| GET | `/api/resources/:id` | – | detail |
| PUT | `/api/resources/:id` | ✔ owner | edit |
| DELETE | `/api/resources/:id` | ✔ owner | delete / mark unavailable |
| PATCH | `/api/resources/:id/availability` | ✔ owner | Available/Reserved/Unavailable |
| POST | `/api/requests` | ✔ | create with priority |
| GET | `/api/requests/mine` | ✔ | my requests |
| GET | `/api/requests/incoming` | ✔ | requests against my resources |
| GET | `/api/requests/pending` | ✔ | **priority-queue order**, read-only peek |
| GET | `/api/requests/next` | ✔ | extract highest priority (does not mutate status) |
| PUT | `/api/requests/:id` | ✔ requester, Pending only | edit |
| POST | `/api/requests/:id/accept` | ✔ resource owner | notify requester |
| POST | `/api/requests/:id/reject` | ✔ resource owner | notify requester |
| POST | `/api/requests/:id/cancel` | ✔ requester | notify owner |
| POST | `/api/requests/:id/complete` | ✔ either party | notify both |
| GET | `/api/requests/:id/contact` | ✔ both parties, **Accepted only** | owner's email + phone, else `403` |
| GET | `/api/search/resources` | – | `q`, `category` — ranked |
| GET | `/api/search/users` | ✔ | by skill |
| GET | `/api/search/matches?requestId=` | ✔ | users whose skills cover the request, ranked |
| GET | `/api/notifications` | ✔ | mine, newest first, `?unread=1` |
| GET | `/api/notifications/unread-count` | ✔ | badge |
| POST | `/api/notifications/:id/read` | ✔ receiver | mark read |
| POST | `/api/notifications/read-all` | ✔ | mark all |
| GET | `/api/dashboard` | ✔ | all counters in one call |

Errors are always `{"error":"message","code":"SNAKE_CASE"}`; `401` unauthenticated, `403` forbidden (locked contact details, not your request), `404`, `409` duplicate, `422` validation.

### 6.6 DSA place → why → complexity (commitment for later stages)

| Structure | Where | Why | Complexity |
|---|---|---|---|
| `std::vector` | every entity store | grow past 100/200/300; cache-friendly, `O(1)` amortised append | append O(1)*, search O(n) |
| `unordered_map<int,int>` | id → vector index | kills the `O(n)` `findXById` in every scan loop | lookup O(1) avg |
| `unordered_map<string,int>` | email/username → index | duplicate check at registration | O(1) avg |
| `unordered_map<string, set<int>>` | skill → user ids (inverted index) | Module 4 skill matching without full scans | build O(total skill tokens), query O(k + matches) |
| `PriorityQueue<T>` **custom array binary heap** | pending help requests | compulsory requirement | insert O(log n), extract O(log n), peek O(1), build-heap O(n) |
| `std::sort` + comparator | relevance ranking, request lists | descending match score | O(n log n) |
| `std::set` | category & skill sets | dedupe, `O(log n)` insert | O(log n) |
| naive `containsText` | keyword search (preserved) | explainable; improvements discussed | O(n·m) |
| `std::deque` | unread notification FIFO feed | oldest-first display, cheap both ends | push/pop O(1) |
| `std::stack` (optional) | undo last delete | only if you want a "Restore" feature — **not** added without real justification | O(1) |

Tie-break inside the heap: compare `priority` first, then **smaller `id` (older) wins**, giving spec-correct "High > Medium > Low, older first". Heap is rebuilt from `requests.txt` at boot; accept/reject/cancel/complete mark the request non-pending and it is filtered out lazily on pop (`O(log n)`), with a documented note on why a decrease-key variant is unnecessary here.

### 6.7 OOPS map (commitment)

| Concept | Concrete instance |
|---|---|
| Encapsulation | services keep their vectors/maps private; every mutation goes through a method |
| Abstraction | `FileManager` hides all `fstream`, temp files and escaping; services never see a stream |
| Inheritance | `Entity` → `User`/`Resource`/`HelpRequest`/`Notification`; `AppError` → `ValidationError`/`AuthError`/`ForbiddenError`/`NotFoundError`/`ConflictError` |
| Polymorphism | `virtual toRecord()/fromRecord()/describe()`; `what()` + a `code()` used by the router to pick the HTTP status |
| Composition | `CommunityPlatform`/router **has-a** each service; `RequestService` **has-a** `PriorityQueue`, a `FileManager`, a `NotificationService` |
| Constructors/destructors | services take injected `dataPath`; destructors flush pending writes |
| Overloading | `SearchService::search(q)`, `search(q, category)`; `PriorityQueue::push(T)`, `push(T, p)` if needed |
| Access specifiers | `contactPhone`/`passwordHash` private-by-convention + never serialised publicly |

---

## 7. Stage plan from here (unchanged from your §15, with my checkpoints)

| Stage | Deliverable | How I verify |
|---|---|---|
| **2** | `Errors.h`, `Entity.h`, `Models.h`, `FileManager`, `PasswordHasher`, `SessionManager`, `UserService`, `Makefile`, minimal `main.cpp` | `make` compiles here-best-effort; you run `make && ./build/cc-server`; round-trip persistence check on `users.txt` |
| **3** | `ResourceService`, `PriorityQueue.h`, `RequestService` + state machine + contact gate | heap unit demo; accept/reject/complete flow |
| **4** | `SearchService`, `NotificationService` | ranking output, notification fan-out |
| **5** | `ApiRouter`, `Server`, `docs/API.md` | `curl` transcript of every endpoint |
| **6** | All frontend pages + static demo for visual check | `PlaywrightScreenshot` on every page, desktop + mobile |
| **7** | `tests/test_api.sh`, integration review, bug fixes | full run on your machine |
| **8** | Build/run/restart guide, DSA+OOPS report, time complexities, limitations | you follow it on macOS |

**Stage 1 is complete.** Nothing has been generated beyond this document and the untouched originals in `audit/uploaded/`. Confirm (or correct C5's `resourceId=0` convention and C3's CommonCrypto choice) and I will start **Stage 2 — Backend Foundation**.
