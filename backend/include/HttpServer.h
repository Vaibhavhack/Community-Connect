#ifndef CC_HTTP_SERVER_H
#define CC_HTTP_SERVER_H
#include <string>
#include "FileManager.h"
#include "SessionManager.h"
#include "UserService.h"
#include "ResourceService.h"
#include "RequestService.h"
#include "NotificationService.h"
#include "SearchService.h"
namespace cc {
class HttpServer {
public:
 HttpServer(const std::string& host,int port,const std::string& frontendDir,FileManager& files,SessionManager& sessions,UserService& users,ResourceService& resources,RequestService& requests,NotificationService& notifications,SearchService& search);
 int run();
private:
 std::string host_; int port_; std::string frontendDir_; FileManager& files_; SessionManager& sessions_; UserService& users_; ResourceService& resources_; RequestService& requests_; NotificationService& notifications_; SearchService& search_;
};
}
#endif
