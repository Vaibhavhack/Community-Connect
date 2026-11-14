// Entity.h
// -----------------------------------------------------------------------------
// Purpose : Abstract base class for every record saved in the .txt files.
//
// Every stored record has two things in common: an integer ID and a creation
// timestamp. Putting them in a base class (INHERITANCE) avoids repeating them
// four times, and the three pure virtual functions force each subclass to say
// how it is converted to and from a text-file line (POLYMORPHISM).
//
// FileManager is a template, so it can load and save any Entity subclass with
// exactly the same code:
//
//     vector<User>         users         = fm.loadAll<User>("users.txt");
//     vector<HelpRequest>  requests      = fm.loadAll<HelpRequest>("requests.txt");
// -----------------------------------------------------------------------------

#ifndef CC_ENTITY_H
#define CC_ENTITY_H

#include <string>
#include <vector>

#include "Errors.h"

namespace cc {

// One stored record is a list of text fields. This is the format every
// subclass reads from and writes to.
using Record = std::vector<std::string>;

class Entity {
public:
    virtual ~Entity() = default; // virtual because we delete through Entity*

    int getId() const { return id_; }
    void setId(int value) { id_ = value; }

    const std::string& getCreatedAt() const { return createdAt_; }
    void setCreatedAt(const std::string& value) { createdAt_ = value; }

    // ---- pure virtual: each record type knows its own storage layout --------
    virtual std::string toRecord() const = 0;   // -> one escaped line
    virtual void fromRecord(const Record& fields) = 0; // <- one unescaped line
    virtual std::string describe() const = 0;   // human readable summary

    // ---- small helpers used by the subclasses -------------------------------
    // Aborts loading of this one line if the file is short/corrupt. The
    // FileManager catches this and skips just the bad line instead of killing
    // the whole application (requirement: "handle corrupted records").
    static void requireFields(const Record& fields, std::size_t minimum,
                              const char* recordName) {
        if (fields.size() < minimum) {
            throw ValidationError(std::string("Corrupt ") + recordName +
                                  " record: expected at least " +
                                  std::to_string(minimum) + " fields, found " +
                                  std::to_string(fields.size()));
        }
    }

    // Safe text -> int conversion with a clear error message.
    static int parseInt(const std::string& value, const char* fieldName) {
        if (value.empty()) {
            throw ValidationError(std::string("Missing number for ") + fieldName);
        }
        try {
            std::size_t used = 0;
            int result = std::stoi(value, &used);
            if (used != value.size()) {
                throw ValidationError(std::string("Corrupt number for ") + fieldName);
            }
            return result;
        } catch (const ValidationError&) {
            throw;
        } catch (const std::exception&) {
            throw ValidationError(std::string("Corrupt number for ") + fieldName);
        }
    }

    static bool parseBool(const std::string& value) {
        return value == "1" || value == "true" || value == "yes";
    }

protected:
    int id_ = 0;
    std::string createdAt_;
};

} // namespace cc

#endif // CC_ENTITY_H
