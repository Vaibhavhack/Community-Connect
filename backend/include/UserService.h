// UserService.h
// -----------------------------------------------------------------------------
// Purpose : MODULE 1 - User Management (register, login, profile, edit, auth).
//
// Everything the original UserManagement.cpp did, moved out of the console:
//   - no cout/cin here; results are returned and errors are thrown
//   - validation is real (email shape, lengths, allowed characters)
//   - passwords are hashed with a per-user salt, never stored in plain text
//   - duplicate checks are O(1) through unordered_map indexes
//
// The service owns the users list and keeps two indexes in sync with it:
//
//     emailIndex_    lowercase email    -> user id      (duplicate prevention)
//     usernameIndex_ lowercase username -> user id      (duplicate prevention)
//
// Both are rebuilt from disk on construction, so a restart restores them.
// -----------------------------------------------------------------------------

#ifndef CC_USER_SERVICE_H
#define CC_USER_SERVICE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "FileManager.h"
#include "Models.h"
#include "PasswordHasher.h"
#include "SessionManager.h"

namespace cc {

// What the browser is allowed to see about a user.
//   full    : own profile -> includes email and contact phone
//   public  : anyone else -> NO email, NO phone, NO password hash
struct UserView {
    int id = 0;
    std::string name;
    std::string username;
    std::string email;
    std::string city;
    std::string locality;
    std::string skills;
    std::string interests;
    std::string contactPhone;
    std::string createdAt;
};

class UserService {
public:
    static constexpr const char* kFileName = "users.txt";

    static constexpr std::size_t kMinPasswordLength = 6;
    static constexpr std::size_t kMaxPasswordLength = 72;

    UserService(FileManager& files, SessionManager& sessions);

    // ---- MODULE 1 operations ------------------------------------------------

    // Returns the new user id. Throws ValidationError / ConflictError.
    int registerUser(const std::string& name,
                     const std::string& username,
                     const std::string& email,
                     const std::string& password,
                     const std::string& city,
                     const std::string& locality,
                     const std::string& skills,
                     const std::string& interests,
                     const std::string& contactPhone);

    // Returns a session token on success, throws AuthError on failure.
    // Accepts either the username or the email address, case-insensitively.
    std::string login(const std::string& identifier, const std::string& password);

    void logout(const std::string& token);

    UserView getOwnProfile(int userId);
    UserView getPublicProfile(int userId) const;

    // Any field left empty means "do not change it". Changing the password
    // requires newPassword (and logs out other sessions).
    UserView updateProfile(int userId,
                           const std::string& name,
                           const std::string& city,
                           const std::string& locality,
                           const std::string& skills,
                           const std::string& interests,
                           const std::string& contactPhone,
                           const std::string& newPassword,
                           const std::string& currentPassword);

    // ---- lookups used by other modules -------------------------------------
    bool exists(int userId) const;
    int findIdByUsernameOrEmail(const std::string& identifier) const;
    UserView publicProfileOf(int userId) const;
    // Public views for search/matching. Never contains password, email or phone.
    std::vector<UserView> listPublicProfiles() const;

    // Contact details are private: only the user themselves, or somebody who
    // has an ACCEPTED request with them, may read them. Enforced here, not in
    // JavaScript. (The request-related case is handled in RequestService.)
    std::string privateContactOf(int userId) const;

    std::size_t userCount() const { return users_.size(); }

private:
    // Validation helpers - they throw ValidationError with a clear message.
    static std::string validateName(const std::string& value);
    static std::string validateUsername(const std::string& value);
    static std::string validateEmail(const std::string& value);
    static std::string validatePassword(const std::string& value);
    static std::string validateCity(const std::string& value);
    static std::string normaliseList(const std::string& value);

    User& requireUser(int userId);
    const User& requireUser(int userId) const;

    void persist();
    void reindex();

    static UserView toView(const User& user, bool includePrivate);
    static std::string hashForLookup(const std::string& value);

    FileManager& files_;
    SessionManager& sessions_;

    std::vector<User> users_;
    std::unordered_map<int, std::size_t> idIndex_;        // user id -> vector position
    std::unordered_map<std::string, int> emailIndex_;     // lowercase email -> user id
    std::unordered_map<std::string, int> usernameIndex_;  // lowercase username -> user id

    int nextUserId_ = 1;
};

} // namespace cc

#endif // CC_USER_SERVICE_H
