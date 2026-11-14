// Models.h
// -----------------------------------------------------------------------------
// Purpose : The four record types stored in data/*.txt plus the enums and the
//           validation/normalisation helpers they share.
//
// Storage layout (one record per line, '|' separated, escaped by FileManager):
//
//   User          : id | createdAt | name | username | email | passwordHash |
//                   city | locality | skills | interests | contactPhone
//   Resource      : id | createdAt | ownerId | title | description | category |
//                   availability | dateAdded
//   HelpRequest   : id | createdAt | requesterId | resourceId | title |
//                   description | requiredSkills | category | priority | status |
//                   acceptOwnerId | updatedAt
//   Notification  : id | createdAt | receiverId | message | type | isRead |
//                   timestamp
//
// IMPORTANT: the order of toRecord()/fromRecord() fields is the file format.
// If you ever change it, bump the header written by FileManager and write a
// migration - otherwise old data files will be misread.
// -----------------------------------------------------------------------------

#ifndef CC_MODELS_H
#define CC_MODELS_H

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "Entity.h"
#include "Errors.h"

namespace cc {

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

// High > Medium > Low. The numeric value is used directly by the heap, so the
// order of these numbers IS the priority order - do not renumber casually.
enum class Priority { Low = 1, Medium = 2, High = 3 };

enum class RequestStatus { Pending, Accepted, Rejected, Completed, Cancelled };

enum class Availability { Available, Reserved, Unavailable };

inline std::string toString(Priority value) {
    switch (value) {
        case Priority::High:   return "High";
        case Priority::Medium: return "Medium";
        case Priority::Low:    return "Low";
    }
    return "Low";
}

inline std::string toString(RequestStatus value) {
    switch (value) {
        case RequestStatus::Pending:   return "Pending";
        case RequestStatus::Accepted:  return "Accepted";
        case RequestStatus::Rejected:  return "Rejected";
        case RequestStatus::Completed: return "Completed";
        case RequestStatus::Cancelled: return "Cancelled";
    }
    return "Pending";
}

inline std::string toString(Availability value) {
    switch (value) {
        case Availability::Available:   return "Available";
        case Availability::Reserved:    return "Reserved";
        case Availability::Unavailable: return "Unavailable";
    }
    return "Available";
}

// Case-insensitive parsing: the frontend may send "high", "HIGH" or "High".
inline Priority parsePriority(const std::string& text) {
    const std::string value = [&text] {
        std::string out = text;
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c) { return char(std::tolower(c)); });
        return out;
    }();

    if (value == "high" || value == "3")   return Priority::High;
    if (value == "medium" || value == "2") return Priority::Medium;
    if (value == "low" || value == "1")    return Priority::Low;

    throw ValidationError("Priority must be High, Medium or Low.");
}

inline RequestStatus parseRequestStatus(const std::string& text) {
    std::string value = text;
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });

    if (value == "pending")   return RequestStatus::Pending;
    if (value == "accepted")  return RequestStatus::Accepted;
    if (value == "rejected")  return RequestStatus::Rejected;
    if (value == "completed") return RequestStatus::Completed;
    if (value == "cancelled" || value == "canceled") return RequestStatus::Cancelled;

    throw ValidationError("Unknown request status: " + text);
}

inline Availability parseAvailability(const std::string& text) {
    std::string value = text;
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });

    if (value == "available")   return Availability::Available;
    if (value == "reserved")    return Availability::Reserved;
    if (value == "unavailable") return Availability::Unavailable;

    throw ValidationError("Unknown availability: " + text);
}

