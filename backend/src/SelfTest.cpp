
#include "SelfTest.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Errors.h"
#include "FileManager.h"
#include "Models.h"
#include "NotificationService.h"
#include "PasswordHasher.h"
#include "PriorityQueue.h"
#include "RequestService.h"
#include "ResourceService.h"
#include "SearchService.h"
#include "SessionManager.h"
#include "UserService.h"

namespace cc {

namespace {

int passed = 0;
int failed = 0;

void check(bool condition, const std::string& what) {
    if (condition) {
        ++passed;
        std::cout << "  [PASS] " << what << "\n";
    } else {
        ++failed;
        std::cout << "  [FAIL] " << what << "\n";
    }
}

void section(const std::string& title) {
    std::cout << "\n" << title << "\n";
}


template <typename Callable>
void checkThrows(Callable callable, const std::string& what) {
    try {
        callable();
        check(false, what + " (no error was thrown)");
    } catch (const AppError&) {
        check(true, what);
    } catch (const std::exception& error) {
        check(false, what + " (unexpected error: " + error.what() + ")");
    }
}

} 

int runSelfTest() {
    passed = 0;
    failed = 0;

    std::cout << "==================================================\n";
    std::cout << " Community Connect - Stage 2 backend self-test\n";
    std::cout << "==================================================\n";

    
    const std::string testDir = "build/selftest-data";
    std::error_code ec;
    std::filesystem::remove_all(testDir, ec);

    
    section("1. Escaping (fields may contain '|', '\\' and newlines)");
   
    {
        const std::string nasty = "Notes | with \\ a pipe\nand a newline";
        const std::string escaped = FileManager::escape(nasty);
        check(escaped.find('\n') == std::string::npos,
              "escape() removes raw newlines");
        check(FileManager::unescape(escaped) == nasty,
              "escape() then unescape() returns the original text");
    }

   
    section("2. Password hashing (PBKDF2-HMAC-SHA256, unique salt)");
   
    {
        const std::string password = "student@12345";
        const std::string hash1 = PasswordHasher::createHash(password);
        const std::string hash2 = PasswordHasher::createHash(password);

        check(hash1.rfind("pbkdf2_sha256$", 0) == 0,
              "stored format starts with pbkdf2_sha256$");
        check(hash1.find(password) == std::string::npos,
              "the plain password is not present in the hash");
        check(hash1 != hash2,
              "the same password gives two different hashes (unique salt)");
        check(PasswordHasher::verify(password, hash1),
              "correct password verifies");
        check(!PasswordHasher::verify("wrong-password", hash1),
              "wrong password is rejected");
        check(!PasswordHasher::verify(password, "garbage"),
              "a corrupt stored hash returns false instead of crashing");
    }

    
    section("3. Registration and validation");
   
    FileManager files(testDir);
    SessionManager sessions;
    UserService users(files, sessions);

    int aditiId = 0;
    {
        aditiId = users.registerUser("Aditi Sharma", "aditi", "Aditi.Sharma@college.edu",
                                     "student@12345", "Pune", "Kothrud",
                                     "C++, DSA, DSA", "Web development, AI",
                                     "+91 98765 43210");

        check(aditiId == 1, "first user gets id 1");
        check(users.userCount() == 1, "user count is 1");
        check(users.getOwnProfile(aditiId).skills == "C++, DSA",
              "skills are trimmed and de-duplicated ('C++, DSA, DSA' -> 'C++, DSA')");
        check(users.getOwnProfile(aditiId).email == "aditi.sharma@college.edu",
              "email is normalised to lowercase");
        check(users.getPublicProfile(aditiId).email.empty(),
              "public profile hides the email");
        check(users.getPublicProfile(aditiId).contactPhone.empty(),
              "public profile hides the contact number");
        check(!users.getOwnProfile(aditiId).email.empty(),
              "own profile still shows the email");

        checkThrows([&] { users.registerUser("Copy Cat", "copycat",
                                             "aditi.sharma@college.edu", "student@12345",
                                             "Pune", "", "", "", ""); },
                    "duplicate email is rejected");

        checkThrows([&] { users.registerUser("Copy Cat", "ADITI",
                                             "copy@college.edu", "student@12345",
                                             "Pune", "", "", "", ""); },
                    "duplicate username is rejected case-insensitively");

        checkThrows([&] { users.registerUser("Bad Email", "badmail", "not-an-email",
                                             "student@12345", "Pune", "", "", "", ""); },
                    "invalid email is rejected");

        checkThrows([&] { users.registerUser("Short Pass", "shortpass",
                                             "short@college.edu", "123",
                                             "Pune", "", "", "", ""); },
                    "password shorter than 6 characters is rejected");

        checkThrows([&] { users.registerUser("No City", "nocity",
                                             "nocity@college.edu", "student@12345",
                                             "", "", "", "", ""); },
                    "missing city is rejected");
    }

    
    section("4. Persistence (data survives a restart)");
    
    {
        check(std::filesystem::exists(files.pathOf(UserService::kFileName)),
              "users.txt was created automatically");

      
        SessionManager sessions2;
        UserService reopened(files, sessions2);

        check(reopened.userCount() == 1, "the record is reloaded from users.txt");
        check(reopened.exists(aditiId), "user id survives the reload");
        check(reopened.getOwnProfile(aditiId).name == "Aditi Sharma",
              "name survives the reload");

      
        check(!reopened.login("aditi", "student@12345").empty(),
              "the stored password hash still verifies after a restart");

       
        const int secondId = reopened.registerUser("Rahul Verma", "rahul",
                                                   "rahul@college.edu", "student@12345",
                                                   "Mumbai", "Andheri",
                                                   "Java, DSA", "Cloud computing", "");
        check(secondId == aditiId + 1,
              "next user id continues after the highest stored id (no collision)");
    }

  
    section("5. Authentication and sessions");
  
    {
        SessionManager sessions3;
        UserService service(files, sessions3);

        const std::string token = service.login("aditi", "student@12345");
        check(!token.empty() && token.size() == 64,
              "login by username returns a 64-character hex token");

        check(sessions3.requireUserId(token) == aditiId,
              "the token resolves to the correct user id");

        const std::string emailToken = service.login("ADITI.SHARMA@college.edu",
                                                     "student@12345");
        check(!emailToken.empty(), "login by email also works (case-insensitive)");

        checkThrows([&] { service.login("aditi", "wrong-password"); },
                    "wrong password is rejected");
        checkThrows([&] { service.login("nobody", "student@12345"); },
                    "unknown account is rejected");

        service.logout(token);
        checkThrows([&] { sessions3.requireUserId(token); },
                    "after logout the token is no longer valid");
        check(sessions3.requireUserId(emailToken) == aditiId,
              "logging out one session does not affect another");
    }

    
    section("6. Profile editing");
    
    {
        SessionManager sessions4;
        UserService service(files, sessions4);

        const UserView edited = service.updateProfile(aditiId,
                                                     "",           
                                                     "Pune City",   
                                                     "", "", "", "", "", "");

        check(edited.city == "Pune City", "city is updated");
        check(edited.name == "Aditi Sharma", "empty field leaves name unchanged");

        checkThrows([&] { service.updateProfile(aditiId, "", "", "", "", "", "", 
                                                "newpassword", "wrong-current"); },
                    "changing the password with a wrong current password fails");

        service.updateProfile(aditiId, "", "", "", "", "", "", "newpassword123",
                              "student@12345");
        checkThrows([&] { service.login("aditi", "student@12345"); },
                    "the old password stops working after a change");
        check(!service.login("aditi", "newpassword123").empty(),
              "the new password works");
    }

   
    section("7. Corrupt record handling");
    
    {
        
        const std::string path = files.pathOf(UserService::kFileName);
        {
            std::ofstream appender(path, std::ios::app);
            appender << "this line is not a valid user record\n";
            appender << "999|2026-01-01T00:00:00Z|Only|Three|Fields\n";
        }

        SessionManager sessions5;
        UserService service(files, sessions5);

        check(service.userCount() == 2,
              "corrupt lines are skipped and the 2 good records still load");
        check(service.exists(aditiId), "the valid records are intact");
    }

    
    const std::string stage3Dir = "build/selftest-data-stage3";
    std::filesystem::remove_all(stage3Dir, ec);

    FileManager files3(stage3Dir);
    SessionManager sessions3b;
    UserService users3(files3, sessions3b);
    ResourceService resources(files3);
    NotificationService notifications(files3);
    RequestService requests(files3, resources, users3, notifications);

  
    const int ownerId = users3.registerUser("Isha Patil", "isha", "isha@college.edu",
                                            "student@12345", "Pune", "Baner",
                                            "C++, DSA", "Teaching", "+91 90000 11111");
    const int requesterId = users3.registerUser("Rahul Verma", "rahul", "rahul@college.edu",
                                                "student@12345", "Mumbai", "Andheri",
                                                "Java, DBMS", "Machine learning", "+91 90000 22222");
    const int strangerId = users3.registerUser("Nina Rao", "nina", "nina@college.edu",
                                               "student@12345", "Nagpur", "Sadar",
                                               "Python", "Robotics", "");

    
    section("8. Priority queue as a data structure");
    
    {
        PriorityQueue<HelpRequest> queue([](const HelpRequest& a, const HelpRequest& b) {
            if (a.priority != b.priority) {
                return static_cast<int>(a.priority) > static_cast<int>(b.priority);
            }
            return a.getId() < b.getId();  
        });

        auto makeRequest = [](int id, Priority priority) {
            HelpRequest item;
            item.setId(id);
            item.priority = priority;
            item.title = "Request " + std::to_string(id);
            return item;
        };

        checkThrows([&] { queue.top(); }, "peeking an empty queue throws");

       
        queue.push(makeRequest(1, Priority::Low));
        queue.push(makeRequest(2, Priority::High));
        queue.push(makeRequest(3, Priority::Medium));

        check(queue.size() == 3, "size() counts the inserted items");
        check(queue.top().getId() == 2, "peek returns the HIGH priority request");

        
        queue.push(makeRequest(5, Priority::High));
        queue.push(makeRequest(4, Priority::High));
        check(queue.top().getId() == 2, "equal priority -> older request (smaller id) first");

        
        const std::size_t sizeBeforePeek = queue.size();
        queue.top();
        check(queue.size() == sizeBeforePeek, "peek does not remove anything from the queue");
    }

    {
        PriorityQueue<HelpRequest> queue([](const HelpRequest& a, const HelpRequest& b) {
            if (a.priority != b.priority) {
                return static_cast<int>(a.priority) > static_cast<int>(b.priority);
            }
            return a.getId() < b.getId();
        });

        auto makeRequest = [](int id, Priority priority) {
            HelpRequest item;
            item.setId(id);
            item.priority = priority;
            item.title = "Request " + std::to_string(id);
            return item;
        };

        std::vector<HelpRequest> source = {
            makeRequest(10, Priority::Low),
            makeRequest(11, Priority::Medium),
            makeRequest(12, Priority::High),
            makeRequest(13, Priority::High)
        };

      
        PriorityQueue<HelpRequest> built([](const HelpRequest& a, const HelpRequest& b) {
            if (a.priority != b.priority) {
                return static_cast<int>(a.priority) > static_cast<int>(b.priority);
            }
            return a.getId() < b.getId();
        });
        built.buildHeap(source);

        check(built.size() == 4, "buildHeap loads all items");
        check(built.top().getId() == 12,
              "buildHeap restores priority order (oldest High first)");

       
        const std::size_t before = built.size();
        const std::vector<HelpRequest> ordered = built.sortedCopy();

        check(ordered.size() == 4, "sortedCopy returns every item");
        check(ordered[0].getId() == 12 && ordered[1].getId() == 13,
              "sortedCopy orders the two High requests oldest-first");
        check(ordered[2].getId() == 11 && ordered[3].getId() == 10,
              "sortedCopy places Medium before Low");
        check(built.size() == before, "sortedCopy leaves the queue unchanged");

        
        std::vector<int> extracted;
        while (!built.empty()) {
            extracted.push_back(built.pop().getId());
        }
        check(extracted == std::vector<int>({12, 13, 11, 10}),
              "repeated pop() yields strict priority order: 12, 13, 11, 10");
        check(built.empty(), "the queue is empty after extracting everything");
        checkThrows([&] { built.pop(); }, "popping an empty queue throws");
    }

    
    section("9. Resource management");
    
    int resourceId = 0;
    {
        resourceId = resources.createResource(ownerId, "C++ Primer (5th edition)",
                                             "Textbook in good condition, covers STL and templates.",
                                             "Books and Textbooks");
        check(resourceId > 0, "an owner can add a resource");
        check(resources.count() == 1, "resource count is 1");
        check(resources.requireResource(resourceId).availability == Availability::Available,
              "a new resource starts as Available");
        check(!resources.requireResource(resourceId).dateAdded.empty(),
              "dateAdded is set automatically");

        checkThrows([&] { resources.createResource(ownerId, "Random item",
                                                   "Some description that is long enough.",
                                                   "Food and groceries"); },
                    "a non-educational category is rejected");

        checkThrows([&] { resources.createResource(ownerId, "Hi",
                                                   "Some description that is long enough.",
                                                   "Stationery"); },
                    "a too-short title is rejected");

        checkThrows([&] { resources.updateResource(resourceId, requesterId, "Hijacked", "", ""); },
                    "a non-owner cannot edit somebody else's resource");

        const Resource edited = resources.updateResource(resourceId, ownerId,
                                                         "C++ Primer (5th ed.)", "", "");
        check(edited.title == "C++ Primer (5th ed.)", "the owner can edit their own resource");
        check(edited.description == "Textbook in good condition, covers STL and templates.",
              "an empty field leaves the description unchanged");

        check(resources.listFiltered("Books and Textbooks", false).size() == 1,
              "filtering by category finds the resource");
        check(resources.listFiltered("Stationery", false).empty(),
              "filtering by another category excludes it");
        check(resources.listFiltered("All", true).size() == 1,
              "the Available filter includes an available resource");
    }

   
    section("10. Help requests, priority queue and the state machine");
   
    int highRequestId = 0;
    {
        
        checkThrows([&] { requests.createRequest(0, resourceId, "Need help",
                                                 "Please help me understand pointers.",
                                                 "C++", "", Priority::High); },
                    "a request cannot be created without logging in");

        highRequestId = requests.createRequest(requesterId, resourceId,
                                               "Need help with STL",
                                               "I need help understanding iterators and algorithms.",
                                               "C++, DSA", "", Priority::High);
        check(highRequestId > 0, "a requester can create a request against a resource");

        const HelpRequest created = requests.requireRequest(highRequestId);
        check(created.status == RequestStatus::Pending, "a new request starts as Pending");
        check(created.priority == Priority::High, "the priority is stored");
        check(created.category == "Books and Textbooks",
              "the resource category is inherited automatically");
        check(!created.updatedAt.empty(), "updatedAt is stamped");

        checkThrows([&] { requests.createRequest(ownerId, resourceId, "My own resource",
                                                 "I should not be able to request my own item.",
                                                 "", "", Priority::Low); },
                    "requesting your own resource is refused (preserved rule)");

       
        const int lowId = requests.createRequest(requesterId, resourceId,
                                                 "Low priority note share",
                                                 "Would like the summary notes whenever convenient.",
                                                 "Notes", "", Priority::Low);
        const int mediumId = requests.createRequest(requesterId, resourceId,
                                                   "Medium priority practice",
                                                   "Could we practise three DSA problems together.",
                                                   "DSA", "", Priority::Medium);

        const std::vector<HelpRequest> pending = requests.listPendingByPriority();
        check(pending.size() == 3, "all three pending requests are in the queue");
        check(pending[0].getId() == highRequestId, "the High request is first");
        check(pending[1].getId() == mediumId, "the Medium request is second");
        check(pending[2].getId() == lowId, "the Low request is last");

        check(requests.pendingCount() == 3, "pendingCount matches the queue size");
        check(requests.peekHighestPriority().getId() == highRequestId,
              "O(1) peek returns the most urgent request");

     
        check(requests.requireRequest(highRequestId).status == RequestStatus::Pending,
              "peeking does not change any status");

        HelpRequest taken;
        check(requests.takeHighestPriority(taken), "takeHighestPriority extracts an item");
        check(taken.getId() == highRequestId, "it extracts the most urgent one");
        check(requests.requireRequest(highRequestId).status == RequestStatus::Pending,
              "extraction does not silently change the request status");
    }

    {
      
        checkThrows([&] { requests.acceptRequest(highRequestId, requesterId); },
                    "the requester cannot accept their own request");
        checkThrows([&] { requests.acceptRequest(highRequestId, strangerId); },
                    "an unrelated user cannot accept somebody else's request");
        checkThrows([&] { requests.rejectRequest(highRequestId, strangerId); },
                    "an unrelated user cannot reject somebody else's request");
        checkThrows([&] { requests.cancelRequest(highRequestId, ownerId); },
                    "the resource owner cannot cancel the requester's request");

        const HelpRequest accepted = requests.acceptRequest(highRequestId, ownerId);
        check(accepted.status == RequestStatus::Accepted, "the resource owner can accept");
        check(accepted.acceptOwnerId == ownerId, "the accepting user is recorded");

        checkThrows([&] { requests.acceptRequest(highRequestId, ownerId); },
                    "a request cannot be accepted twice");
        checkThrows([&] { requests.cancelRequest(highRequestId, requesterId); },
                    "an accepted request cannot be cancelled");
        checkThrows([&] { requests.updateRequest(highRequestId, requesterId, "New title",
                                                 "", "", "", Priority::Low); },
                    "an accepted request cannot be edited");

        check(requests.pendingCount() == 2,
              "the accepted request leaves the priority queue (3 -> 2)");
        check(!requests.listPendingByPriority().empty() &&
                  requests.listPendingByPriority()[0].priority == Priority::Medium,
              "the next most urgent pending request is now Medium");

        checkThrows([&] { requests.completeRequest(highRequestId, strangerId); },
                    "an outsider cannot mark an exchange completed");

        const HelpRequest completed = requests.completeRequest(highRequestId, requesterId);
        check(completed.status == RequestStatus::Completed,
              "either party can mark an accepted exchange completed");

        checkThrows([&] { requests.completeRequest(highRequestId, ownerId); },
                    "an already completed request cannot be completed again");
    }

  
    section("11. Contact privacy gate (enforced in C++, not in the UI)");
    
    {
        
        const int lockedId = requests.createRequest(requesterId, resourceId,
                                                    "Details should stay hidden",
                                                    "This request is still pending, so contacts hide.",
                                                    "C++", "", Priority::Medium);

        checkThrows([&] { requests.contactDetailsFor(lockedId, requesterId); },
                    "the requester cannot see contacts while the request is Pending");
        checkThrows([&] { requests.contactDetailsFor(lockedId, ownerId); },
                    "the owner cannot unlock contacts before accepting");

        
        requests.rejectRequest(lockedId, ownerId);
        checkThrows([&] { requests.contactDetailsFor(lockedId, requesterId); },
                    "a rejected request still hides contact details");

       
        const int openId = requests.createRequest(requesterId, resourceId,
                                                  "Share the book",
                                                  "I would like to borrow the C++ primer please.",
                                                  "C++", "", Priority::High);
        requests.acceptRequest(openId, ownerId);

        const ContactDetails forRequester = requests.contactDetailsFor(openId, requesterId);
        check(forRequester.userId == ownerId,
              "the requester sees the owner's details after acceptance");
        check(forRequester.email == "isha@college.edu", "the owner's email is revealed");
        check(forRequester.phone == "+91 90000 11111", "the owner's phone is revealed");

        const ContactDetails forOwner = requests.contactDetailsFor(openId, ownerId);
        check(forOwner.userId == requesterId,
              "the owner sees the requester's details after acceptance");
        check(forOwner.email == "rahul@college.edu", "the requester's email is revealed");

        checkThrows([&] { requests.contactDetailsFor(openId, strangerId); },
                    "a third user still cannot read the contact details");

       
        check(users3.getPublicProfile(ownerId).email.empty(),
              "public profiles still hide the email");
        check(users3.getPublicProfile(ownerId).contactPhone.empty(),
              "public profiles still hide the phone");
    }

    
    section("12. Notifications");
    
    {
        const std::vector<Notification> ownerFeed = notifications.listFor(ownerId);
        check(!ownerFeed.empty(), "the owner received a notification for each request");

        bool sawResourceRequest = false;
        for (const Notification& item : ownerFeed) {
            if (item.type == notificationType::kResourceRequest) sawResourceRequest = true;
        }
        check(sawResourceRequest, "a resource_request notification exists");

        const std::vector<Notification> requesterFeed = notifications.listFor(requesterId);
        bool sawAccepted = false;
        bool sawRejected = false;
        for (const Notification& item : requesterFeed) {
            if (item.type == notificationType::kRequestAccepted) sawAccepted = true;
            if (item.type == notificationType::kRequestRejected) sawRejected = true;
        }
        check(sawAccepted, "accepting notifies the requester");
        check(sawRejected, "rejecting notifies the requester");

        const std::size_t unread = notifications.unreadCount(requesterId);
        check(unread > 0, "the requester has unread notifications");

      
        const Notification foreign = requesterFeed.front();
        checkThrows([&] { notifications.markRead(ownerId, foreign.getId()); },
                    "a user cannot mark somebody else's notification read");

        notifications.markRead(requesterId, foreign.getId());
        check(notifications.unreadCount(requesterId) == unread - 1,
              "marking one read decreases the unread count");

        const std::size_t cleared = notifications.markAllRead(requesterId);
        check(cleared > 0, "markAllRead reports how many it changed");
        check(notifications.unreadCount(requesterId) == 0,
              "no unread notifications remain");

        
        NotificationService reloaded(files3);
        check(reloaded.listFor(requesterId).size() == requesterFeed.size(),
              "notifications survive a restart (loaded from notifications.txt)");
        check(reloaded.unreadCount(requesterId) == 0,
              "the read/unread state is persisted too");
    }

    
    section("13. Referential integrity on delete");
    
    {
        
        auto liveCheck = [&requests](int id) { return requests.hasLiveRequests(id); };

        checkThrows([&] { resources.deleteResource(resourceId, ownerId, liveCheck); },
                    "a resource with open requests cannot be deleted");

        checkThrows([&] { resources.deleteResource(resourceId, requesterId, liveCheck); },
                    "a non-owner cannot delete the resource at all");

       
        const int spareId = resources.createResource(ownerId, "Spare graph paper",
                                                    "Unused graph paper, no requests attached.",
                                                    "Stationery");
        resources.deleteResource(spareId, ownerId, liveCheck);
        check(!resources.exists(spareId), "an unreferenced resource can be deleted");
        check(resources.count() == 1, "the other resource is untouched");
    }

    std::filesystem::remove_all(stage3Dir, ec);

  
    section("14. Search, skill matching and dashboard");
    
    {
        SearchService search(users3, resources, requests, notifications);

        const std::vector<RankedResource> resourceResults =
            search.searchResources("C++", "", true, 10);
        check(!resourceResults.empty(), "resource search finds C++ resources");
        check(resourceResults.front().resource.getId() == resourceId,
              "resource search ranks the C++ title/description match first");
        check(!resourceResults.front().matchedTerms.empty(),
              "resource search reports the matched query term");

        const std::vector<RankedResource> categoryResults =
            search.searchResources("", "Books and Textbooks", false, 10);
        check(!categoryResults.empty(), "empty-keyword search can filter by category");
        check(categoryResults.front().resource.category == "Books and Textbooks",
              "category filter is applied by the search service");

        const std::vector<UserMatch> skillResults =
            search.searchUsers("C++");
        check(!skillResults.empty(), "user search finds users with a matching skill");
        check(skillResults.front().user.id == ownerId,
              "skill search ranks the exact C++ skill match first");
        check(skillResults.front().user.email.empty() &&
                  skillResults.front().user.contactPhone.empty(),
              "search results never expose private contact details");

        const std::vector<UserMatch> requestMatches =
            search.matchUsersForRequest(highRequestId);
        check(!requestMatches.empty(), "request matching finds a suitable helper");
        check(requestMatches.front().user.id == ownerId,
              "request matching ranks the user with the required C++ skill first");

        const DashboardStats dashboard = search.dashboardFor(ownerId);
        check(dashboard.resourcesOffered == resources.countByOwner(ownerId),
              "dashboard resource count matches ResourceService");
        check(dashboard.unreadNotifications == notifications.unreadCount(ownerId),
              "dashboard unread count matches NotificationService");
        check(dashboard.activeHelpRequests == dashboard.pendingRequests +
                  dashboard.acceptedRequests,
              "dashboard active requests equals pending plus accepted");
    }

    std::cout << "\n--------------------------------------------------\n";
    std::cout << " Passed: " << passed << "   Failed: " << failed << "\n";
    std::cout << "--------------------------------------------------\n";

    if (failed == 0) {
        std::cout << " Stages 2 and 3 verified.\n\n";
    } else {
        std::cout << " Some checks failed - please send this output back.\n\n";
    }

    std::filesystem::remove_all(testDir, ec);

    return failed;
}

} // namespace cc
