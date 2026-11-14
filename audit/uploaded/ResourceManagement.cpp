#include "CommunityPlatform.h"

int CommunityPlatform::findResourceById(int id) {
    for (int i = 0; i < resourceCount; i++) {
        if (resources[i].id == id) {
            return i;
        }
    }

    return -1;
}

void CommunityPlatform::showResource(int index) {
    cout << "\nResource ID: " << resources[index].id;
    cout << "\nTitle: " << resources[index].title;
    cout << "\nCategory: " << resources[index].category;
    cout << "\nDescription: " << resources[index].description;

    int ownerIndex = findUserById(resources[index].ownerId);

    if (ownerIndex != -1) {
        cout << "\nShared by: " << users[ownerIndex].name;
    }

    cout << "\n";
}

void CommunityPlatform::addResource() {
    if (!isLoggedIn()) {
        return;
    }

    if (resourceCount >= MAX_RESOURCES) {
        cout << "Resource limit reached.\n";
        return;
    }

    Resource newResource;

    cout << "\nEnter resource title: ";
    getline(cin, newResource.title);

    if (newResource.title.empty()) {
        cout << "Title cannot be empty.\n";
        return;
    }

    cout << "Enter category: ";
    getline(cin, newResource.category);

    cout << "Enter description: ";
    getline(cin, newResource.description);

    newResource.id = nextResourceId++;
    newResource.ownerId = loggedInUserId;

    resources[resourceCount] = newResource;
    resourceCount++;

    cout << "Resource added successfully!\n";
    cout << "Resource ID: " << newResource.id << "\n";
}

void CommunityPlatform::listResources() {
    if (resourceCount == 0) {
        cout << "\nNo resources available.\n";
        return;
    }

    cout << "\n===== AVAILABLE RESOURCES =====\n";

    for (int i = 0; i < resourceCount; i++) {
        showResource(i);
    }
}
