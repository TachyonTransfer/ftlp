#ifndef WIN32
#include <cstdlib>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <fstream>
#include <iostream>
#include <cstring>
#include <udt.h>
#include <dirent.h>
#include <filesystem>
#include "client_https.hpp"

using HttpsClient = SimpleWeb::Client<SimpleWeb::HTTPS>;
namespace fs = std::filesystem;

using namespace std;
struct thread_data
{
   UDTSOCKET *serv;
   sockaddr_storage *clientaddr;
   int addrlen;
};

#define PORT "9001" // the port users will be connecting to

#ifndef WIN32
void *recvfile(void *);
#else
DWORD WINAPI recvfile(LPVOID);
#endif
#ifndef WIN32
void *listfiles(void *);
#else
DWORD WINAPI listfiles(LPVOID);
#endif

void sigchld_handler(int s)
{
   (void)s; // quiet unused variable warning

   // waitpid() might overwrite errno, so we save and restore it:
   int saved_errno = errno;

   while (waitpid(-1, NULL, WNOHANG) > 0)
      ;

   errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
   if (sa->sa_family == AF_INET)
   {
      return &(((struct sockaddr_in *)sa)->sin_addr);
   }

   return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
   // usage: recvfile [server_port]
   if ((2 < argc) || ((2 == argc) && (0 == atoi(argv[1]))))
   {
      std::cout << "usage: recvfile [server_port]" << endl;
      return 0;
   }

   // use this function to initialize the UDT library
   UDT::startup();

   addrinfo hints;
   addrinfo *res;

   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   string service("9000");
   if (2 == argc)
      service = argv[1];

   if (0 != getaddrinfo(NULL, service.c_str(), &hints, &res))
   {
      std::cout << "illegal port number or port is busy.\n"
                << endl;
      return 0;
   }

   UDTSOCKET serv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

   // Windows UDP issue
   // For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
   int mss = 1052;
   UDT::setsockopt(serv, 0, UDT_MSS, &mss, sizeof(int));
#endif

   if (UDT::ERROR == UDT::setsockopt(serv, 0, UDT_SNDBUF, new int(20480000), sizeof(int)))
   {
      cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }
   if (UDT::ERROR == UDT::setsockopt(serv, 0, UDT_RCVBUF, new int(20480000), sizeof(int)))
   {
      cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen))
   {
      std::cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   freeaddrinfo(res);

   std::cout << "server is ready at port: " << service << endl;

   UDT::listen(serv, 10);

   sockaddr_storage clientaddr;
   int addrlen = sizeof(clientaddr);

   // TCP Setup
   int sockfd; // listen on sock_fd
   struct addrinfo hints_two, *servinfo, *p;
   socklen_t sin_size;
   struct sigaction sa;
   int yes = 1;
   int rv;

   memset(&hints_two, 0, sizeof hints_two);
   hints_two.ai_family = AF_UNSPEC;
   hints_two.ai_socktype = SOCK_STREAM;
   hints_two.ai_flags = AI_PASSIVE; // use my IP

   if ((rv = getaddrinfo(NULL, PORT, &hints_two, &servinfo)) != 0)
   {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
   }

   // loop through all the results and bind to the first we can
   for (p = servinfo; p != NULL; p = p->ai_next)
   {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1)
      {
         perror("server: socket");
         continue;
      }

      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                     sizeof(int)) == -1)
      {
         perror("setsockopt");
         exit(1);
      }

      if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
      {
         close(sockfd);
         perror("server: bind");
         continue;
      }

      break;
   }

   freeaddrinfo(servinfo); // all done with this structure

   if (p == NULL)
   {
      fprintf(stderr, "server: failed to bind\n");
      exit(1);
   }

   if (listen(sockfd, 10) == -1)
   {
      perror("listen");
      exit(1);
   }

   sa.sa_handler = sigchld_handler; // reap all dead processes
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART;
   if (sigaction(SIGCHLD, &sa, NULL) == -1)
   {
      perror("sigaction");
      exit(1);
   }

   printf("server: waiting for connections...\n");

   // while (true)
   //{

   thread_data *my_data = new thread_data();
   my_data->serv = &serv;
   my_data->clientaddr = &clientaddr;
   my_data->addrlen = addrlen;

#ifndef WIN32
   pthread_t filethread;
   pthread_create(&filethread, NULL, recvfile, my_data); // new UDTSOCKET(fhandle));
   pthread_detach(filethread);
#else
   CreateThread(NULL, 0, sendfile, new UDTSOCKET(fhandle), 0, NULL);
#endif

#ifndef WIN32
   pthread_t listthread;
   pthread_create(&listthread, NULL, listfiles, new int(sockfd));
   pthread_detach(listthread);
#else
   CreateThread(NULL, 0, listfiles, new UDTSOCKET(fhandle), 0, NULL);
#endif
   //}

   // UDT::close(serv);

   // // use this function to release the UDT library
   // UDT::cleanup();
   while (true)
      ;
   close(sockfd);

   return 0;
}

