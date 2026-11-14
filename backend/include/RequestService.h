// RequestService.h
// -----------------------------------------------------------------------------
// Purpose : MODULE 3 - Help Request Management, the contact-privacy gate, and
//           the place where the priority queue actually drives behaviour.
//
// Three things make this module different from a plain CRUD list:
//
// 1. PRIORITY QUEUE (the compulsory DSA requirement)
//    Every pending request lives in a hand-written binary heap (PriorityQueue.h).
//    High > Medium > Low, and among equal priorities the OLDER request is
//    served first (smaller id = created earlier, and ids never decrease).
//    The heap is rebuilt from requests.txt at startup, so priority order
//    survives a restart.
//
// 2. STATUS STATE MACHINE
//    Pending ──accept──> Accepted ──complete──> Completed
//        │                    │
//        ├──reject──> Rejected│
//        └──cancel──> Cancelled
//    Anything else is refused, so a request can never jump from Completed back
//    to Pending (which your original manageRequests() allowed).
//
// 3. CONTACT PRIVACY, ENFORCED HERE
//    Not in CSS, not in JavaScript. contactDetailsFor() is the only way to read
//    an email/phone number, and it throws ForbiddenError unless the caller is a
//    party to an ACCEPTED (or Completed) request. Editing the API request by
//    hand cannot get around it, because the check compares the record's ids
//    against the authenticated session user.
// -----------------------------------------------------------------------------

#ifndef CC_REQUEST_SERVICE_H
#define CC_REQUEST_SERVICE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "FileManager.h"
#include "Models.h"
#include "NotificationService.h"
#include "PriorityQueue.h"
#include "ResourceService.h"
#include "UserService.h"

namespace cc {

// What the contact endpoint returns. Only ever produced after the privacy
// check has passed.
struct ContactDetails {
    int userId = 0;
    std::string name;
    std::string email;
    std::string phone;
    std::string city;
    std::string locality;
};

class RequestService {
public:
    static constexpr const char* kFileName = "requests.txt";

    static constexpr std::size_t kMaxTitleLength = 100;
    static constexpr std::size_t kMaxDescriptionLength = 1000;

    RequestService(FileManager& files,
                   ResourceService& resources,
                   UserService& users,
                   NotificationService& notifications);

    // ---- create / read -----------------------------------------------------

    // resourceId = 0 means a general help request.
    // Category is optional for a resource-bound request (it inherits the
    // resource's category); for a general request it must be one of the nine.
    int createRequest(int requesterId,
                      int resourceId,
                      const std::string& title,
                      const std::string& description,
                      const std::string& requiredSkills,
                      const std::string& category,
                      Priority priority);

    HelpRequest requireRequest(int requestId) const;
    bool exists(int requestId) const;

    // Requests I created, newest first.
    std::vector<HelpRequest> listMine(int userId) const;

    // Requests raised against resources I own (only these can I accept/reject).
    std::vector<HelpRequest> listIncoming(int userId) const;

    // ---------------------------------------------------------------------
    // THE PRIORITY QUEUE, exposed three ways
    // ---------------------------------------------------------------------

    // All pending requests in priority order (High first, then oldest).
    // Reads the heap without changing it: it works on a copy, then heap-sorts.
    std::vector<HelpRequest> listPendingByPriority() const;

    // The single most urgent pending request. O(1) peek, queue unchanged.
    HelpRequest peekHighestPriority() const;

    // Extracts the most urgent pending request, i.e. a real pop(). Returns
    // false when nothing is pending. This does NOT change any status - the
    // caller decides what to do with it. Status changes go through accept().
    bool takeHighestPriority(HelpRequest& out);

    std::size_t pendingCount() const;

    // ---- lifecycle: the state machine -------------------------------------

    // Owner of the resource (or, for a general request, any other logged-in
    // user) accepts. Notifies the requester and unlocks contact details.
    HelpRequest acceptRequest(int requestId, int actingUserId);

    HelpRequest rejectRequest(int requestId, int actingUserId);

    // Requester only, while Pending.
    HelpRequest cancelRequest(int requestId, int actingUserId);

    // Either party, once Accepted.
    HelpRequest completeRequest(int requestId, int actingUserId);

    // Requester only, while Pending. Empty field = unchanged.
    HelpRequest updateRequest(int requestId,
                              int actingUserId,
                              const std::string& title,
                              const std::string& description,
                              const std::string& requiredSkills,
                              const std::string& category,
                              Priority priority);

    // ---- contact privacy ---------------------------------------------------

    // Throws ForbiddenError unless the caller is a party to an accepted or
    // completed request. Returns the OTHER person's contact details.
    ContactDetails contactDetailsFor(int requestId, int actingUserId);

    // ---- helpers for other modules -----------------------------------------

    // Used by ResourceService::deleteResource to protect referential integrity.
    bool hasLiveRequests(int resourceId) const;

    std::size_t countPendingForUser(int userId) const;
    std::size_t countAcceptedForUser(int userId) const;
    std::size_t countCompletedForUser(int userId) const;

private:
    // High > Medium > Low; equal priority -> smaller id (older) first.
    static bool higherPriorityFirst(const HelpRequest& a, const HelpRequest& b);

    static std::string validateTitle(const std::string& value);
    static std::string validateDescription(const std::string& value);

    HelpRequest& requireRequestMutable(int requestId);

    // Re-checks the acting user's authority for the specific transition.
    void authorizeTransition(const HelpRequest& request, int actingUserId,
                             RequestStatus target) const;

    // Who is allowed to accept this request?
    bool mayAccept(const HelpRequest& request, int actingUserId) const;

    // Commit helper: store, re-sync the heap, persist, return the record.
    HelpRequest commit(const HelpRequest& request, bool affectsPendingSet);

    // Queue maintenance.
    void markQueueDirty() const;     // defined in RequestService.cpp
    void ensureQueueFresh() const;   // rebuild from pending requests if needed

    std::string requestLabel(const HelpRequest& request) const;

    void persist();

    FileManager& files_;
    ResourceService& resources_;
    UserService& users_;
    NotificationService& notifications_;

    std::vector<HelpRequest> requests_;
    std::unordered_map<int, std::size_t> idIndex_;   // request id -> position
    int nextRequestId_ = 1;

    // The priority queue. mutable because peek/list are logically const but
    // may need to refresh a stale heap.
    mutable PriorityQueue<HelpRequest> pendingQueue_;
    mutable bool queueDirty_ = true;
};

} // namespace cc

#endif // CC_REQUEST_SERVICE_H
