#include "UserService.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace cc {

namespace {

std::string validatePhone(const std::string& value) {
    const std::string phone = trim(value);
    if (phone.empty()) return phone;

    if (phone.size() > 20) {
        throw ValidationError("Contact number is too long (maximum 20 characters).");
    }

    std::size_t digits = 0;
    for (char c : phone) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            ++digits;
        } else if (c == ' ' || c == '+' || c == '-' || c == '(' || c == ')' || c == '.') {
            continue;
        } else {
            throw ValidationError("Contact number may only contain digits, spaces, +, -, ( and ).");
        }
    }

    if (digits < 7) {
        throw ValidationError("Contact number must contain at least 7 digits.");
    }

    return phone;
}

bool hasControlCharacters(const std::string& value) {
    for (char c : value) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 && c != '\t') return true;
        if (u == 0x7F) return true;
    }
    return false;
}

}

UserService::UserService(FileManager& files, SessionManager& sessions)
    : files_(files), sessions_(sessions) {

    users_ = files_.loadAll<User>(kFileName);

    int highestId = 0;
    for (const User& user : users_) {
        highestId = std::max(highestId, user.getId());
    }
    nextUserId_ = highestId + 1;

    reindex();
}

void UserService::reindex() {
    idIndex_.clear();
    emailIndex_.clear();
    usernameIndex_.clear();

    for (std::size_t position = 0; position < users_.size(); ++position) {
        const User& user = users_[position];
        idIndex_[user.getId()] = position;

        const std::string emailKey = hashForLookup(user.email);
        if (!emailKey.empty() && emailIndex_.find(emailKey) == emailIndex_.end()) {
            emailIndex_[emailKey] = user.getId();
        }

        const std::string usernameKey = hashForLookup(user.username);
        if (!usernameKey.empty() && usernameIndex_.find(usernameKey) == usernameIndex_.end()) {
            usernameIndex_[usernameKey] = user.getId();
        }
    }
}

void UserService::persist() {
    files_.saveAll(kFileName, users_);
}

std::string UserService::validateName(const std::string& value) {
    const std::string name = trim(value);

    if (name.empty()) {
        throw ValidationError("Name is required.");
    }
    if (name.size() < 2) {
        throw ValidationError("Name must be at least 2 characters long.");
    }
    if (name.size() > 60) {
        throw ValidationError("Name must be at most 60 characters long.");
    }
    if (hasControlCharacters(name)) {
        throw ValidationError("Name contains invalid characters.");
    }

    return name;
}

std::string UserService::validateUsername(const std::string& value) {
    const std::string username = trim(value);

    if (username.empty()) {
        throw ValidationError("Username is required.");
    }
    if (username.size() < 3) {
        throw ValidationError("Username must be at least 3 characters long.");
    }
    if (username.size() > 30) {
        throw ValidationError("Username must be at most 30 characters long.");
    }

    for (char c : username) {
        const unsigned char u = static_cast<unsigned char>(c);
        const bool allowed = std::isalnum(u) || c == '.' || c == '_' || c == '-';
        if (!allowed) {
            throw ValidationError(
                "Username may only contain letters, digits, dot, underscore and hyphen.");
        }
    }

    if (!std::isalpha(static_cast<unsigned char>(username.front()))) {
        throw ValidationError("Username must start with a letter.");
    }

    return username;
}

