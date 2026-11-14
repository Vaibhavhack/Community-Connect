// SessionManager.h
// -----------------------------------------------------------------------------
// Purpose : Turn a successful login into a token, and a token back into a user.
//
// Why this class has to exist:
//   The original code kept "who is logged in" in a single int member. A web
//   server serves MANY browser tabs at once, so a process-wide int would make
//   every visitor share one account. Each login therefore gets its own random
//   token, and every request carries that token.
//
// Design
//   - Tokens are 32 random bytes (256 bits) from the OS CSPRNG, hex encoded.
//     Not guessable, not sequential, not derived from the user id.
//   - Sessions are held in an unordered_map<string, Session>: lookup is O(1).
//   - Each session expires after kIdleMinutes of inactivity (sliding window),
//     so an abandoned laptop does not stay logged in forever.
//   - Sessions live in memory only. Restarting the server logs everyone out
//     (the DATA is on disk; the sessions are not, and should not be).
//   - std::mutex because the HTTP server handles requests on several threads.
// -----------------------------------------------------------------------------

#ifndef CC_SESSION_MANAGER_H
#define CC_SESSION_MANAGER_H

#include <cstddef>
#include <ctime>
#include <mutex>
#include <string>
#include <unordered_map>

#include "Errors.h"
#include "Models.h"
#include "PasswordHasher.h"

namespace cc {

struct Session {
    std::string token;
    int userId = 0;
    long long lastUsedEpoch = 0;  // seconds, for the idle timeout
    std::string createdAt;
};

class SessionManager {
public:
    // Idle timeout. 120 minutes: long enough for a demo/viva, short enough to
    // be defensible in the report.
    static constexpr int kIdleMinutes = 120;

    // Creates a session and returns the token. Called only after the password
    // has already been verified by UserService.
    std::string createSession(int userId) {
        std::lock_guard<std::mutex> lock(mutex_);

        const std::string token = PasswordHasher::randomHex(32);

        Session session;
        session.token = token;
        session.userId = userId;
        session.lastUsedEpoch = nowEpoch();
        session.createdAt = nowIso8601();

        sessions_[token] = session;
        return token;
    }

    // Returns the user id for a valid token, refreshing its idle timer.
    // Throws AuthError if the token is unknown or expired.
    int requireUserId(const std::string& token) {
        if (token.empty()) {
            throw AuthError("Please log in to continue.");
        }

        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(token);
        if (it == sessions_.end()) {
            throw AuthError("Your session has expired. Please log in again.");
        }

        if (isExpired(it->second)) {
            sessions_.erase(it);
            throw AuthError("Your session has expired. Please log in again.");
        }

        it->second.lastUsedEpoch = nowEpoch();
        return it->second.userId;
    }

    // Same as requireUserId, but returns 0 instead of throwing when the token
    // is absent/invalid. For endpoints that work both logged in and out.
    int optionalUserId(const std::string& token) {
        try {
            return requireUserId(token);
        } catch (const AuthError&) {
            return 0;
        }
    }

    // Logout: invalidate exactly one token. Unknown tokens are ignored.
    bool destroySession(const std::string& token) {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.erase(token) > 0;
    }

    // Used when a password changes or an account is removed.
    void destroyAllSessionsFor(int userId) {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (it->second.userId == userId) {
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Removes expired sessions. main.cpp calls this periodically so the map
    // cannot grow without bound on a long-running server.
    std::size_t purgeExpired() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::size_t removed = 0;
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (isExpired(it->second)) {
                it = sessions_.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        return removed;
    }

    std::size_t activeSessionCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.size();
    }

private:
    static long long nowEpoch() {
        return static_cast<long long>(std::time(nullptr));
    }

    static bool isExpired(const Session& session) {
        const long long idleSeconds = nowEpoch() - session.lastUsedEpoch;
        return idleSeconds > (long long)kIdleMinutes * 60;
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Session> sessions_;
};

} // namespace cc

#endif // CC_SESSION_MANAGER_H
