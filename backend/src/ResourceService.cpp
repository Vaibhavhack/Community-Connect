// ResourceService.cpp
// -----------------------------------------------------------------------------
// MODULE 2 - Resource Management.
// -----------------------------------------------------------------------------

#include "ResourceService.h"

#include <algorithm>

#include "Errors.h"

namespace cc {

namespace {

bool hasControlCharacters(const std::string& value) {
    for (char c : value) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 && c != '\n' && c != '\t') return true;
        if (u == 0x7F) return true;
    }
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ResourceService::ResourceService(FileManager& files) : files_(files) {
    resources_ = files_.loadAll<Resource>(kFileName);

    int highestId = 0;
    for (std::size_t position = 0; position < resources_.size(); ++position) {
        const Resource& resource = resources_[position];
        highestId = std::max(highestId, resource.getId());
        idIndex_[resource.getId()] = position;
    }
    nextResourceId_ = highestId + 1;
}

void ResourceService::persist() {
    files_.saveAll(kFileName, resources_);
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

std::string ResourceService::validateTitle(const std::string& value) {
    const std::string title = trim(value);

    if (title.empty()) {
        throw ValidationError("Resource title is required.");
    }
    if (title.size() < 3) {
        throw ValidationError("Resource title must be at least 3 characters long.");
    }
    if (title.size() > kMaxTitleLength) {
        throw ValidationError("Resource title must be at most " +
                              std::to_string(kMaxTitleLength) + " characters long.");
    }
    if (hasControlCharacters(title)) {
        throw ValidationError("Resource title contains invalid characters.");
    }

    return title;
}

std::string ResourceService::validateDescription(const std::string& value) {
    // Multi-line descriptions are allowed on purpose (notes, syllabus lists).
    const std::string description = trim(value);

    if (description.empty()) {
        throw ValidationError("Resource description is required.");
    }
    if (description.size() < 10) {
        throw ValidationError(
            "Please describe the resource in at least 10 characters so others can judge it.");
    }
    if (description.size() > kMaxDescriptionLength) {
        throw ValidationError("Resource description must be at most " +
                              std::to_string(kMaxDescriptionLength) + " characters long.");
    }
    if (hasControlCharacters(description)) {
        throw ValidationError("Resource description contains invalid characters.");
    }

    return description;
}

// ---------------------------------------------------------------------------
// Lookups
// ---------------------------------------------------------------------------

bool ResourceService::exists(int resourceId) const {
    return resourceId > 0 && idIndex_.find(resourceId) != idIndex_.end();
}

Resource ResourceService::requireResource(int resourceId) const {
    auto it = idIndex_.find(resourceId);
    if (it == idIndex_.end()) {
        throw NotFoundError("Resource not found.");
    }
    return resources_[it->second];
}

Resource& ResourceService::requireResourceMutable(int resourceId) {
    auto it = idIndex_.find(resourceId);
    if (it == idIndex_.end()) {
        throw NotFoundError("Resource not found.");
    }
    return resources_[it->second];
}

void ResourceService::requireOwner(const Resource& resource, int userId) const {
    if (resource.ownerId != userId) {
        throw ForbiddenError("You can only manage resources that you added.");
    }
}

int ResourceService::ownerOf(int resourceId) const {
    return requireResource(resourceId).ownerId;
}

std::string ResourceService::titleOf(int resourceId) const {
    return requireResource(resourceId).title;
}

std::string ResourceService::categoryOf(int resourceId) const {
    return requireResource(resourceId).category;
}

// ---------------------------------------------------------------------------
// Create / list
// ---------------------------------------------------------------------------

int ResourceService::createResource(int ownerId,
                                    const std::string& title,
                                    const std::string& description,
                                    const std::string& category) {
    if (ownerId <= 0) {
        throw AuthError("Please log in to add a resource.");
    }

    Resource resource;

    // Validate everything BEFORE assigning, so a rejected field cannot leave a
    // half-built record behind.
    resource.title        = validateTitle(title);
    resource.description  = validateDescription(description);
    resource.category     = categories::validate(category); // one of the nine
    resource.ownerId      = ownerId;
    resource.availability = Availability::Available;

    resource.setId(nextResourceId_++);
    resource.setCreatedAt(nowIso8601());
    resource.dateAdded = resource.getCreatedAt().substr(0, 10);

    resources_.push_back(resource);
    idIndex_[resource.getId()] = resources_.size() - 1;

    persist();
    return resource.getId();
}

std::vector<Resource> ResourceService::listAll() const {
    std::vector<Resource> result = resources_;

    // Newest first: ids increase with creation, so this is a stable, cheap
    // ordering that needs no date parsing.
    std::sort(result.begin(), result.end(),
              [](const Resource& a, const Resource& b) {
                  return a.getId() > b.getId();
              });

    return result;
}

std::vector<Resource> ResourceService::listFiltered(const std::string& category,
                                                    bool onlyAvailable) const {
    // "All" / empty means no category filter. The category is matched
    // case-insensitively so the query string does not have to be exact.
    const std::string wanted = toLowerCopy(trim(category));
    const bool filterCategory = !wanted.empty() && wanted != "all";

    std::vector<Resource> result;

    for (const Resource& resource : resources_) {
        if (filterCategory && toLowerCopy(resource.category) != wanted) continue;
        if (onlyAvailable && resource.availability != Availability::Available) continue;
        result.push_back(resource);
    }

    std::sort(result.begin(), result.end(),
              [](const Resource& a, const Resource& b) {
                  return a.getId() > b.getId();
              });

    return result;
}

std::vector<Resource> ResourceService::listByOwner(int ownerId) const {
    std::vector<Resource> result;

    for (const Resource& resource : resources_) {
        if (resource.ownerId == ownerId) result.push_back(resource);
    }

    std::sort(result.begin(), result.end(),
              [](const Resource& a, const Resource& b) {
                  return a.getId() > b.getId();
              });

    return result;
}

std::size_t ResourceService::countByOwner(int ownerId) const {
    std::size_t total = 0;
    for (const Resource& resource : resources_) {
        if (resource.ownerId == ownerId) ++total;
    }
    return total;
}

// ---------------------------------------------------------------------------
// Edit / delete / availability
// ---------------------------------------------------------------------------

Resource ResourceService::updateResource(int resourceId,
                                         int userId,
                                         const std::string& title,
                                         const std::string& description,
                                         const std::string& category) {
    Resource& resource = requireResourceMutable(resourceId);

    // The ownership rule from the original code, applied to editing.
    requireOwner(resource, userId);

    // Validate everything first, then commit - same two-phase rule as
    // UserService::updateProfile().
    std::string newTitle       = resource.title;
    std::string newDescription = resource.description;
    std::string newCategory    = resource.category;

    if (!trim(title).empty())       newTitle = validateTitle(title);
    if (!trim(description).empty()) newDescription = validateDescription(description);
    if (!trim(category).empty())    newCategory = categories::validate(category);

    resource.title       = newTitle;
    resource.description = newDescription;
    resource.category    = newCategory;

    persist();
    return resource;
}

void ResourceService::deleteResource(int resourceId,
                                     int userId,
                                     const std::function<bool(int)>& hasLiveRequests) {
    auto it = idIndex_.find(resourceId);
    if (it == idIndex_.end()) {
        throw NotFoundError("Resource not found.");
    }

    const std::size_t position = it->second;
    requireOwner(resources_[position], userId);

    // Referential integrity: refuse the delete while requests still point here.
    // The caller gets an actionable message instead of silently broken data.
    if (hasLiveRequests && hasLiveRequests(resourceId)) {
        throw ConflictError(
            "This resource still has open help requests. Mark it unavailable, or "
            "cancel/complete those requests first, then delete it.");
    }

    resources_.erase(resources_.begin() + static_cast<std::ptrdiff_t>(position));
    idIndex_.erase(it);

    // Erasing shifts every position after it, so the index must be rebuilt.
    // This is O(n) but only happens on a delete, never on a read.
    for (std::size_t index = position; index < resources_.size(); ++index) {
        idIndex_[resources_[index].getId()] = index;
    }

    persist();
}

Resource ResourceService::setAvailability(int resourceId, int userId,
                                          Availability availability) {
    Resource& resource = requireResourceMutable(resourceId);
    requireOwner(resource, userId);

    resource.availability = availability;
    persist();

    return resource;
}

} // namespace cc
