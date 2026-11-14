// ResourceService.h
// -----------------------------------------------------------------------------
// Purpose : MODULE 2 - Resource Management.
//
// Compared with the original ResourceManagement.cpp:
//   - add / list exist (kept), plus edit, delete and availability (missing)
//   - the category is one of the nine educational categories, not free text
//   - every operation re-checks ownership against the session user id, using
//     the same "verify at the point of mutation" rule your manageRequests()
//     already used - extended to edit, delete and availability
//   - a resource that still has live requests cannot be hard-deleted, because
//     that would leave requests pointing at a resource id that no longer exists
//     (the dangling-reference bug in the original code)
// -----------------------------------------------------------------------------

#ifndef CC_RESOURCE_SERVICE_H
#define CC_RESOURCE_SERVICE_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "FileManager.h"
#include "Models.h"

namespace cc {

class ResourceService {
public:
    static constexpr const char* kFileName = "resources.txt";

    static constexpr std::size_t kMaxTitleLength = 100;
    static constexpr std::size_t kMaxDescriptionLength = 1000;

    explicit ResourceService(FileManager& files);

    // ---- create / read ------------------------------------------------------

    // Returns the new resource id. Category must be one of the nine.
    int createResource(int ownerId,
                       const std::string& title,
                       const std::string& description,
                       const std::string& category);

    Resource requireResource(int resourceId) const;
    bool exists(int resourceId) const;

    // All resources, newest first.
    std::vector<Resource> listAll() const;

    // Optional filters; pass an empty string / "All" to ignore one.
    std::vector<Resource> listFiltered(const std::string& category,
                                       bool onlyAvailable) const;

    std::vector<Resource> listByOwner(int ownerId) const;

    std::size_t count() const { return resources_.size(); }
    std::size_t countByOwner(int ownerId) const;

    // ---- edit / delete (owner only) ----------------------------------------

    // Empty arguments leave the field unchanged. Throws ForbiddenError if the
    // caller does not own the resource.
    Resource updateResource(int resourceId,
                            int userId,
                            const std::string& title,
                            const std::string& description,
                            const std::string& category);

    // Deletes an owned resource. `hasLiveRequests` is supplied by the request
    // module; if it returns true the delete is refused, so no request is left
    // pointing at a missing resource.
    void deleteResource(int resourceId,
                        int userId,
                        const std::function<bool(int)>& hasLiveRequests);

    Resource setAvailability(int resourceId, int userId, Availability availability);

    // ---- helpers used by the request module --------------------------------

    int ownerOf(int resourceId) const;
    std::string titleOf(int resourceId) const;
    std::string categoryOf(int resourceId) const;

private:
    // Validation (static so it cannot accidentally touch member state).
    static std::string validateTitle(const std::string& value);
    static std::string validateDescription(const std::string& value);

    Resource& requireResourceMutable(int resourceId);
    void requireOwner(const Resource& resource, int userId) const;

    void persist();

    FileManager& files_;
    std::vector<Resource> resources_;
    std::unordered_map<int, std::size_t> idIndex_;  // resource id -> position
    int nextResourceId_ = 1;
};

} // namespace cc

#endif // CC_RESOURCE_SERVICE_H
