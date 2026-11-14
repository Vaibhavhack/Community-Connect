# STAGE 3 — Resource, Help Request, Priority Queue and Notifications

**Scope delivered:** Module 2 (resources), Module 3 (help requests + the compulsory priority queue), the contact-privacy gate, and Module 5 (notifications).

Status: **code complete, not yet compiled.** No `make` output has reached me, so I still make no claim that this builds. Run the commands in section 8 and send me whatever comes out.

---

## 1. One scope change, stated plainly

`NotificationService` moved from Stage 4 into Stage 3.

Reason: `acceptRequest`, `rejectRequest` and `completeRequest` are only *correct* if the other person is told. Section 5 of your brief requires the owner to be notified when someone requests a resource, and the requester to be notified on accept/reject. Shipping those three methods without notifications would have meant shipping them twice.

Stage 4 therefore becomes: skill matching, ranked search, dashboard aggregation.

## 2. Files delivered

| File | Type | Purpose |
|---|---|---|
| `backend/include/PriorityQueue.h` | header | **custom array-backed binary max-heap** — push O(log n), pop O(log n), peek O(1), buildHeap O(n), sortedCopy O(n log n) |
| `backend/include/ResourceService.h` + `src/ResourceService.cpp` | class | **Module 2** — create, read, filter, edit, delete, availability |
| `backend/include/NotificationService.h` + `src/NotificationService.cpp` | class | **Module 5** — 6 event types, unread count, mark read/all |
| `backend/include/RequestService.h` + `src/RequestService.cpp` | class | **Module 3** — priority queue integration, 5-state machine, contact gate |
| `backend/include/ConsoleDemo.h` + `src/ConsoleDemo.cpp` | rewritten | original menu, now driving all four services |
| `backend/src/SelfTest.cpp` | extended | sections 8–13 added (heap, resources, requests, privacy, notifications, integrity) |
| `backend/src/main.cpp`, `backend/CMakeLists.txt` | updated | five services wired up |

## 3. The priority queue, in detail

### Why it is a real priority queue and not a sorted vector

A sorted vector answers "show everything in order" **once**. A priority queue answers "what is most urgent right now" **repeatedly, while items keep arriving**. Inserting into a sorted vector costs O(n) because every later element shifts; inserting into a heap costs O(log n). That difference is the entire justification, and it is the first thing an examiner should ask about.

### How the heap works

The heap is a `std::vector<HelpRequest>` interpreted as a complete binary tree, so there are no pointers and no per-node allocation:

```
parent(i)    = (i - 1) / 2
leftChild(i) = 2i + 1
rightChild(i) = 2i + 2
```

- **push** — append at the end, then `siftUp`: swap with the parent while the child outranks it. At most the height of the tree → **O(log n)**.
- **pop** — take the root (the answer), move the last leaf to the root, shrink, then `siftDown`: keep swapping with the stronger child → **O(log n)**.
- **top** — the root *is* the best element by the heap property → **O(1)**.
- **buildHeap** — start at the last internal node and `siftDown` each one towards the root. Leaves are already valid heaps, which is why this is **O(n)**, not O(n log n) from n separate inserts. This is what rebuilds the queue from `requests.txt` at startup.
- **sortedCopy** — repeatedly extract the root from a *copy*. That is literally heap sort → **O(n log n)**.

### The comparison rule

```cpp
bool RequestService::higherPriorityFirst(const HelpRequest& a, const HelpRequest& b) {
    if (a.priority != b.priority)
        return static_cast<int>(a.priority) > static_cast<int>(b.priority); // High > Medium > Low
    return a.getId() < b.getId();   // tie: smaller id = created earlier = older
}
```

`Priority` is `enum class Priority { Low = 1, Medium = 2, High = 3 }`, so the numeric values *are* the ordering — the comparison needs no lookup table.

The tie-break uses **id, not a timestamp string**, because ids are handed out in creation order and can never decrease. That is provably older-first, and it avoids parsing dates on every comparison. (The ISO-8601 `createdAt` would also work, since that format sorts lexicographically — worth mentioning as an alternative.)

### Where the queue actually influences the application

The brief insists the queue must not be decorative. It is used in four real places:

1. `listPendingByPriority()` — the "Pending Requests" screen. Full heap-sort order, read-only.
2. `peekHighestPriority()` — the dashboard's "most urgent request" card. O(1), no mutation.
3. `takeHighestPriority()` — "Process next request" in the console demo. A genuine `pop()`.
4. `listIncoming()` — requests against your resources, ordered by the same comparator, so owners see urgent work first.

**Queue consistency strategy.** Rather than implement decrease-key (which needs an id→heap-position map and is fiddly to get right), any mutation marks the queue dirty and the next read rebuilds it with an O(n) bottom-up heapify. Accepting one request therefore costs O(n) *once, lazily*, instead of O(log n) plus a fragile invariant. For a few hundred requests on a laptop this is the right engineering trade-off, and I would rather defend that sentence in a viva than defend a half-correct decrease-key. It is written in the code comments too.

## 4. The status state machine

Your original `manageRequests()` let a user set any status to any status — including dragging **Completed back to Pending**. That is now impossible.

```
                  ┌──────── accept (owner) ────────► Accepted ──── complete (either) ──► Completed
Pending ──────────┤                                    │
                  ├──────── reject (owner) ────────► Rejected
                  └──────── cancel (requester) ────► Cancelled
```

| Transition | Who may do it | Guard |
|---|---|---|
| → Accepted | resource owner only; for a general request, any other registered user | must be `Pending` |
| → Rejected | same rule as Accept | must be `Pending` |
| → Cancelled | requester only | must be `Pending` |
| → Completed | requester, accepter, or resource owner | must be `Accepted` |
| → Pending | **nobody** | always refused |

