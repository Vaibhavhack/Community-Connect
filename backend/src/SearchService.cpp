

#include "SearchService.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

#include "Errors.h"

namespace cc {

SearchService::SearchService(const UserService& users,
                             const ResourceService& resources,
                             const RequestService& requests,
                             const NotificationService& notifications)
    : users_(users),
      resources_(resources),
      requests_(requests),
      notifications_(notifications) {}

namespace {

std::string normaliseTerm(const std::string& value) {
    std::string out;
    out.reserve(value.size());

    for (char c : value) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) || c == '+' || c == '#' || c == '.') {
            out.push_back(static_cast<char>(std::tolower(u)));
        } else if (!out.empty() && out.back() != ' ') {
            out.push_back(' ');
        }
    }

    return trim(out);
}

bool termMatchesValue(const std::string& term, const std::string& value) {
    const std::string wanted = normaliseTerm(term);
    const std::string actual = normaliseTerm(value);
    if (wanted.empty() || actual.empty()) return false;
    if (wanted == actual) return true;

   
    std::istringstream words(actual);
    std::string word;
    while (words >> word) {
        if (word == wanted || word.find(wanted) != std::string::npos ||
            wanted.find(word) != std::string::npos) {
            return true;
        }
    }
    return actual.find(wanted) != std::string::npos;
}

std::vector<std::string> uniqueTerms(const std::vector<std::string>& input) {
    std::vector<std::string> result;
    std::set<std::string> seen;

    for (const std::string& raw : input) {
        const std::string term = normaliseTerm(raw);
        if (term.empty()) continue;
        if (seen.insert(term).second) result.push_back(term);
    }

    return result;
}

} 

std::vector<std::string> SearchService::queryTerms(const std::string& text) {
    std::vector<std::string> raw;
    std::string current;

    for (char c : text) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) || c == '+' || c == '#') {
            current.push_back(c);
        } else {
            if (!current.empty()) {
                raw.push_back(current);
                current.clear();
            }
        }
    }
    if (!current.empty()) raw.push_back(current);

    return uniqueTerms(raw);
}

bool SearchService::containsInsensitive(const std::string& text,
                                        const std::string& query) {
    const std::string wanted = toLowerCopy(trim(query));
    if (wanted.empty()) return true;
    return toLowerCopy(text).find(wanted) != std::string::npos;
}

int SearchService::resourceScore(const Resource& resource,
                                 const std::vector<std::string>& terms,
                                 const std::string& fullQuery,
                                 std::vector<std::string>& matchedTerms) {
    int score = 0;
    const std::string title = toLowerCopy(resource.title);
    const std::string category = toLowerCopy(resource.category);
    const std::string description = toLowerCopy(resource.description);
    const std::string query = toLowerCopy(trim(fullQuery));

    if (!query.empty() && title == query) score += 30;
    else if (!query.empty() && title.find(query) != std::string::npos) score += 20;

    if (!query.empty() && category == query) score += 15;
    else if (!query.empty() && category.find(query) != std::string::npos) score += 8;

    if (!query.empty() && description.find(query) != std::string::npos) score += 4;

    for (const std::string& term : terms) {
        bool matched = false;

        if (containsInsensitive(resource.title, term)) {
            score += 10;
            matched = true;
        }
        if (containsInsensitive(resource.category, term)) {
            score += 6;
            matched = true;
        }
        if (containsInsensitive(resource.description, term)) {
            score += 2;
            matched = true;
        }

        if (matched) matchedTerms.push_back(term);
    }

    return score;
}

std::vector<RankedResource> SearchService::searchResources(const std::string& query,
                                                           const std::string& category,
                                                           bool onlyAvailable,
                                                           std::size_t limit) const {
    const std::string cleanQuery = trim(query);
    const std::vector<std::string> terms = queryTerms(cleanQuery);
    const std::string wantedCategory = toLowerCopy(trim(category));

    std::vector<RankedResource> result;

    for (const Resource& resource : resources_.listAll()) {
        if (!wantedCategory.empty() && wantedCategory != "all" &&
            toLowerCopy(resource.category) != wantedCategory) {
            continue;
        }
        if (onlyAvailable && !resource.isAvailable()) continue;

        RankedResource item;
        item.resource = resource;

        if (cleanQuery.empty()) {
           
            item.score = 0;
        } else {
            item.score = resourceScore(resource, terms, cleanQuery, item.matchedTerms);
            if (item.score == 0) continue;
        }

        result.push_back(item);
    }

    std::sort(result.begin(), result.end(), [](const RankedResource& a,
                                               const RankedResource& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.resource.getId() > b.resource.getId();
    });

    if (limit > 0 && result.size() > limit) result.resize(limit);
    return result;
}

