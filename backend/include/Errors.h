// Errors.h
// -----------------------------------------------------------------------------
// Purpose : One small exception hierarchy used by every layer of the backend.
//
// Why it exists:
//   The service layer must never know about HTTP. Instead it throws a typed
//   error, and the router (Stage 5) turns the TYPE of that error into an HTTP
//   status code:
//
//       ValidationError -> 422   AuthError -> 401   ForbiddenError -> 403
//       NotFoundError   -> 404   ConflictError -> 409
//
//   That is real polymorphism doing real work (virtual code() + a status that
//   the base class already knows), not inheritance added just to look advanced.
// -----------------------------------------------------------------------------

#ifndef CC_ERRORS_H
#define CC_ERRORS_H

#include <stdexcept>
#include <string>

namespace cc {

// Base class for every error the application raises on purpose.
class AppError : public std::runtime_error {
public:
    AppError(const std::string& message, int httpStatus)
        : std::runtime_error(message), httpStatus_(httpStatus) {}

    // HTTP status the router should answer with.
    int httpStatus() const { return httpStatus_; }

    // Stable machine-readable code, e.g. "validation_error".
    virtual std::string code() const { return "error"; }

private:
    int httpStatus_;
};

// Bad input from the user: missing field, wrong length, unknown category...
class ValidationError : public AppError {
public:
    explicit ValidationError(const std::string& message)
        : AppError(message, 422) {}
    std::string code() const override { return "validation_error"; }
};

// Not logged in, or wrong credentials.
class AuthError : public AppError {
public:
    explicit AuthError(const std::string& message)
        : AppError(message, 401) {}
    std::string code() const override { return "unauthenticated"; }
};

// Logged in, but not allowed to touch THIS record.
class ForbiddenError : public AppError {
public:
    explicit ForbiddenError(const std::string& message)
        : AppError(message, 403) {}
    std::string code() const override { return "forbidden"; }
};

class NotFoundError : public AppError {
public:
    explicit NotFoundError(const std::string& message)
        : AppError(message, 404) {}
    std::string code() const override { return "not_found"; }
};

// Duplicate email/username, illegal state change, resource still in use...
class ConflictError : public AppError {
public:
    explicit ConflictError(const std::string& message)
        : AppError(message, 409) {}
    std::string code() const override { return "conflict"; }
};

// Disk problem: cannot create data folder, cannot write temp file, ...
class StorageError : public AppError {
public:
    explicit StorageError(const std::string& message)
        : AppError(message, 500) {}
    std::string code() const override { return "storage_error"; }
};

} // namespace cc

#endif // CC_ERRORS_H
