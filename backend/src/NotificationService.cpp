// NotificationService.cpp
// -----------------------------------------------------------------------------
// MODULE 5 - in-app notifications.
// -----------------------------------------------------------------------------

#include "NotificationService.h"

#include <algorithm>

#include "Errors.h"

namespace cc {

NotificationService::NotificationService(FileManager& files) : files_(files) {
    notifications_ = files_.loadAll<Notification>(kFileName);

    int highestId = 0;
    for (std::size_t position = 0; position < notifications_.size(); ++position) {
        const int id = notifications_[position].getId();
        highestId = std::max(highestId, id);
        idIndex_[id] = position;
    }
    nextNotificationId_ = highestId + 1;
}

void NotificationService::persist() {
    files_.saveAll(kFileName, notifications_);
}

int NotificationService::notify(int receiverId,
                                const std::string& message,
                                const std::string& type) {
    if (receiverId <= 0) {
        // Defensive: a notification with no receiver can never be read, so it
        // would just be dead data in the file.
        throw ValidationError("A notification needs a valid receiver.");
    }

    Notification item;
    item.setId(nextNotificationId_++);
    item.setCreatedAt(nowIso8601());
    item.receiverId = receiverId;
    item.message = message;
    item.type = type;
    item.isRead = false;
    item.timestamp = item.getCreatedAt();

    notifications_.push_back(item);
    idIndex_[item.getId()] = notifications_.size() - 1;

    persist();
    return item.getId();
}

std::vector<Notification> NotificationService::listFor(int userId, bool onlyUnread) const {
    std::vector<Notification> result;

    for (const Notification& item : notifications_) {
        if (item.receiverId != userId) continue;   // authorisation by construction
        if (onlyUnread && item.isRead) continue;
        result.push_back(item);
    }

    // Newest first. Ids increase monotonically, so a descending id sort is the
    // same as a descending timestamp sort and avoids parsing dates.
    std::sort(result.begin(), result.end(),
              [](const Notification& a, const Notification& b) {
                  return a.getId() > b.getId();
              });

    return result;
}

std::size_t NotificationService::unreadCount(int userId) const {
    std::size_t count = 0;
    for (const Notification& item : notifications_) {
        if (item.receiverId == userId && !item.isRead) ++count;
    }
    return count;
}

void NotificationService::markRead(int userId, int notificationId) {
    auto it = idIndex_.find(notificationId);
    if (it == idIndex_.end()) {
        throw NotFoundError("Notification not found.");
    }

    Notification& item = notifications_[it->second];

    // Ownership check on the RECORD, not on the request: knowing an id is not
    // permission to read somebody else's notification.
    if (item.receiverId != userId) {
        throw ForbiddenError("This notification belongs to another user.");
    }

    if (item.isRead) {
        return; // already read: nothing to do, and not an error
    }

    item.isRead = true;
    persist();
}

std::size_t NotificationService::markAllRead(int userId) {
    std::size_t changed = 0;

    for (Notification& item : notifications_) {
        if (item.receiverId == userId && !item.isRead) {
            item.isRead = true;
            ++changed;
        }
    }

    if (changed > 0) persist();
    return changed;
}

// ---------------------------------------------------------------------------
// Convenience wrappers - one place for each sentence, so the UI stays consistent
// ---------------------------------------------------------------------------

void NotificationService::notifyResourceRequested(int ownerId, int requesterId,
                                                  const std::string& requesterName,
                                                  const std::string& resourceTitle,
                                                  int requestId) {
    (void)requesterId; // kept in the signature so callers read clearly

    const std::string message = requesterName + " requested your resource \"" +
                                resourceTitle + "\" (request #" +
                                std::to_string(requestId) + ").";
    notify(ownerId, message, notificationType::kResourceRequest);
}

void NotificationService::notifyAccepted(int requesterId, const std::string& ownerName,
                                         const std::string& requestTitle, int requestId) {
    const std::string message = ownerName + " accepted your request \"" + requestTitle +
                                "\" (#" + std::to_string(requestId) +
                                "). Their contact details are now visible on the request page.";
    notify(requesterId, message, notificationType::kRequestAccepted);
}

void NotificationService::notifyRejected(int requesterId, const std::string& ownerName,
                                         const std::string& requestTitle, int requestId) {
    const std::string message = ownerName + " rejected your request \"" + requestTitle +
                                "\" (#" + std::to_string(requestId) + ").";
    notify(requesterId, message, notificationType::kRequestRejected);
}

void NotificationService::notifyCancelled(int ownerId, const std::string& requesterName,
                                          const std::string& requestTitle, int requestId) {
    const std::string message = requesterName + " cancelled their request \"" +
                                requestTitle + "\" (#" + std::to_string(requestId) + ").";
    notify(ownerId, message, notificationType::kRequestCancelled);
}

void NotificationService::notifyCompleted(int userId, const std::string& requestTitle,
                                          int requestId) {
    const std::string message = "The exchange for \"" + requestTitle + "\" (#" +
                                std::to_string(requestId) + ") was marked completed.";
    notify(userId, message, notificationType::kRequestCompleted);
}

void NotificationService::notifyPriorityChanged(int requesterId,
                                                const std::string& requestTitle,
                                                int oldPriority, int newPriority,
                                                int requestId) {
    const std::string from = toString(static_cast<Priority>(oldPriority));
    const std::string to   = toString(static_cast<Priority>(newPriority));

    const std::string message = "Your request \"" + requestTitle + "\" (#" +
                                std::to_string(requestId) + ") changed priority from " +
                                from + " to " + to + ".";
    notify(requesterId, message, notificationType::kPriorityUpdate);
}

} // namespace cc