#ifndef WIN32
void *recvfileproc(void *s)
#else
DWORD WINAPI recvfile(LPVOID usocket)
#endif
{
   UDTSOCKET fhandle = *(UDTSOCKET *)s;

   HttpsClient client("app.posthog.com", false);
   std::string name_error_string = "{\"api_key\": \"RxdBl8vjdTwic7xTzoKTdbmeSC1PCzV6sw-x-FKSB-k\",\"event\": \"file upload\",\"properties\": {\"distinct_id\": \"molten\"},}";
   std::string namelength_error_string = "{\"api_key\": \"RxdBl8vjdTwic7xTzoKTdbmeSC1PCzV6sw-x-FKSB-k\",\"event\": \"file upload\",\"properties\": {\"distinct_id\": \"molten\"},}";
   std::string length_error_string = "{\"api_key\": \"RxdBl8vjdTwic7xTzoKTdbmeSC1PCzV6sw-x-FKSB-k\",\"event\": \"file upload\",\"properties\": {\"distinct_id\": \"molten\"},}";
   std::string size_error_string = "{\"api_key\": \"RxdBl8vjdTwic7xTzoKTdbmeSC1PCzV6sw-x-FKSB-k\",\"event\": \"file upload\",\"properties\": {\"distinct_id\": \"molten\"},}";
   std::string receiving_error_string = "{\"api_key\": \"RxdBl8vjdTwic7xTzoKTdbmeSC1PCzV6sw-x-FKSB-k\",\"event\": \"file upload\",\"properties\": {\"distinct_id\": \"molten\"},}";
   // std::string upload_tracking_string = "{\"api_key\": \"RxdBl8vjdTwic7xTzoKTdbmeSC1PCzV6sw-x-FKSB-k\",\"event\": \"file upload\",\"properties\": {\"distinct_id\": \"molten\"},}";

   std::string upload_tracking_string = "{\"api_key\": \"RxdBl8vjdTwic7xTzoKTdbmeSC1PCzV6sw-x-FKSB-k\",\"event\": \"[event name]\",\"properties\": {\"distinct_id\": \"[your users' distinct id]\",\"name\": \"value1\",\"size\": \"value2\"},}";
   // aquiring file name information from client
   char file[1024];
   int len;

   if (UDT::ERROR == UDT::recv(fhandle, (char *)&len, sizeof(int), 0))
   {
      client.request("POST", "/capture", name_error_string);
      std::cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   if (UDT::ERROR == UDT::recv(fhandle, file, len, 0))
   {
      client.request("POST", "/capture", namelength_error_string);
      std::cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }
   file[len] = '\0';

   // open the file
   int64_t size;

   // recv file size information
   if (UDT::ERROR == UDT::recv(fhandle, (char *)&size, sizeof(int64_t), 0))
   {

      std::cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   // Checking if we have enough disk space
   bool enough = std::filesystem::space(std::filesystem::path("/")).free > size;
   if (UDT::ERROR == UDT::send(fhandle, (char *)&enough, sizeof(bool), 0))
   {
      return 0;
   }
   if (!enough)
   {
      std::cout << "not enough space on disk..." << endl;
      return 0;
   }

   UDT::TRACEINFO trace;
   UDT::perfmon(fhandle, &trace);

   // recv the file
   fstream ofs(std::string("./files/" + std::string(file)).c_str(), ios::out | ios::binary | ios::trunc);
   int64_t recvsize;
   int64_t offset = 0;
   if (UDT::ERROR == UDT::recvfile(fhandle, ofs, offset, size))
   {
      std::cout << "recvfile: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   UDT::perfmon(fhandle, &trace);
   std::cout << "speed = " << trace.mbpsSendRate << "Mbits/sec" << endl;

   UDT::close(fhandle);

   ofs.close();

#ifndef WIN32
   return NULL;
#else
   return 0;
#endif
}

#ifndef WIN32
void *recvfile(void *s)
#else
DWORD WINAPI recvfile(LPVOID usocket)
#endif
{
   UDTSOCKET fhandle;

   while (true)
   {
      thread_data dat = *(thread_data *)s;
      UDTSOCKET serv = *dat.serv;
      sockaddr_storage clientaddr = *dat.clientaddr;
      int addrlen = dat.addrlen;
      UDT::setTLS(serv, 0);

      if (UDT::INVALID_SOCK == (fhandle = UDT::accept(serv, (sockaddr *)&clientaddr, &addrlen)))
      {
         std::cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
         continue;
      }

      char clienthost[NI_MAXHOST];
      char clientservice[NI_MAXSERV];
      getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST | NI_NUMERICSERV);
      std::cout << "new connection: " << clienthost << ":" << clientservice << endl;

#ifndef WIN32
      pthread_t fileprocthread;
      pthread_create(&fileprocthread, NULL, recvfileproc, new UDTSOCKET(fhandle));
      pthread_detach(fileprocthread);
#else
      CreateThread(NULL, 0, recvfileproc, new UDTSOCKET(fhandle), 0, NULL);
#endif
   }

#ifndef WIN32
   return NULL;
#else
   return 0;
#endif
}

#ifndef WIN32
void *listfiles(void *socket)
#else
DWORD WINAPI listfiles(LPVOID usocket)
#endif
{
   int sockfd = *(int *)socket;
   int new_fd;

   while (true)
   {
      struct sockaddr_storage their_addr; // connector's address information
      socklen_t sin_size = sizeof their_addr;
      new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
      if (new_fd == -1)
      {
         perror("accept");
         continue;
      }

      {
         char s[INET6_ADDRSTRLEN];
         inet_ntop(their_addr.ss_family,
                   get_in_addr((struct sockaddr *)&their_addr),
                   s, sizeof s);
         printf("server: got connection from %s\n", s);
      }

      string s;

      DIR *dir;
      struct dirent *ent;
      if ((dir = opendir("./files")) != NULL)
      {
         /* print all the files and directories within directory */
         while ((ent = readdir(dir)) != NULL)
         {
            s.append(ent->d_name).append("\n");
         }
         closedir(dir);
      }
      else
      {
         /* could not open directory */
         perror("");
         return NULL;
      }

      // child doesn't need the listener
      if (send(new_fd, s.c_str(), s.length(), 0) == -1)
         perror("send");

      close(new_fd);
   }

#ifndef WIN32
   return NULL;
#else
   return 0;
#endif
}
