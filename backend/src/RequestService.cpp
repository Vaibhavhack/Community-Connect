
#include "RequestService.h"

#include <algorithm>
#include <cstddef>

#include "Errors.h"

namespace cc {

namespace {

bool hasControlCharacters(const std::string& value) {
    for (char c : value) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 && c != '\n' && c != '\t') return true;
        if (u == 0x7F) return true;
    }
    return false;
}

} 
RequestService::RequestService(FileManager& files,
                               ResourceService& resources,
                               UserService& users,
                               NotificationService& notifications)
    : files_(files),
      resources_(resources),
      users_(users),
      notifications_(notifications),
      pendingQueue_(higherPriorityFirst) {

    requests_ = files_.loadAll<HelpRequest>(kFileName);

    int highestId = 0;
    for (std::size_t position = 0; position < requests_.size(); ++position) {
        const HelpRequest& request = requests_[position];
        highestId = std::max(highestId, request.getId());
        idIndex_[request.getId()] = position;
    }
    nextRequestId_ = highestId + 1;

    
    queueDirty_ = true;
}

void RequestService::persist() {
    files_.saveAll(kFileName, requests_);
}


bool RequestService::higherPriorityFirst(const HelpRequest& a, const HelpRequest& b) {
    if (a.priority != b.priority) {
        return static_cast<int>(a.priority) > static_cast<int>(b.priority);
    }
    return a.getId() < b.getId();
}

void RequestService::markQueueDirty() const {
    queueDirty_ = true;
}

void RequestService::ensureQueueFresh() const {
    if (!queueDirty_) return;

   
    std::vector<HelpRequest> pending;
    pending.reserve(requests_.size());

    for (const HelpRequest& request : requests_) {
        if (request.status == RequestStatus::Pending) {
            pending.push_back(request);
        }
    }

    
    pendingQueue_.buildHeap(pending);
    queueDirty_ = false;
}

std::vector<HelpRequest> RequestService::listPendingByPriority() const {
    ensureQueueFresh();

    
    return pendingQueue_.sortedCopy();
}

HelpRequest RequestService::peekHighestPriority() const {
    ensureQueueFresh();

    if (pendingQueue_.empty()) {
        throw NotFoundError("There are no pending help requests.");
    }
    return pendingQueue_.top(); // O(1)
}

bool RequestService::takeHighestPriority(HelpRequest& out) {
    ensureQueueFresh();

    if (pendingQueue_.empty()) {
        return false;
    }

    out = pendingQueue_.pop();   
    markQueueDirty();            
    return true;
}

std::size_t RequestService::pendingCount() const {
    ensureQueueFresh();
    return pendingQueue_.size();
}


bool RequestService::exists(int requestId) const {
    return requestId > 0 && idIndex_.find(requestId) != idIndex_.end();
}

HelpRequest RequestService::requireRequest(int requestId) const {
    auto it = idIndex_.find(requestId);
    if (it == idIndex_.end()) {
        throw NotFoundError("Help request not found.");
    }
    return requests_[it->second];
}

HelpRequest& RequestService::requireRequestMutable(int requestId) {
    auto it = idIndex_.find(requestId);
    if (it == idIndex_.end()) {
        throw NotFoundError("Help request not found.");
    }
    return requests_[it->second];
}

std::string RequestService::requestLabel(const HelpRequest& request) const {
    return "\"" + request.title + "\" (#" + std::to_string(request.getId()) + ")";
}

std::vector<HelpRequest> RequestService::listMine(int userId) const {
    std::vector<HelpRequest> result;

    for (const HelpRequest& request : requests_) {
        if (request.requesterId == userId) result.push_back(request);
    }

    std::sort(result.begin(), result.end(),
              [](const HelpRequest& a, const HelpRequest& b) {
                  return a.getId() > b.getId();
              });
    return result;
}

std::vector<HelpRequest> RequestService::listIncoming(int userId) const {
    std::vector<HelpRequest> result;

    for (const HelpRequest& request : requests_) {
        if (request.resourceId == 0) continue;             // no resource, no owner
        if (!resources_.exists(request.resourceId)) continue;

        if (resources_.ownerOf(request.resourceId) == userId &&
            request.requesterId != userId) {               // never show my own request here
            result.push_back(request);
        }
    }

    // Most urgent first - this list is about decisions, not history.
    std::sort(result.begin(), result.end(), higherPriorityFirst);
    return result;
}

