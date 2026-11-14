# Stage 8 - DSA and OOPS Report

## 1. Data Structures

### Vector
Used for the in-memory collections of users, resources, requests and notifications.

Typical use:

```cpp
std::vector<User> users_;
std::vector<Resource> resources_;
std::vector<HelpRequest> requests_;
```

Advantages:
- Simple to understand.
- Fast iteration.
- Good cache locality.
- Suitable because the complete local dataset is loaded into memory.

Typical lookup by position is O(1); searching an unsorted vector is O(n).

### Hash Map
`std::unordered_map` is used for indexes such as user ID, email and username.

Average complexity:
- Insert: O(1)
- Search: O(1)
- Delete: O(1)

This avoids repeatedly scanning the complete user list for duplicate email or username checks.

### Set-like matching
Skill matching normalizes skills case-insensitively and compares the required skill set with user skill sets. This prevents `C++` and `c++` from being treated as unrelated skills.

### Queue / notification collection
Notifications are stored in a persistent vector and exposed newest-first. The data structure is appropriate because notifications are append-oriented and are filtered by receiver and read state.

### Binary Heap Priority Queue
This is the compulsory DSA feature.

The project implements an array-backed binary max-heap in `PriorityQueue.h` rather than calling `std::priority_queue`.

Ordering:

```text
High > Medium > Low
```

For equal priority, the older request is processed first. Request IDs increase with creation order, so the smaller ID is the older request.

Complexities:

| Operation | Complexity |
|---|---:|
| `push` | O(log n) |
| `pop` | O(log n) |
| `top` | O(1) |
| `buildHeap` | O(n) |
| sorted display | O(n log n) |

The queue is rebuilt from pending requests after startup and whenever a request mutation makes the heap stale.

### Searching
Case-insensitive text searching is used for resource titles/descriptions and user skills.

A simple substring search over a string of length `n` with a pattern of length `m` is O(n*m) in the worst case.

### Sorting / relevance ranking
Search results can be ranked by the number of matching terms/skills and then ordered so more relevant results appear first.

Sorting `n` results costs O(n log n).

---

# 2. OOPS Concepts

## Encapsulation
Each service owns its data and exposes controlled public operations.

Examples:
- `UserService`
- `ResourceService`
- `RequestService`
- `NotificationService`

The internal vectors and indexes are private.

## Abstraction
The frontend does not need to know how users are stored in `.txt` files or how a heap is maintained.

For example:

```cpp
requests.acceptRequest(requestId, actingUserId);
```

hides the validation, state transition, persistence and notification work.

## Inheritance
`User`, `Resource`, `HelpRequest` and `Notification` are record types built around the common `Entity` base in the project model.

Inheritance is used only where there is a meaningful common identity/timestamp concept.

## Polymorphism
The design avoids artificial inheritance hierarchies. Where behavior does not require runtime polymorphism, normal composition and service classes are preferable for this project.

This is intentional: OOPS should be demonstrated meaningfully rather than added only for marks.

## Constructors
Services receive their dependencies through constructors:

```cpp
UserService(FileManager& files, SessionManager& sessions);
```

This is composition and makes dependencies explicit.

## Access Specifiers
`public` methods form the service API, while storage, indexes and helper methods remain `private`.

## Composition
`RequestService` uses:
- `FileManager`
- `ResourceService`
- `UserService`
- `NotificationService`

This is one of the strongest OOPS examples in the project because the request module coordinates several independent responsibilities without owning their data.
