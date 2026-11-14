// SearchService.h
// -----------------------------------------------------------------------------
// MODULE 4 - Search, skill matching and dashboard aggregation.
//
// This service keeps the search/matching rules out of the HTTP layer. It uses
// the data already loaded by UserService, ResourceService and RequestService;
// permanent storage remains the same local .txt files.
// -----------------------------------------------------------------------------

#ifndef CC_SEARCH_SERVICE_H
#define CC_SEARCH_SERVICE_H

#include <cstddef>
#include <string>
#include <vector>

#include "Models.h"
#include "NotificationService.h"
#include "RequestService.h"
#include "ResourceService.h"
#include "UserService.h"

namespace cc {

struct RankedResource {
    Resource resource;
    int score = 0;
    std::vector<std::string> matchedTerms;
};

struct UserMatch {
    UserView user;
    int score = 0;
    std::vector<std::string> matchedSkills;
    std::vector<std::string> matchedInterests;
};

struct DashboardStats {
    std::size_t resourcesOffered = 0;
    std::size_t activeHelpRequests = 0;   // Pending + Accepted created by the user
    std::size_t pendingRequests = 0;
    std::size_t acceptedRequests = 0;
    std::size_t completedExchanges = 0;
    std::size_t incomingRequests = 0;     // Pending requests against own resources
    std::size_t unreadNotifications = 0;
    bool hasHighestPriorityRequest = false;
    HelpRequest highestPriorityRequest;
};

class SearchService {
public:
    SearchService(const UserService& users,
                  const ResourceService& resources,
                  const RequestService& requests,
                  const NotificationService& notifications);

    // Search resources by title, description and category. Results are ranked
    // by a simple explainable score and then by newest resource id.
    std::vector<RankedResource> searchResources(const std::string& query,
                                                const std::string& category = "",
                                                bool onlyAvailable = false,
                                                std::size_t limit = 50) const;

    // Search public user profiles by skills/interests. Email and phone are
    // never present in UserMatch.user.
    std::vector<UserMatch> searchUsers(const std::string& skillQuery,
                                       const std::string& interestQuery = "",
                                       const std::string& city = "",
                                       std::size_t limit = 50) const;

    // Match users to the skills required by one help request.
    std::vector<UserMatch> matchUsersForRequest(int requestId,
                                                std::size_t limit = 20) const;

    // Aggregate the information needed by the dashboard in one call.
    DashboardStats dashboardFor(int userId) const;

private:
    static std::vector<std::string> queryTerms(const std::string& text);
    static bool containsInsensitive(const std::string& text,
                                    const std::string& query);
    static int resourceScore(const Resource& resource,
                             const std::vector<std::string>& terms,
                             const std::string& fullQuery,
                             std::vector<std::string>& matchedTerms);
    static UserMatch scoreUser(const UserView& user,
                               const std::vector<std::string>& requiredSkills,
                               const std::vector<std::string>& desiredInterests,
                               const std::string& city);

    const UserService& users_;
    const ResourceService& resources_;
    const RequestService& requests_;
    const NotificationService& notifications_;
};

} // namespace cc

#endif // CC_SEARCH_SERVICE_H