bool RequestService::hasLiveRequests(int resourceId) const {
    for (const HelpRequest& request : requests_) {
        if (request.resourceId != resourceId) continue;

        if (request.status == RequestStatus::Pending ||
            request.status == RequestStatus::Accepted) {
            return true;
        }
    }
    return false;
}


int RequestService::createRequest(int requesterId,
                                  int resourceId,
                                  const std::string& title,
                                  const std::string& description,
                                  const std::string& requiredSkills,
                                  const std::string& category,
                                  Priority priority) {
    if (requesterId <= 0) {
        throw AuthError("Please log in to create a help request.");
    }

    HelpRequest request;

    
    request.title          = validateTitle(title);
    request.description    = validateDescription(description);
    request.requiredSkills = joinList(splitList(requiredSkills));
    request.priority       = priority;
    request.requesterId    = requesterId;

    int ownerToNotify = 0;
    std::string resourceTitle;

    if (resourceId > 0) {
        
        if (!resources_.exists(resourceId)) {
            throw NotFoundError("Resource not found.");
        }

        
        if (resources_.ownerOf(resourceId) == requesterId) {
            throw ForbiddenError("You cannot request your own resource.");
        }

        if (resources_.requireResource(resourceId).availability == Availability::Unavailable) {
            throw ConflictError("That resource is currently marked unavailable.");
        }

        request.resourceId = resourceId;
        resourceTitle = resources_.titleOf(resourceId);

        
        request.category = trim(category).empty()
                               ? resources_.categoryOf(resourceId)
                               : categories::validate(category);

        ownerToNotify = resources_.ownerOf(resourceId);

    } else {
       
        request.resourceId = 0;

        if (trim(category).empty()) {
            throw ValidationError("Please choose a related resource category.");
        }
        request.category = categories::validate(category);
    }

    request.status        = RequestStatus::Pending;
    request.acceptOwnerId = 0;
    request.setId(nextRequestId_++);
    request.setCreatedAt(nowIso8601());
    request.updatedAt = request.getCreatedAt();

    
    ensureQueueFresh();

    requests_.push_back(request);
    idIndex_[request.getId()] = requests_.size() - 1;

    
    pendingQueue_.push(request);

    persist();

    
    if (ownerToNotify > 0) {
        const std::string requesterName = users_.getPublicProfile(requesterId).name;
        notifications_.notifyResourceRequested(ownerToNotify, requesterId, requesterName,
                                               resourceTitle, request.getId());
    }

    return request.getId();
}
bool RequestService::mayAccept(const HelpRequest& request, int actingUserId) const {
    if (actingUserId <= 0) return false;
    if (actingUserId == request.requesterId) return false;

    if (request.resourceId > 0) {
        if (!resources_.exists(request.resourceId)) return false;
        return resources_.ownerOf(request.resourceId) == actingUserId;
    }

    return users_.exists(actingUserId);
}

void RequestService::authorizeTransition(const HelpRequest& request,
                                         int actingUserId,
                                         RequestStatus target) const {
    if (actingUserId <= 0) {
        throw AuthError("Please log in first.");
    }

    const std::string label = requestLabel(request);

    switch (target) {
        case RequestStatus::Accepted:
        case RequestStatus::Rejected:
            if (request.status != RequestStatus::Pending) {
                throw ConflictError("Only a pending request can be " +
                                    std::string(target == RequestStatus::Accepted
                                                    ? "accepted."
                                                    : "rejected.") +
                                    " This one is already " +
                                    toString(request.status) + ".");
            }
            if (!mayAccept(request, actingUserId)) {
                throw ForbiddenError(
                    "Only the owner of the related resource can " +
                    std::string(target == RequestStatus::Accepted ? "accept" : "reject") +
                    " this request.");
            }
            return;

        case RequestStatus::Cancelled:
            if (request.requesterId != actingUserId) {
                throw ForbiddenError("Only the person who created a request can cancel it.");
            }
            if (request.status != RequestStatus::Pending) {
                throw ConflictError("Only a pending request can be cancelled. This one is "
                                    "already " + toString(request.status) + ".");
            }
            return;

        case RequestStatus::Completed: {
            if (request.status != RequestStatus::Accepted) {
                throw ConflictError("Only an accepted request can be marked completed. This "
                                    "one is " + toString(request.status) + ".");
            }

            const bool isRequester = request.requesterId == actingUserId;
            const bool isAccepter  = request.acceptOwnerId == actingUserId;
            bool isResourceOwner   = false;
            if (request.resourceId > 0 && resources_.exists(request.resourceId)) {
                isResourceOwner = resources_.ownerOf(request.resourceId) == actingUserId;
            }

            if (!isRequester && !isAccepter && !isResourceOwner) {
                throw ForbiddenError("Only the two people involved in the exchange can "
                                     "mark it completed.");
            }
            return;
        }

        case RequestStatus::Pending:
            
            throw ConflictError("A request cannot be moved back to Pending (" + label + ").");
    }
}


