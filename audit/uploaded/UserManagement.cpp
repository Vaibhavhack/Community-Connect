#include "CommunityPlatform.h"

CommunityPlatform::CommunityPlatform() {
    userCount = 0;
    resourceCount = 0;
    requestCount = 0;

    nextUserId = 1;
    nextResourceId = 1;
    nextRequestId = 1;

    loggedInUserId = -1;
}

int CommunityPlatform::findUserById(int id) {
    for (int i = 0; i < userCount; i++) {
        if (users[i].id == id) {
            return i;
        }
    }

    return -1;
}

bool CommunityPlatform::isLoggedIn() {
    if (loggedInUserId == -1) {
        cout << "Please login first.\n";
        return false;
    }

    return true;
}

void CommunityPlatform::registerUser() {
    if (userCount >= MAX_USERS) {
        cout << "User limit reached.\n";
        return;
    }

    User newUser;

    cout << "\nEnter your name: ";
    getline(cin, newUser.name);

    cout << "Choose username: ";
    getline(cin, newUser.username);

    if (newUser.username.empty()) {
        cout << "Username cannot be empty.\n";
        return;
    }

    for (int i = 0; i < userCount; i++) {
        if (users[i].username == newUser.username) {
            cout << "Username already exists.\n";
            return;
        }
    }

    cout << "Choose password: ";
    getline(cin, newUser.password);

    cout << "Enter your skills: ";
    getline(cin, newUser.skills);

    cout << "Enter your interests: ";
    getline(cin, newUser.interests);

    newUser.id = nextUserId++;

    users[userCount] = newUser;
    userCount++;

    cout << "\nRegistration successful!\n";
    cout << "Your User ID is: " << newUser.id << "\n";
}

void CommunityPlatform::login() {
    if (loggedInUserId != -1) {
        cout << "Please logout before switching accounts.\n";
        return;
    }

    string username, password;

    cout << "\nEnter username: ";
    getline(cin, username);

    cout << "Enter password: ";
    getline(cin, password);

    for (int i = 0; i < userCount; i++) {
        if (users[i].username == username &&
            users[i].password == password) {

            loggedInUserId = users[i].id;

            cout << "Welcome, " << users[i].name << "!\n";
            return;
        }
    }

    cout << "Invalid username or password.\n";
}

void CommunityPlatform::viewProfile() {
    if (!isLoggedIn()) {
        return;
    }

    int index = findUserById(loggedInUserId);

    if (index == -1) {
        cout << "User not found.\n";
        return;
    }

    cout << "\n===== USER PROFILE =====\n";
    cout << "User ID: " << users[index].id << "\n";
    cout << "Name: " << users[index].name << "\n";
    cout << "Username: " << users[index].username << "\n";
    cout << "Skills: " << users[index].skills << "\n";
    cout << "Interests: " << users[index].interests << "\n";
}

void CommunityPlatform::logout() {
    if (loggedInUserId == -1) {
        cout << "No user is logged in.\n";
        return;
    }

    loggedInUserId = -1;

    cout << "Logout successful.\n";
}