#ifndef WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <udt.h>
#include "tachyon.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>

#define MAXDATASIZE 1024 // max number of bytes we can get at once

using namespace std;

enum TACHYON_ERR
{
    SEND = -7,
    SEND_NAME,
    SEND_NAME_LENGTH,
    SEND_LENGTH,
    CONNECT,
    SIZE,
    ADDR
};

std::vector<std::string> splitString(const std::string &str)
{
    std::vector<std::string> tokens;

    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, '\n'))
    {
        tokens.push_back(token);
    }

    return tokens;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

Tachyon::Tachyon(const char *address, const char *port) : m_UDPPort(port),
                                                          m_Address(address)
{
    m_TCPPort = to_string(stoi(string(port)) + 1);
}

Tachyon::~Tachyon()
{
}

double Tachyon::upload(const char *key, const char *filename, v8::Local<v8::Function> func, v8::Local<v8::Context> ctx, v8::Isolate *isolate)
{
    // use this function to initialize the UDT library
    UDT::startup();

    std::string tmp_key(key);

    std::string tmp_filename_two(filename);
    std::string tmp_filename = tmp_filename_two.substr(tmp_filename_two.find_last_of("/\\") + 1);
    struct addrinfo hints, *peer;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    UDTSOCKET fhandle = UDT::socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
#ifdef OSX
    if (UDT::ERROR == UDT::setsockopt(fhandle, 0, UDP_RCVBUF, new int(1000000), sizeof(int)))
    {
        cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }
#endif
    if (UDT::ERROR == UDT::setsockopt(fhandle, 0, UDT_SNDBUF, new int(20480000), sizeof(int)))
    {
        cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }
    if (UDT::ERROR == UDT::setsockopt(fhandle, 0, UDT_RCVBUF, new int(20480000), sizeof(int)))
    {
        cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }

    if (0 != getaddrinfo(m_Address.c_str(), m_UDPPort.c_str(), &hints, &peer))
    {
        return TACHYON_ERR::ADDR;
    }

    UDT::setTLS(fhandle, 1);

    // connect to the server, implict bind
    if (UDT::ERROR == UDT::connect(fhandle, peer->ai_addr, peer->ai_addrlen))
    {
        std::cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
        return TACHYON_ERR::CONNECT;
    }

    freeaddrinfo(peer);

    // send name information of the requested file
    int len = strlen(tmp_key.c_str());

    if (UDT::ERROR == UDT::send(fhandle, (char *)&len, sizeof(int), 0))
    {
        return TACHYON_ERR::SEND_NAME_LENGTH;
    }

    if (UDT::ERROR == UDT::send(fhandle, tmp_key.c_str(), len, 0))
    {
        return TACHYON_ERR::SEND_NAME;
    }

    // send size information

    fstream ifs(tmp_filename_two.c_str(), ios::in | ios::binary);

    ifs.seekg(0, ios::end);
    int64_t size = ifs.tellg();
    ifs.seekg(0, ios::beg);
    int64_t offset = 0;

    if (UDT::ERROR == UDT::send(fhandle, (char *)&size, sizeof(int64_t), 0))
    {
        return TACHYON_ERR::SEND_LENGTH;
    }

    // recv file size information
    bool enough;
    if (UDT::ERROR == UDT::recv(fhandle, (char *)&enough, sizeof(bool), 0))
    {
        std::cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
        return TACHYON_ERR::SIZE;
    }
    if (!enough)
        return TACHYON_ERR::SIZE;
    if (size < 0)
    {
        return TACHYON_ERR::SIZE;
    }

    UDT::TRACEINFO trace;
    UDT::perfmon(fhandle, &trace);

    // send the file
    if (UDT::ERROR == UDT::sendfile(fhandle, ifs, offset, size, 364000, func, ctx, isolate))
    {
        return TACHYON_ERR::SEND;
    }

    UDT::perfmon(fhandle, &trace);
    double speed = trace.mbpsSendRate;

    UDT::close(fhandle);

    ifs.close();

    // use this function to release the UDT library
    UDT::cleanup();
    return speed;
}
std::vector<std::string> Tachyon::list()
{
    std::vector<std::string> ret;

    int sockfd, numbytes;
    char *buf = (char *)malloc(1024);
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(m_Address.c_str(), m_TCPPort.c_str(), &hints, &servinfo)) != 0)
    {
        return ret;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        return ret;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);

    freeaddrinfo(servinfo); // all done with this structure

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
    {
        return ret;
    }

    buf[numbytes] = '\0';

    close(sockfd);

    return splitString(std::string(buf));
}
