
#include "ConsoleDemo.h"

#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "Errors.h"

namespace cc {

namespace {

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

bool readInt(const std::string& prompt, int& value) {
    std::cout << prompt;
    if (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return false;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

bool readPriority(const std::string& prompt, Priority& value) {
    std::cout << prompt << "\n"
              << "  1. High\n"
              << "  2. Medium\n"
              << "  3. Low\n";

    int choice = 0;
    if (!readInt("Choose priority: ", choice)) return false;

    switch (choice) {
        case 1: value = Priority::High;   return true;
        case 2: value = Priority::Medium; return true;
        case 3: value = Priority::Low;    return true;
        default: return false;
    }
}
std::string chooseCategory() {
    const std::vector<std::string>& all = categories::all();

    std::cout << "\nCategories:\n";
    for (std::size_t i = 0; i < all.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << all[i] << "\n";
    }

    int choice = 0;
    if (!readInt("Choose category number (0 to cancel): ", choice)) return "";
    if (choice <= 0 || choice > static_cast<int>(all.size())) return "";

    return all[static_cast<std::size_t>(choice - 1)];
}

void printProfile(const UserView& profile, bool isOwnProfile) {
    std::cout << "\n===== " << (isOwnProfile ? "MY PROFILE" : "PROFILE") << " =====\n";
    std::cout << "User ID   : " << profile.id << "\n";
    std::cout << "Name      : " << profile.name << "\n";
    std::cout << "Username  : " << profile.username << "\n";
    std::cout << "City      : " << profile.city;
    if (!profile.locality.empty()) std::cout << " (" << profile.locality << ")";
    std::cout << "\n";
    std::cout << "Skills    : " << (profile.skills.empty() ? "-" : profile.skills) << "\n";
    std::cout << "Interests : " << (profile.interests.empty() ? "-" : profile.interests) << "\n";

    if (isOwnProfile) {
        std::cout << "Email     : " << profile.email << "\n";
        std::cout << "Contact   : "
                  << (profile.contactPhone.empty() ? "-" : profile.contactPhone) << "\n";
    } else {
        std::cout << "(email and contact number are private)\n";
    }
    std::cout << "Member since: " << profile.createdAt << "\n";
}

void printResourceBrief(const Resource& resource, const UserService& users) {
    std::cout << "  #" << resource.getId() << " | " << resource.title
              << " | " << resource.category
              << " | " << toString(resource.availability);

   
    try {
        std::cout << " | by " << users.getPublicProfile(resource.ownerId).name;
    } catch (const AppError&) {
        std::cout << " | by (unknown user)";
    }
    std::cout << "\n";
}

void printResourceDetail(const Resource& resource, const UserService& users) {
    std::cout << "\n===== RESOURCE #" << resource.getId() << " =====\n";
    std::cout << "Title       : " << resource.title << "\n";
    std::cout << "Category    : " << resource.category << "\n";
    std::cout << "Availability: " << toString(resource.availability) << "\n";
    std::cout << "Added on    : " << resource.dateAdded << "\n";

    try {
        const UserView owner = users.getPublicProfile(resource.ownerId);
        std::cout << "Shared by   : " << owner.name << " (" << owner.city << ")\n";
    } catch (const AppError&) {
        std::cout << "Shared by   : (account no longer available)\n";
    }

    std::cout << "Description :\n" << resource.description << "\n";
}

void printRequest(const HelpRequest& request, const UserService& users,
                  const ResourceService& resources) {
    std::cout << "  #" << request.getId()
              << " | " << toString(request.priority)
              << " | " << toString(request.status)
              << " | " << request.title << "\n";

    if (request.resourceId > 0) {
        try {
            std::cout << "       resource: " << resources.titleOf(request.resourceId) << "\n";
        } catch (const AppError&) {
            std::cout << "       resource: #" << request.resourceId << " (removed)\n";
        }
    } else {
        std::cout << "       general help request (no specific resource)\n";
    }

    try {
        std::cout << "       from: " << users.getPublicProfile(request.requesterId).name << "\n";
    } catch (const AppError&) {
        std::cout << "       from: (unknown user)\n";
    }

    if (!request.requiredSkills.empty()) {
        std::cout << "       skills needed: " << request.requiredSkills << "\n";
    }
    std::cout << "       created: " << request.getCreatedAt() << "\n";
}

void printNotification(const Notification& item) {
    std::cout << (item.isRead ? "  [read]   " : "  [UNREAD] ")
              << "#" << item.getId() << " " << item.message << "\n";
}
void showContactDetails(RequestService& requests, int requestId, int userId) {
    const ContactDetails details = requests.contactDetailsFor(requestId, userId);

    std::cout << "\n----- CONTACT DETAILS -----\n";
    std::cout << "Name  : " << details.name << "\n";
    std::cout << "Email : " << details.email << "\n";
    std::cout << "Phone : " << (details.phone.empty() ? "-" : details.phone) << "\n";
    std::cout << "City  : " << details.city;
    if (!details.locality.empty()) std::cout << " (" << details.locality << ")";
    std::cout << "\n";
}

void myRequestsMenu(RequestService& requests, UserService& users,
                    ResourceService& resources, int userId) {
    const std::vector<HelpRequest> mine = requests.listMine(userId);

    std::cout << "\n===== MY HELP REQUESTS =====\n";
    if (mine.empty()) {
        std::cout << "You have not created any requests yet.\n";
        return;
    }

    for (const HelpRequest& request : mine) {
        printRequest(request, users, resources);
    }

    int requestId = 0;
    if (!readInt("\nEnter a request ID to manage (0 to go back): ", requestId)) return;
    if (requestId == 0) return;

    const HelpRequest request = requests.requireRequest(requestId);
    if (request.requesterId != userId) {
        std::cout << "That request does not belong to you.\n";
        return;
    }

    std::cout << "\n1. View full details\n"
              << "2. View contact details (only after acceptance)\n"
              << "3. Edit (pending only)\n"
              << "4. Cancel (pending only)\n"
              << "5. Mark completed (accepted only)\n";

    int choice = 0;
    if (!readInt("Choose: ", choice)) return;

    switch (choice) {
        case 1:
            printRequest(request, users, resources);
            std::cout << "  description: " << request.description << "\n";
            break;

        case 2:
            showContactDetails(requests, requestId, userId);
            break;

        case 3: {
            std::cout << "(press Enter to keep a field unchanged)\n";
            const std::string title    = readLine("New title: ");
            const std::string desc     = readLine("New description: ");
            const std::string skills   = readLine("New required skills: ");
            const std::string category = readLine("New category (leave empty to keep): ");

            Priority priority = request.priority;
            if (!readPriority("Change priority?", priority)) priority = request.priority;

            requests.updateRequest(requestId, userId, title, desc, skills, category, priority);
            std::cout << "Request updated.\n";
            break;
        }

        case 4:
            requests.cancelRequest(requestId, userId);
            std::cout << "Request cancelled.\n";
            break;

        case 5:
            requests.completeRequest(requestId, userId);
            std::cout << "Exchange marked completed.\n";
            break;

        default:
            std::cout << "Invalid choice.\n";
    }
}

void incomingRequestsMenu(RequestService& requests, UserService& users,
                          ResourceService& resources, int userId) {
    const std::vector<HelpRequest> incoming = requests.listIncoming(userId);

    std::cout << "\n===== REQUESTS FOR YOUR RESOURCES =====\n";
    if (incoming.empty()) {
        std::cout << "Nobody has requested your resources yet.\n";
        return;
    }


    for (const HelpRequest& request : incoming) {
        printRequest(request, users, resources);
    }

    int requestId = 0;
    if (!readInt("\nEnter a request ID to act on (0 to go back): ", requestId)) return;
    if (requestId == 0) return;

    std::cout << "\n1. Accept\n"
              << "2. Reject\n"
              << "3. View contact details (accepted only)\n"
              << "4. Mark completed (accepted only)\n";

    int choice = 0;
    if (!readInt("Choose: ", choice)) return;

    switch (choice) {
        case 1:
            requests.acceptRequest(requestId, userId);
            std::cout << "Request accepted. The requester has been notified and can now "
                         "see your contact details.\n";
            break;

        case 2:
            requests.rejectRequest(requestId, userId);
            std::cout << "Request rejected. The requester has been notified.\n";
            break;

        case 3:
            showContactDetails(requests, requestId, userId);
            break;

        case 4:
            requests.completeRequest(requestId, userId);
            std::cout << "Exchange marked completed.\n";
            break;

        default:
            std::cout << "Invalid choice.\n";
    }
}


void priorityQueueMenu(RequestService& requests, UserService& users,
                       ResourceService& resources) {
    std::cout << "\n===== PENDING REQUESTS BY PRIORITY =====\n";

    if (requests.pendingCount() == 0) {
        std::cout << "The priority queue is empty - no pending requests.\n";
        return;
    }
    const std::vector<HelpRequest> ordered = requests.listPendingByPriority();

    std::cout << "Order: High > Medium > Low, and older requests first within a level.\n\n";

    std::size_t rank = 1;
    for (const HelpRequest& request : ordered) {
        std::cout << "[" << rank++ << "] ";
        printRequest(request, users, resources);
    }

    const HelpRequest highest = requests.peekHighestPriority();
    std::cout << "\nHighest priority right now: #" << highest.getId()
              << " \"" << highest.title << "\" (" << toString(highest.priority) << ")\n";

    int choice = 0;
    if (!readInt("\n1. Extract the highest priority request\n0. Back\nChoose: ", choice)) return;
    if (choice != 1) return;

    HelpRequest taken;
    if (!requests.takeHighestPriority(taken)) {
        std::cout << "Nothing to extract.\n";
        return;
    }

    std::cout << "Extracted: #" << taken.getId() << " " << taken.title << "\n";
    std::cout << "(Extracting only removes it from the queue for this view; the request "
                 "status is unchanged.)\n";
}

void notificationsMenu(NotificationService& notifications, int userId) {
    const std::vector<Notification> feed = notifications.listFor(userId);

    std::cout << "\n===== NOTIFICATIONS =====\n";
    std::cout << "Unread: " << notifications.unreadCount(userId) << "\n\n";

    if (feed.empty()) {
        std::cout << "No notifications yet.\n";
        return;
    }

    for (const Notification& item : feed) {
        printNotification(item);
    }

    int choice = 0;
    if (!readInt("\n1. Mark one as read\n2. Mark all as read\n0. Back\nChoose: ", choice)) return;

    if (choice == 1) {
        int notificationId = 0;
        if (!readInt("Notification ID: ", notificationId)) return;
        notifications.markRead(userId, notificationId);
        std::cout << "Marked as read.\n";
    } else if (choice == 2) {
        const std::size_t changed = notifications.markAllRead(userId);
        std::cout << changed << " notification(s) marked as read.\n";
    }
}

void resourcesMenu(ResourceService& resources, RequestService& requests,
                   UserService& users, int userId) {
    while (true) {
        std::cout << "\n===== RESOURCES =====\n"
                  << "1. Browse all\n"
                  << "2. Filter by category\n"
                  << "3. Show only available\n"
                  << "4. My resources\n"
                  << "0. Back\n";

        int choice = 0;
        if (!readInt("Choose: ", choice)) return;
        if (choice == 0) return;

        try {
            if (choice == 1 || choice == 2 || choice == 3) {
                std::string category;
                bool onlyAvailable = (choice == 3);

                if (choice == 2) {
                    category = chooseCategory();
                    if (category.empty()) continue;
                }

                const std::vector<Resource> list =
                    resources.listFiltered(category, onlyAvailable);

                std::cout << "\n" << list.size() << " resource(s):\n";
                for (const Resource& resource : list) {
                    printResourceBrief(resource, users);
                }

                int detailId = 0;
                if (!readInt("\nResource ID for full details (0 to back): ", detailId)) continue;
                if (detailId == 0) continue;

                printResourceDetail(resources.requireResource(detailId), users);

                if (userId != 0 &&
                    resources.requireResource(detailId).ownerId != userId) {
                    int requestIt = 0;
                    if (!readInt("1. Request this resource\n0. Back\nChoose: ", requestIt)) continue;
                    if (requestIt == 1) {
                        const std::string title = readLine("Request title: ");
                        const std::string desc  = readLine("Describe the help you need: ");
                        const std::string skills = readLine("Skills involved (optional): ");

                        Priority priority = Priority::Medium;
                        if (!readPriority("How urgent is it?", priority)) continue;

                        const int id = requests.createRequest(userId, detailId, title, desc,
                                                              skills, "", priority);
                        std::cout << "Request #" << id << " created and sent to the owner.\n";
                    }
                }

            } else if (choice == 4) {
                const std::vector<Resource> mine = resources.listByOwner(userId);

                std::cout << "\n" << mine.size() << " of your resource(s):\n";
                if (mine.empty()) continue;

                for (const Resource& resource : mine) {
                    printResourceBrief(resource, users);
                }

                int manageId = 0;
                if (!readInt("\nResource ID to manage (0 to back): ", manageId)) continue;
                if (manageId == 0) continue;

                std::cout << "\n1. Edit\n2. Set availability\n3. Delete\n";
                int action = 0;
                if (!readInt("Choose: ", action)) continue;

                if (action == 1) {
                    std::cout << "(press Enter to keep a field unchanged)\n";
                    const std::string title = readLine("New title: ");
                    const std::string desc  = readLine("New description: ");
                    std::string category;
                    int changeCategory = 0;
                    readInt("Change category? 1 = yes: ", changeCategory);
                    if (changeCategory == 1) category = chooseCategory();

                    resources.updateResource(manageId, userId, title, desc, category);
                    std::cout << "Resource updated.\n";

                } else if (action == 2) {
                    std::cout << "\n1. Available\n2. Reserved\n3. Unavailable\n";
                    int status = 0;
                    if (!readInt("Choose: ", status)) continue;

                    Availability availability = Availability::Available;
                    if (status == 2) availability = Availability::Reserved;
                    if (status == 3) availability = Availability::Unavailable;

                    resources.setAvailability(manageId, userId, availability);
                    std::cout << "Availability set to " << toString(availability) << ".\n";

                } else if (action == 3) {
                   
                    resources.deleteResource(manageId, userId,
                                             [&requests](int id) {
                                                 return requests.hasLiveRequests(id);
                                             });
                    std::cout << "Resource deleted.\n";
                }
            }
        } catch (const AppError& error) {
            std::cout << "\n[!] " << error.what() << "\n";
        }
    }
}

void helpRequestMenu(RequestService& requests, ResourceService& resources,
                     UserService& users, int userId) {
    while (true) {
        std::cout << "\n===== HELP REQUESTS =====\n"
                  << "1. Create a request for one of my resources' peers\n"
                  << "2. Create a general help request\n"
                  << "3. My requests\n"
                  << "4. Requests for my resources\n"
                  << "0. Back\n";

        int choice = 0;
        if (!readInt("Choose: ", choice)) return;
        if (choice == 0) return;

        try {
            if (choice == 1 || choice == 2) {
                int resourceId = 0;

                if (choice == 1) {
                    const std::vector<Resource> all = resources.listAll();
                    if (all.empty()) {
                        std::cout << "There are no resources to request yet.\n";
                        continue;
                    }
                    for (const Resource& resource : all) {
                        printResourceBrief(resource, users);
                    }
                    if (!readInt("Resource ID: ", resourceId)) continue;
                }

                const std::string title  = readLine("Request title: ");
                const std::string desc   = readLine("Describe the help you need: ");
                const std::string skills = readLine("Required skills (optional): ");

                std::string category;
                if (choice == 2) {
                    category = chooseCategory();
                    if (category.empty()) continue;
                }

                Priority priority = Priority::Medium;
                if (!readPriority("Priority?", priority)) continue;

                const int id = requests.createRequest(userId, resourceId, title, desc,
                                                      skills, category, priority);
                std::cout << "\nRequest #" << id << " created.\n";

            } else if (choice == 3) {
                myRequestsMenu(requests, users, resources, userId);
            } else if (choice == 4) {
                incomingRequestsMenu(requests, users, resources, userId);
            }
        } catch (const AppError& error) {
            std::cout << "\n[!] " << error.what() << "\n";
        }
    }
}

} 

void searchAndDashboardMenu(SearchService& search,
                             int userId) {
    while (true) {
        std::cout << "\n===== SEARCH & DASHBOARD =====\n"
                  << "1. Search resources\n"
                  << "2. Search users by skills/interests\n"
                  << "3. Match users to a help request\n"
                  << "4. View dashboard summary\n"
                  << "0. Back\n";

        int choice = 0;
        if (!readInt("Choose: ", choice)) return;
        if (choice == 0) return;

        try {
            if (choice == 1) {
                const std::string query = readLine("Keyword (leave empty for all): ");
                const std::string category = readLine("Category (leave empty for all): ");
                const std::vector<RankedResource> results =
                    search.searchResources(query, category, false, 20);

                std::cout << "\n" << results.size() << " ranked resource(s):\n";
                for (const RankedResource& item : results) {
                    std::cout << "  #" << item.resource.getId()
                              << " | score=" << item.score
                              << " | " << item.resource.title
                              << " | " << item.resource.category << "\n";
                }
            } else if (choice == 2) {
                const std::string skills = readLine("Skills to match (comma separated): ");
                const std::string interests = readLine("Interests to match (optional): ");
                const std::string city = readLine("City filter (optional): ");
                const std::vector<UserMatch> results =
                    search.searchUsers(skills, interests, city, 20);

                std::cout << "\n" << results.size() << " matching user(s):\n";
                for (const UserMatch& item : results) {
                    std::cout << "  #" << item.user.id
                              << " | score=" << item.score
                              << " | " << item.user.name
                              << " | skills: " << item.user.skills << "\n";
                }
            } else if (choice == 3) {
                int requestId = 0;
                if (!readInt("Request ID: ", requestId)) continue;
                const std::vector<UserMatch> results = search.matchUsersForRequest(requestId, 20);

                std::cout << "\n" << results.size() << " suggested helper(s):\n";
                for (const UserMatch& item : results) {
                    std::cout << "  #" << item.user.id
                              << " | score=" << item.score
                              << " | " << item.user.name
                              << " | matched skills: " << joinList(item.matchedSkills) << "\n";
                }
            } else if (choice == 4) {
                const DashboardStats stats = search.dashboardFor(userId);
                std::cout << "\n===== DASHBOARD =====\n"
                          << "Resources offered   : " << stats.resourcesOffered << "\n"
                          << "Active help requests: " << stats.activeHelpRequests << "\n"
                          << "Pending requests    : " << stats.pendingRequests << "\n"
                          << "Accepted requests   : " << stats.acceptedRequests << "\n"
                          << "Completed exchanges : " << stats.completedExchanges << "\n"
                          << "Incoming requests   : " << stats.incomingRequests << "\n"
                          << "Unread notifications: " << stats.unreadNotifications << "\n";

                if (stats.hasHighestPriorityRequest) {
                    std::cout << "Most urgent pending  : #" << stats.highestPriorityRequest.getId()
                              << " (" << toString(stats.highestPriorityRequest.priority)
                              << ") " << stats.highestPriorityRequest.title << "\n";
                } else {
                    std::cout << "Most urgent pending  : none\n";
                }
            } else {
                std::cout << "Invalid choice.\n";
            }
        } catch (const AppError& error) {
            std::cout << "\n[!] " << error.what() << "\n";
        }
    }
}

int runConsoleDemo(FileManager& files,
                   SessionManager& sessions,
                   UserService& users,
                   ResourceService& resources,
                   RequestService& requests,
                   NotificationService& notifications,
                   SearchService& search) {
    (void)files;

    std::cout << "\n==================================================\n";
    std::cout << " COMMUNITY RESOURCE AND HELP EXCHANGE (console)\n";
    std::cout << " Community Connect - Stages 2, 3 and 4\n";
    std::cout << "==================================================\n";

    std::string token;
    int currentUserId = 0;

    while (true) {
        
        if (currentUserId != 0) {
            const std::size_t unread = notifications.unreadCount(currentUserId);
            if (unread > 0) {
                std::cout << "\n*** " << unread << " unread notification(s) ***\n";
            }
        }

        std::cout << "\n----------------------------------\n";
        if (currentUserId == 0) {
            std::cout << "1. Register\n"
                      << "2. Login\n"
                      << "3. Browse resources\n";
        } else {
            try {
                const UserView me = users.getOwnProfile(currentUserId);
                std::cout << "Logged in as " << me.name << " (@" << me.username << ")\n";
            } catch (const AppError&) {
                currentUserId = 0;
                token.clear();
                continue;
            }

            std::cout << "4. View my profile\n"
                      << "5. Edit my profile\n"
                      << "6. Change password\n"
                      << "7. Resources (browse / add / manage)\n"
                      << "8. Help requests\n"
                      << "9. Pending requests by priority\n"
                      << "10. Notifications\n"
                      << "11. Search / Matching / Dashboard\n"
                      << "12. Logout\n";
        }
        std::cout << "0. Exit\n";

        int choice = -1;
        if (!readInt("Enter choice: ", choice)) {
            std::cout << "Please enter a valid number.\n";
            continue;
        }

        try {
            if (currentUserId == 0) {
                if (choice == 0) break;

                if (choice == 1) {
                    const std::string name      = readLine("Enter your name: ");
                    const std::string username  = readLine("Choose username: ");
                    const std::string email     = readLine("Enter your email: ");
                    const std::string password  = readLine("Choose password (min 6 chars): ");
                    const std::string city      = readLine("Enter your city: ");
                    const std::string locality  = readLine("Enter your locality (optional): ");
                    const std::string skills    = readLine("Skills (comma separated): ");
                    const std::string interests = readLine("Interests (comma separated): ");
                    const std::string phone     = readLine("Contact number (optional, private): ");

                    const int newId = users.registerUser(name, username, email, password,
                                                         city, locality, skills, interests, phone);

                    std::cout << "\nRegistration successful! Your User ID is " << newId << ".\n"
                              << "Please log in to continue.\n";

                } else if (choice == 2) {
                    const std::string identifier = readLine("Enter username or email: ");
                    const std::string password   = readLine("Enter password: ");

                    token = users.login(identifier, password);
                    currentUserId = sessions.requireUserId(token);

                    std::cout << "\nWelcome, " << users.getOwnProfile(currentUserId).name << "!\n";

                } else if (choice == 3) {
                    const std::vector<Resource> all = resources.listFiltered("", false);
                    std::cout << "\n" << all.size() << " resource(s) available:\n";
                    for (const Resource& resource : all) {
                        printResourceBrief(resource, users);
                    }
                } else {
                    std::cout << "Invalid choice.\n";
                }
            } else {
                if (choice == 0) break;

                switch (choice) {
                    case 4:
                        printProfile(users.getOwnProfile(currentUserId), true);
                        break;

                    case 5: {
                        std::cout << "(press Enter to keep a field unchanged)\n";
                        const std::string name      = readLine("New name: ");
                        const std::string city      = readLine("New city: ");
                        const std::string locality  = readLine("New locality: ");
                        const std::string skills    = readLine("New skills: ");
                        const std::string interests = readLine("New interests: ");
                        const std::string phone     = readLine("New contact number: ");

                        const UserView updated = users.updateProfile(currentUserId, name, city,
                                                                    locality, skills, interests,
                                                                    phone, "", "");
                        std::cout << "\nProfile updated.\n";
                        printProfile(updated, true);
                        break;
                    }

                    case 6: {
                        const std::string current = readLine("Current password: ");
                        const std::string next    = readLine("New password (min 6 chars): ");

                        users.updateProfile(currentUserId, "", "", "", "", "", "", next, current);
                        std::cout << "Password changed successfully.\n";
                        break;
                    }

                    case 7: {
                    
                        int addFirst = 0;
                        if (!readInt("\nAdd a new resource? 1 = yes, 0 = just browse: ", addFirst)) {
                            break;
                        }

                        if (addFirst == 1) {
                            const std::string title = readLine("Resource title: ");
                            const std::string desc  = readLine("Description: ");
                            const std::string category = chooseCategory();
                            if (category.empty()) break;

                            const int id = resources.createResource(currentUserId, title,
                                                                    desc, category);
                            std::cout << "Resource added with ID " << id << ".\n";
                        }

                        resourcesMenu(resources, requests, users, currentUserId);
                        break;
                    }

                    case 8:
                        helpRequestMenu(requests, resources, users, currentUserId);
                        break;

                    case 9:
                        priorityQueueMenu(requests, users, resources);
                        break;

                    case 10:
                        notificationsMenu(notifications, currentUserId);
                        break;

                    case 11:
                        searchAndDashboardMenu(search, currentUserId);
                        break;

                    case 12:
                        users.logout(token);
                        token.clear();
                        currentUserId = 0;
                        std::cout << "Logout successful.\n";
                        break;

                    default:
                        std::cout << "Invalid choice.\n";
                }
            }
        } catch (const AppError& error) {
            std::cout << "\n[!] " << error.what() << "\n";
        }
    }

    std::cout << "\nThank you for using the platform!\n";
    return 0;
}

} 
