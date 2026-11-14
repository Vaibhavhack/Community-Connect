// ConsoleDemo.h
// -----------------------------------------------------------------------------
// Purpose : The original console experience, rebuilt on top of the service
//           layer. It exists to prove that the web API and the old menu share
//           one core - not to duplicate logic.
//
// It contains NO business rules and NO file access: only cin/cout. Everything
// it does is a call into UserService / ResourceService / RequestService /
// NotificationService.
//
// Run with: ./build/cc-server --console
// -----------------------------------------------------------------------------

#ifndef CC_CONSOLE_DEMO_H
#define CC_CONSOLE_DEMO_H

#include "FileManager.h"
#include "NotificationService.h"
#include "RequestService.h"
#include "ResourceService.h"
#include "SessionManager.h"
#include "SearchService.h"
#include "UserService.h"

namespace cc {

// Interactive loop. Returns the process exit code.
int runConsoleDemo(FileManager& files,
                   SessionManager& sessions,
                   UserService& users,
                   ResourceService& resources,
                   RequestService& requests,
                   NotificationService& notifications,
                   SearchService& search);

} // namespace cc

#endif // CC_CONSOLE_DEMO_H
