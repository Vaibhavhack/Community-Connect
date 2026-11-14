
#include "PasswordHasher.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include "Errors.h"

#if defined(__APPLE__)
  #include <CommonCrypto/CommonCrypto.h>
  #include <CommonCrypto/CommonKeyDerivation.h>
  #include <CommonCrypto/CommonRandom.h>   
#else
  #include <openssl/evp.h>
  #include <openssl/rand.h>
#endif

namespace cc {

namespace {

const char* kBase64Alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

bool isBase64Char(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '+' || c == '/';
}

int base64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}


bool splitHash(const std::string& stored, std::string parts[4]) {
    std::size_t start = 0;
    for (int i = 0; i < 4; ++i) {
        const std::size_t end = stored.find('$', start);
        if (i < 3) {
            if (end == std::string::npos) return false;
            parts[i] = stored.substr(start, end - start);
            start = end + 1;
        } else {
            parts[i] = stored.substr(start); 
        }
    }
    return !parts[0].empty() && !parts[1].empty() &&
           !parts[2].empty() && !parts[3].empty();
}

} 

namespace {


std::vector<std::uint8_t> randomBytesImpl(std::size_t count) {
    std::vector<std::uint8_t> buffer(count, 0);
    if (count == 0) return buffer;

#if defined(__APPLE__)
    const int result = CCRandomGenerateBytes(buffer.data(), count);
    if (result != kCCSuccess) {
        throw StorageError("The system random number generator failed.");
    }
#else
    if (RAND_bytes(buffer.data(), static_cast<int>(count)) != 1) {
        throw StorageError("The system random number generator failed.");
    }
#endif

    return buffer;
}


std::vector<std::uint8_t> pbkdf2Impl(const std::string& password,
                                     const std::vector<std::uint8_t>& salt,
                                     int iterations,
                                     std::size_t keyLength) {
    std::vector<std::uint8_t> key(keyLength, 0);

#if defined(__APPLE__)
    const int status = CCKeyDerivationPBKDF(
        kCCPBKDF2,
        password.data(), password.size(),
        salt.data(), salt.size(),
        kCCPRFHmacAlgSHA256,
        static_cast<unsigned int>(iterations),
        key.data(), key.size());

    if (status != kCCSuccess) {
        throw StorageError("PBKDF2 key derivation failed.");
    }
#else
    const int ok = PKCS5_PBKDF2_HMAC(
        password.data(), static_cast<int>(password.size()),
        salt.data(), static_cast<int>(salt.size()),
        iterations, EVP_sha256(),
        static_cast<int>(key.size()), key.data());

    if (ok != 1) {
        throw StorageError("PBKDF2 key derivation failed.");
    }
#endif

    return key;
}

} 
std::vector<std::uint8_t> PasswordHasher::randomBytes(std::size_t count) {
    return randomBytesImpl(count);
}

std::vector<std::uint8_t> PasswordHasher::pbkdf2(
    const std::string& password,
    const std::vector<std::uint8_t>& salt,
    int iterations,
    std::size_t keyLength) {
    return pbkdf2Impl(password, salt, iterations, keyLength);
}

std::string PasswordHasher::createHash(const std::string& password) {
    if (password.empty()) {
        throw ValidationError("Password cannot be empty.");
    }

    const std::vector<std::uint8_t> salt = randomBytes(kSaltBytes);
    const std::vector<std::uint8_t> key = pbkdf2(password, salt, kIterations, kKeyBytes);

    return std::string("pbkdf2_sha256$") + std::to_string(kIterations) + "$" +
           base64Encode(salt) + "$" + base64Encode(key);
}

bool PasswordHasher::verify(const std::string& password, const std::string& storedHash) {
    if (password.empty() || storedHash.empty()) return false;

    std::string parts[4];
    if (!splitHash(storedHash, parts)) return false;
    if (parts[0] != "pbkdf2_sha256") return false;

    int iterations = 0;
    try {
        iterations = std::stoi(parts[1]);
    } catch (const std::exception&) {
        return false;
    }
    if (iterations <= 0 || iterations > 10000000) return false;

    std::vector<std::uint8_t> salt;
    std::vector<std::uint8_t> expected;
    try {
        salt = base64Decode(parts[2]);
        expected = base64Decode(parts[3]);
    } catch (const std::exception&) {
        return false;
    }
    if (salt.empty() || expected.empty()) return false;

    std::vector<std::uint8_t> actual;
    try {
        actual = pbkdf2(password, salt, iterations, expected.size());
    } catch (const std::exception&) {
        return false;
    }

    if (actual.size() != expected.size()) return false;

    // Constant-time comparison of the derived bytes.
    unsigned char difference = 0;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        difference |= static_cast<unsigned char>(actual[i] ^ expected[i]);
    }
    return difference == 0;
}

std::string PasswordHasher::randomHex(std::size_t bytes) {
    const std::vector<std::uint8_t> buffer = randomBytes(bytes);

    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(buffer.size() * 2);

    for (std::uint8_t byte : buffer) {
        out.push_back(digits[(byte >> 4) & 0x0F]);
        out.push_back(digits[byte & 0x0F]);
    }
    return out;
}



std::string PasswordHasher::base64Encode(const std::vector<std::uint8_t>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 2 < data.size()) {
        const std::uint32_t triple =
            (std::uint32_t(data[i]) << 16) |
            (std::uint32_t(data[i + 1]) << 8) |
             std::uint32_t(data[i + 2]);

        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 6) & 0x3F]);
        out.push_back(kBase64Alphabet[triple & 0x3F]);
        i += 3;
    }

    const std::size_t remaining = data.size() - i;

    if (remaining == 1) {
        const std::uint32_t triple = std::uint32_t(data[i]) << 16;
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back('=');
        out.push_back('=');
    } else if (remaining == 2) {
        const std::uint32_t triple =
            (std::uint32_t(data[i]) << 16) | (std::uint32_t(data[i + 1]) << 8);
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 6) & 0x3F]);
        out.push_back('=');
    }

    return out;
}

std::vector<std::uint8_t> PasswordHasher::base64Decode(const std::string& text) {
    std::vector<std::uint8_t> out;
    out.reserve((text.size() / 4) * 3);

    std::uint32_t buffer = 0;
    int bits = 0;

    for (char c : text) {
        if (c == '=' || c == '\n' || c == '\r' || c == ' ') continue;
        if (!isBase64Char(c)) {
            throw ValidationError("Invalid base64 data in stored password hash.");
        }

        buffer = (buffer << 6) | static_cast<std::uint32_t>(base64Value(c));
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((buffer >> bits) & 0xFF));
        }
    }

    return out;
}

} 