HelpRequest RequestService::commit(const HelpRequest& request, bool affectsPendingSet) {
    auto it = idIndex_.find(request.getId());
    if (it == idIndex_.end()) {
        throw NotFoundError("Help request not found.");
    }

    requests_[it->second] = request;

    
    if (affectsPendingSet) {
        markQueueDirty();
    }

    persist();
    return requests_[it->second];
}

HelpRequest RequestService::acceptRequest(int requestId, int actingUserId) {
    HelpRequest request = requireRequest(requestId);
    authorizeTransition(request, actingUserId, RequestStatus::Accepted);

    request.status = RequestStatus::Accepted;
    request.acceptOwnerId = actingUserId;
    request.updatedAt = nowIso8601();

    const HelpRequest saved = commit(request, true);

    const std::string accepterName = users_.getPublicProfile(actingUserId).name;
    notifications_.notifyAccepted(request.requesterId, accepterName,
                                  request.title, request.getId());

    return saved;
}

HelpRequest RequestService::rejectRequest(int requestId, int actingUserId) {
    HelpRequest request = requireRequest(requestId);
    authorizeTransition(request, actingUserId, RequestStatus::Rejected);

    request.status = RequestStatus::Rejected;
    request.acceptOwnerId = 0;
    request.updatedAt = nowIso8601();

    const HelpRequest saved = commit(request, true);

    const std::string rejecterName = users_.getPublicProfile(actingUserId).name;
    notifications_.notifyRejected(request.requesterId, rejecterName,
                                  request.title, request.getId());

    return saved;
}

HelpRequest RequestService::cancelRequest(int requestId, int actingUserId) {
    HelpRequest request = requireRequest(requestId);
    authorizeTransition(request, actingUserId, RequestStatus::Cancelled);

    request.status = RequestStatus::Cancelled;
    request.updatedAt = nowIso8601();

    const HelpRequest saved = commit(request, true);

    // Tell the resource owner, if there is one.
    if (request.resourceId > 0 && resources_.exists(request.resourceId)) {
        const int ownerId = resources_.ownerOf(request.resourceId);
        const std::string requesterName = users_.getPublicProfile(request.requesterId).name;
        notifications_.notifyCancelled(ownerId, requesterName,
                                        request.title, request.getId());
    }

    return saved;
}

HelpRequest RequestService::completeRequest(int requestId, int actingUserId) {
    HelpRequest request = requireRequest(requestId);
    authorizeTransition(request, actingUserId, RequestStatus::Completed);

    request.status = RequestStatus::Completed;
    request.updatedAt = nowIso8601();

    const HelpRequest saved = commit(request, true);

    // Notify the OTHER party (the one who did not press the button).
    const int counterpart = (actingUserId == request.requesterId)
                                ? request.acceptOwnerId
                                : request.requesterId;
    if (counterpart > 0) {
        notifications_.notifyCompleted(counterpart, request.title, request.getId());
    }

    return saved;
}

HelpRequest RequestService::updateRequest(int requestId,
                                          int actingUserId,
                                          const std::string& title,
                                          const std::string& description,
                                          const std::string& requiredSkills,
                                          const std::string& category,
                                          Priority priority) {
    HelpRequest request = requireRequest(requestId);

    if (request.requesterId != actingUserId) {
        throw ForbiddenError("You can only edit requests that you created.");
    }
    if (request.status != RequestStatus::Pending) {
        throw ConflictError("Only a pending request can be edited. This one is already " +
                            toString(request.status) + ".");
    }

    const Priority oldPriority = request.priority;

    // Two-phase: validate into locals, then assign.
    std::string newTitle       = request.title;
    std::string newDescription = request.description;
    std::string newSkills      = request.requiredSkills;
    std::string newCategory    = request.category;

    if (!trim(title).empty())          newTitle = validateTitle(title);
    if (!trim(description).empty())    newDescription = validateDescription(description);
    if (!trim(requiredSkills).empty()) newSkills = joinList(splitList(requiredSkills));
    if (!trim(category).empty())       newCategory = categories::validate(category);

    request.title          = newTitle;
    request.description    = newDescription;
    request.requiredSkills = newSkills;
    request.category       = newCategory;
    request.priority       = priority;
    request.updatedAt      = nowIso8601();

    const HelpRequest saved = commit(request, true);

    
    if (priority != oldPriority) {
        notifications_.notifyPriorityChanged(request.requesterId, request.title,
                                              static_cast<int>(oldPriority),
                                              static_cast<int>(priority),
                                              request.getId());
    }

    return saved;
}


