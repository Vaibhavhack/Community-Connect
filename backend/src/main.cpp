// main.cpp
// -----------------------------------------------------------------------------
// Purpose : Program entry point. Command-line parsing and start-up only.
//
// The original main.cpp contained the whole menu. It now only wires the
// objects together and hands control to one of two run modes.
//
// Usage
//   ./build/cc-server --selftest          verify the Stage 2 backend (start here)
//   ./build/cc-server --console           console version of the original menu
//   ./build/cc-server                     (Stage 5 will start the HTTP server)
//
// Options
//   --data <dir>   where the .txt files live (default: ./data)
//   --help         this text
// -----------------------------------------------------------------------------

#include <exception>
#include <iostream>
#include <string>

#include "ConsoleDemo.h"
#include "Errors.h"
#include "FileManager.h"
#include "NotificationService.h"
#include "RequestService.h"
#include "ResourceService.h"
#include "SearchService.h"
#include "SelfTest.h"
#include "SessionManager.h"
#include "UserService.h"
#include "HttpServer.h"

namespace {

void printUsage() {
    std::cout <<
        "Community Connect - Community Resource and Help Exchange Platform\n"
        "\n"
        "Usage: cc-server [options]\n"
        "\n"
        "  --selftest        Run the backend self-test (Stage 2 verification)\n"
        "  --console         Run the console version of the application\n"
        "  --seed-demo       Create sample data only when the data directory is empty\n"
        "  --host <addr>     HTTP bind address (default: 127.0.0.1)\n"
        "  --port <n>        HTTP port (default: 8080)\n"
        "  --frontend <dir>  Frontend directory (default: ../frontend)\n"
        "  --data <dir>      Directory for users.txt, resources.txt, ...\n"
        "                    (default: ./data)\n"
        "  --help            Show this message\n"
        "\n"
        "The default mode starts the local HTTP server.\n";
}

} // namespace

int main(int argc, char* argv[]) {
    bool selfTest = false;
    bool consoleMode = false;
    bool seedDemo = false;
    std::string dataDir = "data";
    std::string host = "127.0.0.1";
    int port = 8080;
    std::string frontendDir = "../frontend";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--selftest") {
            selfTest = true;
        } else if (arg == "--console") {
            consoleMode = true;
        } else if (arg == "--seed-demo") {
            seedDemo = true;
        } else if (arg == "--host") {
            if (i + 1 >= argc) { std::cerr << "Error: --host needs an address.\n"; return 1; }
            host = argv[++i];
        } else if (arg == "--port") {
            if (i + 1 >= argc) { std::cerr << "Error: --port needs a number.\n"; return 1; }
            port = std::stoi(argv[++i]);
            if (port < 1 || port > 65535) { std::cerr << "Error: invalid port.\n"; return 1; }
        } else if (arg == "--frontend") {
            if (i + 1 >= argc) { std::cerr << "Error: --frontend needs a directory.\n"; return 1; }
            frontendDir = argv[++i];
        } else if (arg == "--data") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --data needs a directory name.\n";
                return 1;
            }
            dataDir = argv[++i];
        } else {
            std::cerr << "Error: unknown option '" << arg << "'.\n\n";
            printUsage();
            return 1;
        }
    }

    try {
        // The self-test builds its own throwaway services in build/selftest-data,
        // so it runs BEFORE anything touches the real data directory.
        if (selfTest) {
            return cc::runSelfTest() == 0 ? 0 : 1;
        }

        // Creating the FileManager creates data/ if it does not exist yet, so a
        // fresh clone works with no manual setup.
        cc::FileManager files(dataDir);
        cc::SessionManager sessions;

        // Composition order matters: the request module needs the resource,
        // user and notification modules to already exist.
        cc::UserService users(files, sessions);
        cc::ResourceService resources(files);
        cc::NotificationService notifications(files);
        cc::RequestService requests(files, resources, users, notifications);
        cc::SearchService search(users, resources, requests, notifications);

        if (seedDemo) {
            if (users.userCount() != 0 || resources.count() != 0 || notifications.totalCount() != 0) {
                std::cout << "Demo seed skipped: existing data was found. No real data was changed.\n";
                return 0;
            }
            const int aman = users.registerUser("Aman Sharma", "aman", "aman.demo@communityconnect.local", "demo1234", "Dehradun", "Rajpur Road", "C++, DSA, Algorithms", "Programming, Projects", "9000000001");
            const int riya = users.registerUser("Riya Singh", "riya", "riya.demo@communityconnect.local", "demo1234", "Dehradun", "Clement Town", "Java, DBMS, Web Development", "Web, Databases", "9000000002");
            const int kabir = users.registerUser("Kabir Mehta", "kabir", "kabir.demo@communityconnect.local", "demo1234", "Haridwar", "Jwalapur", "Python, Machine Learning", "AI, Data Science", "9000000003");
            const int r1 = resources.createResource(aman, "C++ and DSA Notes", "Exam-focused notes covering C++, STL, stacks, queues, trees and graphs.", "Notes and Study Materials");
            const int r2 = resources.createResource(riya, "DBMS Previous Year Papers", "Collection of university DBMS papers arranged by semester for practice.", "Previous Year Question Papers");
            const int r3 = resources.createResource(kabir, "Machine Learning Project Guide", "A beginner-friendly guide for planning and documenting an academic ML project.", "Academic Projects");
            (void)r1; (void)r2; (void)r3;
            requests.createRequest(riya, r1, "Need C++ notes", "I need C++ and DSA notes for end-term revision.", "C++, DSA", "Notes and Study Materials", cc::Priority::High);
            requests.createRequest(kabir, 0, "Need DBMS guidance", "Looking for help understanding normalization and SQL joins.", "DBMS, SQL", "Programming Resources", cc::Priority::Medium);
            std::cout << "Demo data created successfully.\nUsers: aman / riya / kabir\nDemo password: demo1234\n";
            return 0;
        }

        if (consoleMode) {
            return cc::runConsoleDemo(files, sessions, users, resources, requests, notifications, search);
        }

        cc::HttpServer server(host, port, frontendDir, files, sessions, users, resources, requests, notifications, search);
        return server.run();

    } catch (const cc::AppError& error) {
        std::cerr << "\nStartup failed: " << error.what() << "\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "\nUnexpected startup error: " << error.what() << "\n";
        return 1;
    }
}
