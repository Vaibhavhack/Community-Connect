# STAGE 4 - Search, Matching and Dashboard

Status: **implemented and self-tested**.

## Scope

Stage 4 adds the search/matching layer without moving business logic into JavaScript.

Files added:

- `backend/include/SearchService.h`
- `backend/src/SearchService.cpp`

Files updated:

- `backend/include/UserService.h`
- `backend/src/UserService.cpp`
- `backend/include/ConsoleDemo.h`
- `backend/src/ConsoleDemo.cpp`
- `backend/src/RequestService.cpp`
- `backend/src/SelfTest.cpp`
- `backend/src/main.cpp`

## 1. Resource search and ranking

`SearchService::searchResources()` searches the title, category and description.

The score is deliberately simple enough to explain in a viva:

- exact title match: +30
- title contains the complete query: +20
- exact category match: +15
- category contains the query: +8
- description contains the complete query: +4
- each matching query term in title: +10
- each matching query term in category: +6
- each matching query term in description: +2

Results are sorted by score descending, then by newer resource id.

The method also supports category and availability filters and a result limit.

Complexity: for `R` resources and `Q` query terms, the basic scan is approximately
`O(R * Q * L)` where `L` is the amount of text inspected. The final sort is
`O(M log M)` for `M` matched resources.

## 2. Skill and interest search

`SearchService::searchUsers()` reads public profiles from `UserService` and
matches requested skills/interests against the user's skill/interests lists.

Scoring:

- matching skill: +10
- matching interest: +3
- exact city match when a city filter is supplied: +2

The returned `UserView` never contains email or phone, so search cannot leak
private contact information.

## 3. Help-request matching

`matchUsersForRequest(requestId)` reads the request's `requiredSkills`, then
scores every other public user.

A matching skill contributes +10. Results are returned in descending relevance,
with the user id used as a deterministic tie-break.

No machine learning is used; the algorithm is transparent and easy to explain.

## 4. Dashboard aggregation

`dashboardFor(userId)` combines existing service operations into one dashboard
result:

- resources offered
- active help requests
- pending requests
- accepted requests
- completed exchanges
- incoming requests
- unread notifications
- highest-priority pending request

This keeps the frontend from making unnecessary business calculations.

## 5. Important Stage 3 fixes found during verification

The uploaded Stage 3 code compiled only after a const-correctness issue was fixed:
`UserService::getPublicProfile()` is a read-only operation and is now `const`.

The priority queue self-test also exposed a real duplicate-insertion bug in
`RequestService::createRequest()`. The old sequence rebuilt the queue *after*
adding the new request and then pushed that same request again. Stage 4 fixes the
order: refresh the existing heap first, append the new request, then perform one
heap insertion.

The corrected backend self-test now reports:

`Passed: 136   Failed: 0`

## 6. Console demonstration

The console menu now includes:

`Search / Matching / Dashboard`

This uses exactly the same `SearchService` that the future HTTP API will use.

## 7. Next stage

Stage 5 should now add the local HTTP layer:

- JSON request/response handling
- authentication/session middleware
- API routes
- authorization checks at the API boundary
- contact-details endpoint using the existing C++ privacy gate

No frontend work is required yet; the frontend belongs to Stage 6.