UserMatch SearchService::scoreUser(const UserView& user,
                                   const std::vector<std::string>& requiredSkills,
                                   const std::vector<std::string>& desiredInterests,
                                   const std::string& city) {
    UserMatch result;
    result.user = user;

    const std::vector<std::string> skills = splitList(user.skills);
    const std::vector<std::string> interests = splitList(user.interests);

    for (const std::string& required : requiredSkills) {
        for (const std::string& skill : skills) {
            if (termMatchesValue(required, skill)) {
                result.score += 10;
                result.matchedSkills.push_back(skill);
                break;
            }
        }
    }

    for (const std::string& desired : desiredInterests) {
        for (const std::string& interest : interests) {
            if (termMatchesValue(desired, interest)) {
                result.score += 3;
                result.matchedInterests.push_back(interest);
                break;
            }
        }
    }

    const std::string wantedCity = trim(city);
    if (!wantedCity.empty() &&
        toLowerCopy(trim(user.city)) == toLowerCopy(wantedCity)) {
        result.score += 2;
    }

    return result;
}

std::vector<UserMatch> SearchService::searchUsers(const std::string& skillQuery,
                                                  const std::string& interestQuery,
                                                  const std::string& city,
                                                  std::size_t limit) const {
    const std::vector<std::string> skills = splitList(skillQuery);
    const std::vector<std::string> interests = splitList(interestQuery);

    std::vector<UserMatch> result;
    for (const UserView& user : users_.listPublicProfiles()) {
        UserMatch match = scoreUser(user, skills, interests, city);
        if (match.score <= 0) continue;
        result.push_back(match);
    }

    std::sort(result.begin(), result.end(), [](const UserMatch& a, const UserMatch& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.user.id < b.user.id;
    });

    if (limit > 0 && result.size() > limit) result.resize(limit);
    return result;
}

std::vector<UserMatch> SearchService::matchUsersForRequest(int requestId,
                                                           std::size_t limit) const {
    const HelpRequest request = requests_.requireRequest(requestId);
    const std::vector<std::string> requiredSkills = request.requiredSkillList();

    if (requiredSkills.empty()) return {};

    std::vector<UserMatch> result;
    for (const UserView& user : users_.listPublicProfiles()) {
       
        if (user.id == request.requesterId) continue;

        UserMatch match = scoreUser(user, requiredSkills, {}, "");
        if (match.score <= 0) continue;
        result.push_back(match);
    }

    std::sort(result.begin(), result.end(), [](const UserMatch& a, const UserMatch& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.user.id < b.user.id;
    });

    if (limit > 0 && result.size() > limit) result.resize(limit);
    return result;
}

DashboardStats SearchService::dashboardFor(int userId) const {
    if (!users_.exists(userId)) {
        throw NotFoundError("User not found.");
    }

    DashboardStats stats;
    stats.resourcesOffered = resources_.countByOwner(userId);
    stats.pendingRequests = requests_.countPendingForUser(userId);
    stats.acceptedRequests = requests_.countAcceptedForUser(userId);
    stats.completedExchanges = requests_.countCompletedForUser(userId);
    stats.activeHelpRequests = stats.pendingRequests + stats.acceptedRequests;
    stats.incomingRequests = requests_.listIncoming(userId).size();
    stats.unreadNotifications = notifications_.unreadCount(userId);

    try {
        stats.highestPriorityRequest = requests_.peekHighestPriority();
        stats.hasHighestPriorityRequest = true;
    } catch (const AppError&) {
        stats.hasHighestPriorityRequest = false;
    }

    return stats;
}

} // namespace cc