ContactDetails RequestService::contactDetailsFor(int requestId, int actingUserId) {
    if (actingUserId <= 0) {
        throw AuthError("Please log in to view contact details.");
    }

    const HelpRequest request = requireRequest(requestId);

    if (request.status != RequestStatus::Accepted &&
        request.status != RequestStatus::Completed) {
        throw ForbiddenError(
            "Contact details stay private until the request is accepted. This request is " +
            toString(request.status) + ".");
    }

    
    int counterpart = 0;

    if (actingUserId == request.requesterId) {
      
        counterpart = request.acceptOwnerId;
        if (counterpart == 0 && request.resourceId > 0 && resources_.exists(request.resourceId)) {
            counterpart = resources_.ownerOf(request.resourceId);
        }
    } else {
        
        const bool isAccepter = request.acceptOwnerId == actingUserId;
        bool isResourceOwner = false;
        if (request.resourceId > 0 && resources_.exists(request.resourceId)) {
            isResourceOwner = resources_.ownerOf(request.resourceId) == actingUserId;
        }

        if (isAccepter || isResourceOwner) {
            counterpart = request.requesterId;
        } else {
            throw ForbiddenError(
                "Only the two people involved in this exchange can see each other's "
                "contact details.");
        }
    }

    if (counterpart <= 0 || !users_.exists(counterpart)) {
        throw NotFoundError("The other participant's account is no longer available.");
    }

   
    const UserView profile = users_.getOwnProfile(counterpart);

    ContactDetails details;
    details.userId   = profile.id;
    details.name     = profile.name;
    details.email    = profile.email;
    details.phone    = profile.contactPhone;
    details.city     = profile.city;
    details.locality = profile.locality;

    return details;
}


std::size_t RequestService::countPendingForUser(int userId) const {
    std::size_t total = 0;
    for (const HelpRequest& request : requests_) {
        if (request.requesterId == userId && request.status == RequestStatus::Pending) ++total;
    }
    return total;
}

std::size_t RequestService::countAcceptedForUser(int userId) const {
    std::size_t total = 0;
    for (const HelpRequest& request : requests_) {
        if (request.status != RequestStatus::Accepted) continue;
        if (request.requesterId == userId || request.acceptOwnerId == userId) ++total;
    }
    return total;
}

std::size_t RequestService::countCompletedForUser(int userId) const {
    std::size_t total = 0;
    for (const HelpRequest& request : requests_) {
        if (request.status != RequestStatus::Completed) continue;
        if (request.requesterId == userId || request.acceptOwnerId == userId) ++total;
    }
    return total;
}


std::string RequestService::validateTitle(const std::string& value) {
    const std::string title = trim(value);

    if (title.empty()) {
        throw ValidationError("Request title is required.");
    }
    if (title.size() < 3) {
        throw ValidationError("Request title must be at least 3 characters long.");
    }
    if (title.size() > kMaxTitleLength) {
        throw ValidationError("Request title must be at most " +
                              std::to_string(kMaxTitleLength) + " characters long.");
    }
    if (hasControlCharacters(title)) {
        throw ValidationError("Request title contains invalid characters.");
    }

    return title;
}

std::string RequestService::validateDescription(const std::string& value) {
    const std::string description = trim(value);

    if (description.empty()) {
        throw ValidationError("Please describe the help you need.");
    }
    if (description.size() < 10) {
        throw ValidationError(
            "Please describe the help you need in at least 10 characters.");
    }
    if (description.size() > kMaxDescriptionLength) {
        throw ValidationError("Description must be at most " +
                              std::to_string(kMaxDescriptionLength) + " characters long.");
    }
    if (hasControlCharacters(description)) {
        throw ValidationError("Description contains invalid characters.");
    }

    return description;
}

} 