// ---------------------------------------------------------------------------
// String helpers (skills, interests, categories)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Timestamps
// ---------------------------------------------------------------------------
// Every record stores an ISO-8601 UTC timestamp, e.g. "2026-11-13T09:41:07Z".
// This format sorts correctly as a plain string, which is why the priority
// queue can break ties with a string comparison.
inline std::string nowIso8601() {
    const std::time_t stamp = std::time(nullptr);
    std::tm parts{};
#if defined(_WIN32)
    gmtime_s(&parts, &stamp);
#else
    gmtime_r(&stamp, &parts);
#endif

    std::ostringstream out;
    out << std::put_time(&parts, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

inline std::string todayIso8601() { return nowIso8601().substr(0, 10); }

inline std::string trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();

    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;

    return text.substr(begin, end - begin);
}

inline std::string toLowerCopy(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    return out;
}

// "C++ , DSA,  c++" -> {"C++", "DSA"}   (trimmed, de-duplicated case-insensitively)
inline std::vector<std::string> splitList(const std::string& text) {
    std::vector<std::string> items;
    std::string current;
    std::vector<std::string> seenLower;

    auto flush = [&]() {
        std::string item = trim(current);
        current.clear();
        if (item.empty()) return;

        const std::string lower = toLowerCopy(item);
        if (std::find(seenLower.begin(), seenLower.end(), lower) != seenLower.end()) {
            return; // duplicate skill, ignore
        }
        seenLower.push_back(lower);
        items.push_back(item);
    };

    for (char c : text) {
        // Only comma and semicolon separate skills. A slash must NOT split,
        // otherwise "HTML/CSS" or "C/C++" would become two bogus skills.
        if (c == ',' || c == ';') {
            flush();
        } else {
            current.push_back(c);
        }
    }
    flush();

    return items;
}

inline std::string joinList(const std::vector<std::string>& items) {
    std::string out;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) out += ", ";
        out += items[i];
    }
    return out;
}

// ---------------------------------------------------------------------------
// Educational categories (requirement section 6 - ONLY these nine)
// ---------------------------------------------------------------------------
namespace categories {

inline const std::vector<std::string>& all() {
    static const std::vector<std::string> list = {
        "Books and Textbooks",
        "Notes and Study Materials",
        "Previous Year Question Papers",
        "Stationery",
        "Academic Projects",
        "Programming Resources",
        "Online Learning Resources",
        "Educational Equipment",
        "Other Educational Resources"
    };
    return list;
}

// Returns the canonical spelling, or throws. This is the single gate used by
// the API, so the frontend dropdown and the backend can never drift apart.
inline std::string validate(const std::string& input) {
    const std::string wanted = toLowerCopy(trim(input));
    if (wanted.empty()) {
        throw ValidationError("Category is required.");
    }

    for (const std::string& category : all()) {
        if (toLowerCopy(category) == wanted) {
            return category; // store the official spelling
        }
    }

    throw ValidationError("'" + input + "' is not a valid educational category. "
                          "Choose one of the nine predefined categories.");
}

} // namespace categories

// ---------------------------------------------------------------------------
// User
// ---------------------------------------------------------------------------
struct User : public Entity {
    std::string name;
    std::string username;
    std::string email;
    std::string passwordHash;   // NEVER sent to the browser
    std::string city;
    std::string locality;       // optional
    std::string skills;         // comma separated, display form
    std::string interests;      // comma separated, display form
    std::string contactPhone;   // private: revealed only after an accepted request

    std::string toRecord() const override {
        return std::to_string(id_) + "|" + createdAt_ + "|" + name + "|" +
               username + "|" + email + "|" + passwordHash + "|" + city + "|" +
               locality + "|" + skills + "|" + interests + "|" + contactPhone;
    }

    void fromRecord(const Record& fields) override {
        requireFields(fields, 11, "user");
        id_          = parseInt(fields[0], "user id");
        createdAt_   = fields[1];
        name         = fields[2];
        username     = fields[3];
        email        = fields[4];
        passwordHash = fields[5];
        city         = fields[6];
        locality     = fields[7];
        skills       = fields[8];
        interests    = fields[9];
        contactPhone = fields[10];
    }

    std::string describe() const override {
        return "#" + std::to_string(id_) + " " + name + " (@" + username + ")";
    }

    // Skills as a clean vector - used by the matching module in Stage 4.
    std::vector<std::string> skillList() const { return splitList(skills); }
    std::vector<std::string> interestList() const { return splitList(interests); }
};

// ---------------------------------------------------------------------------
// Resource
// ---------------------------------------------------------------------------
struct Resource : public Entity {
    int ownerId = 0;
    std::string title;
    std::string description;
    std::string category;
    Availability availability = Availability::Available;
    std::string dateAdded;   // same value as createdAt_, kept explicit for the UI