std::string UserService::validateEmail(const std::string& value) {
    const std::string email = toLowerCopy(trim(value));

    if (email.empty()) {
        throw ValidationError("Email is required.");
    }
    if (email.size() > 100) {
        throw ValidationError("Email is too long (maximum 100 characters).");
    }
    if (hasControlCharacters(email)) {
        throw ValidationError("Email contains invalid characters.");
    }
    if (std::count(email.begin(), email.end(), '@') != 1) {
        throw ValidationError("Email must contain exactly one '@'.");
    }

    const std::size_t at = email.find('@');
    const std::string local = email.substr(0, at);
    const std::string domain = email.substr(at + 1);

    if (local.empty()) {
        throw ValidationError("Email is missing the part before '@'.");
    }
    if (domain.size() < 3 || domain.find('.') == std::string::npos) {
        throw ValidationError("Email domain looks invalid (for example: name@college.edu).");
    }
    if (domain.front() == '.' || domain.back() == '.' ||
        domain.find("..") != std::string::npos) {
        throw ValidationError("Email domain looks invalid.");
    }
    if (email.find(' ') != std::string::npos) {
        throw ValidationError("Email cannot contain spaces.");
    }

    return email;
}

std::string UserService::validatePassword(const std::string& value) {
    if (value.empty()) {
        throw ValidationError("Password is required.");
    }
    if (value.size() < kMinPasswordLength) {
        throw ValidationError("Password must be at least " +
                              std::to_string(kMinPasswordLength) + " characters long.");
    }
    if (value.size() > kMaxPasswordLength) {
        throw ValidationError("Password must be at most " +
                              std::to_string(kMaxPasswordLength) + " characters long.");
    }
    return value;
}

std::string UserService::validateCity(const std::string& value) {
    const std::string city = trim(value);

    if (city.empty()) {
        throw ValidationError("City is required.");
    }
    if (city.size() > 60) {
        throw ValidationError("City must be at most 60 characters long.");
    }
    if (hasControlCharacters(city)) {
        throw ValidationError("City contains invalid characters.");
    }

    return city;
}

std::string UserService::normaliseList(const std::string& value) {
    return joinList(splitList(value));
}

std::string UserService::hashForLookup(const std::string& value) {
    return toLowerCopy(trim(value));
}

User& UserService::requireUser(int userId) {
    auto it = idIndex_.find(userId);
    if (it == idIndex_.end()) {
        throw NotFoundError("User not found.");
    }
    return users_[it->second];
}

const User& UserService::requireUser(int userId) const {
    auto it = idIndex_.find(userId);
    if (it == idIndex_.end()) {
        throw NotFoundError("User not found.");
    }
    return users_[it->second];
}

bool UserService::exists(int userId) const {
    return userId > 0 && idIndex_.find(userId) != idIndex_.end();
}

int UserService::findIdByUsernameOrEmail(const std::string& identifier) const {
    const std::string key = hashForLookup(identifier);
    if (key.empty()) return 0;

    auto byEmail = emailIndex_.find(key);
    if (byEmail != emailIndex_.end()) return byEmail->second;

    auto byUsername = usernameIndex_.find(key);
    if (byUsername != usernameIndex_.end()) return byUsername->second;

    return 0;
}

int UserService::registerUser(const std::string& name,
                              const std::string& username,
                              const std::string& email,
                              const std::string& password,
                              const std::string& city,
                              const std::string& locality,
                              const std::string& skills,
                              const std::string& interests,
                              const std::string& contactPhone) {

    User user;
    user.name         = validateName(name);
    user.username     = validateUsername(username);
    user.email        = validateEmail(email);
    user.city         = validateCity(city);
    user.locality     = trim(locality);
    user.skills       = normaliseList(skills);
    user.interests    = normaliseList(interests);
    user.contactPhone = validatePhone(contactPhone);

    if (user.locality.size() > 60) {
        throw ValidationError("Locality must be at most 60 characters long.");
    }

    validatePassword(password);

    if (emailIndex_.find(hashForLookup(user.email)) != emailIndex_.end()) {
        throw ConflictError("An account with this email already exists.");
    }
    if (usernameIndex_.find(hashForLookup(user.username)) != usernameIndex_.end()) {
        throw ConflictError("This username is already taken.");
    }

    user.setId(nextUserId_++);
    user.setCreatedAt(nowIso8601());
    user.passwordHash = PasswordHasher::createHash(password);

    users_.push_back(user);

    idIndex_[user.getId()] = users_.size() - 1;
    emailIndex_[hashForLookup(user.email)] = user.getId();
    usernameIndex_[hashForLookup(user.username)] = user.getId();

    persist();

    return user.getId();
}