`authorizeTransition()` is the single place these rules live, so the API, the console and any test all get identical behaviour. Self-acceptance is blocked structurally: `mayAccept()` returns false when `actingUserId == requesterId`.

## 5. Contact privacy — enforced in C++, not in CSS

`RequestService::contactDetailsFor(requestId, actingUserId)` is the **only** method in the backend that returns another user's email or phone number. It refuses unless all of the following hold:

1. The request status is `Accepted` or `Completed` (a `Rejected` request stays locked).
2. The caller is a party to that request — the requester, the accepter, or the resource owner.
3. The caller is identified by a **session token**, never by a `userId` in the request body.

Point 3 is what makes it real. A visitor editing the HTTP request by hand can change `requestId`, but they cannot change *who the session says they are*. The self-test verifies all four negative cases: pending, rejected, and a third unrelated user.

`UserView` (from Stage 2) has no `passwordHash` member at all, and public profile views leave `email`/`contactPhone` empty. So there are three independent layers: the view type, the service gate, and the session identity.

## 6. Referential integrity on delete

Your original code could delete a resource while requests still pointed at it, leaving `requests[i].resourceId` dangling. `ResourceService::deleteResource()` now takes a `hasLiveRequests` predicate from the request module and refuses while any request is `Pending` or `Accepted`, with a message telling the user what to do instead (mark unavailable, or cancel/complete those requests).

This is dependency injection rather than a direct `ResourceService → RequestService` dependency, which keeps the two modules from needing each other. Both directions would be a circular include.

## 7. DSA / OOPS in this stage

| Concept | Where | Complexity |
|---|---|---|
| **Binary heap (custom)** | `PriorityQueue<T>` | push/pop O(log n), peek O(1), buildHeap O(n) |
| **Heap sort** | `PriorityQueue::sortedCopy` | O(n log n) |
| `std::function` comparator | `PriorityQueue<T>` constructor | lets one structure serve any ordering policy |
| `std::vector` backing store | the heap's array | O(1) index access, no pointer chasing |
| `unordered_map<int, size_t>` | every service's id index | O(1) lookup instead of O(n) scan |
| `std::sort` + comparator | `listMine`, `listIncoming`, `listAll` | O(n log n) |
| `enum class` state machine | `RequestStatus`, `authorizeTransition` | O(1) transition check |
| Template class | `PriorityQueue<T>` | works for `HelpRequest` and anything else later |
| Composition | `RequestService` has-a `ResourceService`, `UserService`, `NotificationService`, `PriorityQueue` | — |
| Functional composition | `hasLiveRequests` callback passed into `ResourceService` | avoids a circular dependency |

## 8. How to verify

```bash
cd backend
make
./build/cc-server --selftest
```

Sections **8–13** are new and cover:

- **8. Priority queue** — insertion out of order; High beats Medium beats Low; equal priority → older first; `sortedCopy` leaves the queue unchanged; extracting everything yields exactly `12, 13, 11, 10`; popping an empty queue throws.
- **9. Resources** — create; a food category is rejected; a short title is rejected; a non-owner cannot edit; partial edit leaves untouched fields alone; category and availability filters.
- **10. Requests** — unauthenticated creation refused; requesting your own resource refused (your preserved rule); priority order in the queue; peek doesn't mutate; takeover doesn't change status; owner-only accept/reject; no double-accept; accepted requests can't be cancelled or edited; outsider can't complete; no double-complete.
- **11. Contact privacy** — locked while Pending; still locked after Rejected; both parties see each other after Accepted; a third user still refused; public profiles still hide the fields.
- **12. Notifications** — owner notified per request; requester notified on accept *and* reject; can't mark someone else's read; unread count decrements; mark-all works; survives a restart with read state intact.
- **13. Referential integrity** — resource with open requests can't be deleted; non-owner can't delete; an unreferenced resource deletes fine.

Also try the console, which exercises the same services:

```bash
./build/cc-server --console
```

Register two users in two runs, add a resource as one, request it as the other with High priority, then log in as the owner and accept. Item **9. Pending requests by priority** prints the heap order and lets you extract the top item.

## 9. Bugs I found in my own Stage 3 code before handing over

Recording these because they are the kind of thing that would have cost you an evening:

1. **Infinite recursion** — an edit of mine left a duplicated `ensureQueueFresh()` whose body called itself, and `listPendingByPriority()` had been deleted. Repaired; the file is now 676 lines and verified by reading it back.
2. **Duplicate definition** — `markQueueDirty()` was defined inline in the header *and* in the `.cpp`, which would have failed at link time. Header now only declares it.
3. **Two inverted test assertions** — the self-test used `checkThrows` for `queue.top()` and for `createRequest` with a valid user. Both should succeed, not throw, so the tests would have failed even with correct code. Fixed: peek now asserts the size is unchanged, and the auth test passes `0`.
4. **`NotificationService` index built twice** — a leftover loop wrote every position as `0` before a second loop fixed it. Simplified to one pass.

## 10. Deferred to later stages

Stage 4: `SearchService` (skill matching, ranked results), dashboard aggregation. Stage 5: `ApiRouter`, `Server`, JSON. Stage 6: frontend. Stage 7: `tests/test_api.sh`. Stage 8: sample data seeding, full DSA/OOPS report.

**Still outstanding from you:** the `make` output. If it fails, send the first error block and I will fix it in one pass — Stage 4 adds files, so catching a Stage 2/3 compile error first keeps the failure surface small.
