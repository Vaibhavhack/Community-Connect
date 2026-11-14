#include "CommunityPlatform.h"

void CommunityPlatform::run() {
    int choice;

    do {
        cout << "\n====================================\n";
        cout << " COMMUNITY RESOURCE AND HELP EXCHANGE\n";
        cout << "====================================\n";

        if (loggedInUserId == -1) {
            cout << "1. Register\n";
            cout << "2. Login\n";
        } else {
            cout << "3. View Profile\n";
            cout << "4. Add Resource\n";
            cout << "5. List Resources\n";
            cout << "6. Search Resources\n";
            cout << "7. Submit Help Request\n";
            cout << "8. View My Requests\n";
            cout << "9. Manage Requests\n";
            cout << "10. Logout\n";
        }

        cout << "0. Exit\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Please enter a valid number.\n";
            continue;
        }

        cin.ignore(10000, '\n');

        if (loggedInUserId == -1) {
            switch (choice) {
                case 1:
                    registerUser();
                    break;

                case 2:
                    login();
                    break;

                case 0:
                    cout << "Thank you for using the platform!\n";
                    break;

                default:
                    cout << "Invalid choice.\n";
            }
        } else {
            switch (choice) {
                case 3:
                    viewProfile();
                    break;

                case 4:
                    addResource();
                    break;

                case 5:
                    listResources();
                    break;

                case 6:
                    searchResources();
                    break;

                case 7:
                    submitRequest();
                    break;

                case 8:
                    viewMyRequests();
                    break;

                case 9:
                    manageRequests();
                    break;

                case 10:
                    logout();
                    break;

                case 0:
                    cout << "Thank you for using the platform!\n";
                    break;

                default:
                    cout << "Invalid choice.\n";
            }
        }

    } while (choice != 0);
}

int main() {
    CommunityPlatform app;

    app.run();

    return 0;
}