std::string UserService::login(const std::string& identifier, const std::string& password) {
    const int userId = findIdByUsernameOrEmail(identifier);

    const std::string failure = "Invalid username/email or password.";

    if (userId == 0) {
        throw AuthError(failure);
    }

    const User& user = requireUser(userId);

    if (!PasswordHasher::verify(password, user.passwordHash)) {
        throw AuthError(failure);
    }

    return sessions_.createSession(userId);
}

void UserService::logout(const std::string& token) {
    sessions_.destroySession(token);
}

UserView UserService::toView(const User& user, bool includePrivate) {
    UserView view;
    view.id        = user.getId();
    view.name      = user.name;
    view.username  = user.username;
    view.city      = user.city;
    view.locality  = user.locality;
    view.skills    = user.skills;
    view.interests = user.interests;
    view.createdAt = user.getCreatedAt();

    if (includePrivate) {
        view.email        = user.email;
        view.contactPhone = user.contactPhone;
    }

    return view;
}

UserView UserService::getOwnProfile(int userId) {
    return toView(requireUser(userId), true);
}

UserView UserService::getPublicProfile(int userId) const {
    return toView(requireUser(userId), false);
}

UserView UserService::publicProfileOf(int userId) const {
    if (!exists(userId)) {
        throw NotFoundError("User not found.");
    }
    return toView(requireUser(userId), false);
}

std::vector<UserView> UserService::listPublicProfiles() const {
    std::vector<UserView> result;
    result.reserve(users_.size());
    for (const User& user : users_) {
        result.push_back(toView(user, false));
    }
    std::sort(result.begin(), result.end(), [](const UserView& a, const UserView& b) {
        return a.id < b.id;
    });
    return result;
}

std::string UserService::privateContactOf(int userId) const {
    if (!exists(userId)) {
        throw NotFoundError("User not found.");
    }
    const User& user = requireUser(userId);

    std::string contact = user.email;
    if (!user.contactPhone.empty()) {
        contact += " / " + user.contactPhone;
    }
    return contact;
}

UserView UserService::updateProfile(int userId,
                                    const std::string& name,
                                    const std::string& city,
                                    const std::string& locality,
                                    const std::string& skills,
                                    const std::string& interests,
                                    const std::string& contactPhone,
                                    const std::string& newPassword,
                                    const std::string& currentPassword) {

    User& user = requireUser(userId);

    std::string newName      = user.name;
    std::string newCity      = user.city;
    std::string newLocality  = user.locality;
    std::string newSkills    = user.skills;
    std::string newInterests = user.interests;
    std::string newPhone     = user.contactPhone;
    std::string newHash      = user.passwordHash;

    if (!trim(name).empty())         newName = validateName(name);
    if (!trim(city).empty())         newCity = validateCity(city);
    if (!trim(locality).empty()) {
        const std::string value = trim(locality);
        if (value.size() > 60) {
            throw ValidationError("Locality must be at most 60 characters long.");
        }
        newLocality = value;
    }
    if (!trim(skills).empty())       newSkills = normaliseList(skills);
    if (!trim(interests).empty())    newInterests = normaliseList(interests);
    if (!trim(contactPhone).empty()) newPhone = validatePhone(contactPhone);

    if (!newPassword.empty()) {
        if (currentPassword.empty() ||
            !PasswordHasher::verify(currentPassword, user.passwordHash)) {
            throw AuthError("Current password is incorrect.");
        }

        validatePassword(newPassword);
        newHash = PasswordHasher::createHash(newPassword);
    }

    user.name         = newName;
    user.city         = newCity;
    user.locality     = newLocality;
    user.skills       = newSkills;
    user.interests    = newInterests;
    user.contactPhone = newPhone;
    user.passwordHash = newHash;

    persist();

    return toView(user, true);
}

} 