    std::string toRecord() const override {
        return std::to_string(id_) + "|" + createdAt_ + "|" +
               std::to_string(ownerId) + "|" + title + "|" + description + "|" +
               category + "|" + toString(availability) + "|" + dateAdded;
    }

    void fromRecord(const Record& fields) override {
        requireFields(fields, 8, "resource");
        id_          = parseInt(fields[0], "resource id");
        createdAt_   = fields[1];
        ownerId      = parseInt(fields[2], "owner id");
        title        = fields[3];
        description  = fields[4];
        category     = fields[5];
        availability = parseAvailability(fields[6]);
        dateAdded    = fields[7];

        if (dateAdded.empty()) dateAdded = createdAt_;
    }

    std::string describe() const override {
        return "#" + std::to_string(id_) + " " + title + " [" + category + "]";
    }

    bool isAvailable() const { return availability == Availability::Available; }
};

// ---------------------------------------------------------------------------
// HelpRequest
// ---------------------------------------------------------------------------
// resourceId == 0 means "general help request" (not tied to one resource).
// That is how the spec's field list and the "request this resource" flow both
// work with a single record type.
// ---------------------------------------------------------------------------
struct HelpRequest : public Entity {
    int requesterId = 0;
    int resourceId = 0;
    std::string title;
    std::string description;
    std::string requiredSkills;
    std::string category;
    Priority priority = Priority::Medium;
    RequestStatus status = RequestStatus::Pending;
    int acceptOwnerId = 0;    // 0 until somebody accepts
    std::string updatedAt;

    std::string toRecord() const override {
        return std::to_string(id_) + "|" + createdAt_ + "|" +
               std::to_string(requesterId) + "|" + std::to_string(resourceId) + "|" +
               title + "|" + description + "|" + requiredSkills + "|" + category +
               "|" + toString(priority) + "|" + toString(status) + "|" +
               std::to_string(acceptOwnerId) + "|" + updatedAt;
    }

    void fromRecord(const Record& fields) override {
        requireFields(fields, 12, "help request");
        id_            = parseInt(fields[0], "request id");
        createdAt_     = fields[1];
        requesterId    = parseInt(fields[2], "requester id");
        resourceId     = parseInt(fields[3], "resource id");
        title          = fields[4];
        description    = fields[5];
        requiredSkills = fields[6];
        category       = fields[7];
        priority       = parsePriority(fields[8]);
        status         = parseRequestStatus(fields[9]);
        acceptOwnerId  = parseInt(fields[10], "accepting owner id");
        updatedAt      = fields[11];

        if (updatedAt.empty()) updatedAt = createdAt_;
    }

    std::string describe() const override {
        return "#" + std::to_string(id_) + " " + title + " (" +
               toString(priority) + ", " + toString(status) + ")";
    }

    bool isPending() const { return status == RequestStatus::Pending; }
    std::vector<std::string> requiredSkillList() const { return splitList(requiredSkills); }
};

// ---------------------------------------------------------------------------
// Notification
// ---------------------------------------------------------------------------
// type is one of: resource_request | request_accepted | request_rejected |
//                 request_cancelled | request_completed | priority_update
// ---------------------------------------------------------------------------
struct Notification : public Entity {
    int receiverId = 0;
    std::string message;
    std::string type;
    bool isRead = false;
    std::string timestamp;

    std::string toRecord() const override {
        return std::to_string(id_) + "|" + createdAt_ + "|" +
               std::to_string(receiverId) + "|" + message + "|" + type + "|" +
               (isRead ? "1" : "0") + "|" + timestamp;
    }

    void fromRecord(const Record& fields) override {
        requireFields(fields, 7, "notification");
        id_         = parseInt(fields[0], "notification id");
        createdAt_  = fields[1];
        receiverId  = parseInt(fields[2], "receiver id");
        message     = fields[3];
        type        = fields[4];
        isRead      = parseBool(fields[5]);
        timestamp   = fields[6];

        if (timestamp.empty()) timestamp = createdAt_;
    }

    std::string describe() const override {
        return "#" + std::to_string(id_) + " " + type + ": " + message;
    }
};

} // namespace cc

#endif // CC_MODELS_H
