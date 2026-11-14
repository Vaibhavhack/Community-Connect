// NotificationService.h
// -----------------------------------------------------------------------------
// Purpose : MODULE 5 - in-app notifications.
//
// Pulled into Stage 3 (rather than Stage 4) because accept / reject / complete
// are only correct if they tell the other person something happened. A status
// change that nobody is told about is not a working feature.
//
// Design notes
//   - A notification is written the moment the event happens, so it survives a
//     restart (requirement: "Retrieve notifications after application restart").
//   - Reads are cheap: unreadCount() and listFor() both filter a vector that is
//     already in memory, and the receiver check is part of every method so one
//     user can never read or mark another user's notification.
//   - The service never builds the message text for a domain event - callers
//     pass a complete sentence - EXCEPT for the convenience helpers at the
//     bottom, which keep the wording identical everywhere it is triggered.
// -----------------------------------------------------------------------------

#ifndef CC_NOTIFICATION_SERVICE_H
#define CC_NOTIFICATION_SERVICE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "FileManager.h"
#include "Models.h"

namespace cc {

// The six events the platform notifies about.
namespace notificationType {
    constexpr const char* kResourceRequest   = "resource_request";
    constexpr const char* kRequestAccepted   = "request_accepted";
    constexpr const char* kRequestRejected   = "request_rejected";
    constexpr const char* kRequestCancelled  = "request_cancelled";
    constexpr const char* kRequestCompleted  = "request_completed";
    constexpr const char* kPriorityUpdate    = "priority_update";
}

class NotificationService {
public:
    static constexpr const char* kFileName = "notifications.txt";

    explicit NotificationService(FileManager& files);

    // Creates and saves one notification. Returns the new notification id.
    int notify(int receiverId, const std::string& message, const std::string& type);

    // Newest first. onlyUnread = true gives just the unread ones.
    std::vector<Notification> listFor(int userId, bool onlyUnread = false) const;

    std::size_t unreadCount(int userId) const;
    std::size_t totalCount() const { return notifications_.size(); }

    // Marks ONE notification read. Throws ForbiddenError if it belongs to
    // somebody else, NotFoundError if it does not exist.
    void markRead(int userId, int notificationId);

    // Returns how many were actually changed.
    std::size_t markAllRead(int userId);

    // Convenience wrappers, so the wording of each event is defined once.
    void notifyResourceRequested(int ownerId, int requesterId,
                                 const std::string& requesterName,
                                 const std::string& resourceTitle,
                                 int requestId);
    void notifyAccepted(int requesterId, const std::string& ownerName,
                        const std::string& requestTitle, int requestId);
    void notifyRejected(int requesterId, const std::string& ownerName,
                        const std::string& requestTitle, int requestId);
    void notifyCancelled(int ownerId, const std::string& requesterName,
                         const std::string& requestTitle, int requestId);
    void notifyCompleted(int userId, const std::string& requestTitle, int requestId);
    void notifyPriorityChanged(int requesterId, const std::string& requestTitle,
                               int oldPriority, int newPriority, int requestId);

private:
    void persist();

    FileManager& files_;
    std::vector<Notification> notifications_;
    std::unordered_map<int, std::size_t> idIndex_;  // notification id -> position
    int nextNotificationId_ = 1;
};

} // namespace cc

#endif // CC_NOTIFICATION_SERVICE_H
