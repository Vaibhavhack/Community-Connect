#include "CommunityPlatform.h"

bool CommunityPlatform::containsText(
    string text, string keyword) {

    if (keyword.empty()) {
        return true;
    }

    if (keyword.length() > text.length()) {
        return false;
    }

    for (int i = 0;
         i <= (int)text.length() - (int)keyword.length();
         i++) {

        bool matched = true;

        for (int j = 0; j < (int)keyword.length(); j++) {

            char a = text[i + j];
            char b = keyword[j];

            if (a >= 'A' && a <= 'Z') {
                a = a + ('a' - 'A');
            }

            if (b >= 'A' && b <= 'Z') {
                b = b + ('a' - 'A');
            }

            if (a != b) {
                matched = false;
                break;
            }
        }

        if (matched) {
            return true;
        }
    }

    return false;
}

void CommunityPlatform::searchResources() {
    string keyword;

    cout << "\nEnter keyword to search: ";
    getline(cin, keyword);

    bool found = false;

    cout << "\n===== SEARCH RESULTS =====\n";

    for (int i = 0; i < resourceCount; i++) {

        if (containsText(resources[i].title, keyword) ||
            containsText(resources[i].category, keyword) ||
            containsText(resources[i].description, keyword)) {

            showResource(i);
            found = true;
        }
    }

    if (!found) {
        cout << "No matching resources found.\n";
    }
}