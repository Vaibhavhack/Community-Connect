#include "CommunityPlatform.h"

int CommunityPlatform::findRequestById(int id) {
    for (int i = 0; i < requestCount; i++) {
        if (requests[i].id == id) {
            return i;
        }
    }

    return -1;
}

void CommunityPlatform::submitRequest() {
    if (!isLoggedIn()) {
        return;
    }

    if (requestCount >= MAX_REQUESTS) {
        cout << "Request limit reached.\n";
        return;
    }

    listResources();

    if (resourceCount == 0) {
        return;
    }

    cout << "\nEnter Resource ID: ";

    int resourceId;
    if (!(cin >> resourceId)) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid Resource ID.\n";
        return;
    }

    cin.ignore(10000, '\n');

    int resourceIndex = findResourceById(resourceId);

    if (resourceIndex == -1) {
        cout << "Resource not found.\n";
        return;
    }

    if (resources[resourceIndex].ownerId == loggedInUserId) {
        cout << "You cannot request your own resource.\n";
        return;
    }

    HelpRequest newRequest;

    newRequest.id = nextRequestId++;
    newRequest.requesterId = loggedInUserId;
    newRequest.resourceId = resourceId;
    newRequest.status = "Pending";

    cout << "Describe the help you need: ";
    getline(cin, newRequest.message);

    requests[requestCount] = newRequest;
    requestCount++;

    cout << "Help request submitted successfully!\n";
    cout << "Request ID: " << newRequest.id << "\n";
}

void CommunityPlatform::viewMyRequests() {
    if (!isLoggedIn()) {
        return;
    }

    bool found = false;

    cout << "\n===== MY REQUESTS =====\n";

    for (int i = 0; i < requestCount; i++) {

        if (requests[i].requesterId == loggedInUserId) {

            cout << "\nRequest ID: " << requests[i].id;
            cout << "\nResource ID: " << requests[i].resourceId;
            cout << "\nMessage: " << requests[i].message;
            cout << "\nStatus: " << requests[i].status << "\n";

            found = true;
        }
    }

    if (!found) {
        cout << "You have no requests.\n";
    }
}

void CommunityPlatform::manageRequests() {
    if (!isLoggedIn()) {
        return;
    }

    bool found = false;

    cout << "\n===== REQUESTS FOR YOUR RESOURCES =====\n";

    for (int i = 0; i < requestCount; i++) {

        int resourceIndex =
            findResourceById(requests[i].resourceId);

        if (resourceIndex != -1 &&
            resources[resourceIndex].ownerId == loggedInUserId) {

            cout << "\nRequest ID: " << requests[i].id;
            cout << "\nResource: "
                 << resources[resourceIndex].title;
            cout << "\nMessage: " << requests[i].message;
            cout << "\nStatus: " << requests[i].status << "\n";

            found = true;
        }
    }

    if (!found) {
        cout << "No requests for your resources.\n";
        return;
    }

    cout << "\nEnter Request ID to update (0 to cancel): ";

    int requestId;
    if (!(cin >> requestId)) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid Request ID.\n";
        return;
    }

    cin.ignore(10000, '\n');

    if (requestId == 0) {
        return;
    }

    int index = findRequestById(requestId);

    if (index == -1) {
        cout << "Request not found.\n";
        return;
    }

    int resourceIndex =
        findResourceById(requests[index].resourceId);

    if (resourceIndex == -1 ||
        resources[resourceIndex].ownerId != loggedInUserId) {

        cout << "You cannot update this request.\n";
        return;
    }

    cout << "\n1. Pending\n";
    cout << "2. Accepted\n";
    cout << "3. Rejected\n";
    cout << "4. Completed\n";
    cout << "Choose status: ";

    int choice;
    if (!(cin >> choice)) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid choice.\n";
        return;
    }

    cin.ignore(10000, '\n');

    switch (choice) {
        case 1:
            requests[index].status = "Pending";
            break;

        case 2:
            requests[index].status = "Accepted";
            break;

        case 3:
            requests[index].status = "Rejected";
            break;

        case 4:
            requests[index].status = "Completed";
            break;

        default:
            cout << "Invalid choice.\n";
            return;
    }

    cout << "Request status updated successfully.\n";
}