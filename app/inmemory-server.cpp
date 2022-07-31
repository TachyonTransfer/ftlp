/*****************************************************************************
Copyright (c) 2001 - 2010, The Board of Trustees of the University of Illinois.
All rights reserved.

Copyright (c) 2020 - 2022, Tachyon Transfer, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions
  and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the University of Illinois
  nor the names of its contributors may be used to
  endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <arpa/inet.h>
#include <udt.h>
#include <iostream>
#include <cstring>
#include <charconv>
#include <chrono>

class Timer
{
    // make things readable
    using clk = std::chrono::steady_clock;

    clk::time_point b; // begin
    clk::time_point e; // end

public:
    void clear() { b = e = clk::now(); }
    void start() { b = clk::now(); }
    void stop() { e = clk::now(); }

    friend std::ostream& operator<<(std::ostream& o, const Timer& timer)
    {
        return o << timer.secs();
    }

    // return time difference in seconds
    double secs() const
    {
        if(e <= b)
            return 0.0;
        auto d = std::chrono::duration_cast<std::chrono::microseconds>(e - b);
        return d.count() / 1000000.0;
    }
};


int main()
{
    using namespace std;

    UDTSOCKET serv = UDT::socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(9000);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);

    if (UDT::ERROR == UDT::bind(serv, (sockaddr *)&my_addr, sizeof(my_addr)))
    {
        cout << "bind: " << UDT::getlasterror().getErrorMessage();
        return 0;
    }

    UDT::listen(serv, 10);

    int namelen;
    sockaddr_in their_addr;

    UDTSOCKET recver = UDT::accept(serv, (sockaddr *)&their_addr, &namelen);

    cout << "new connection: " << inet_ntoa(their_addr.sin_addr) << ":" << ntohs(their_addr.sin_port) << endl;

    const int filesize_bufsize = 256;
    char *filesize_buf = new char[filesize_bufsize];
    long filesize = 0;
    long currsize = 0;
    long recvsize = 0;
    char *recvbuf;

    currsize = UDT::recv(recver, filesize_buf, filesize_bufsize, 0);
    if (UDT::ERROR == currsize) 
    {
        cout << "recv filesize: " << UDT::getlasterror().getErrorMessage();
        return -1;
    }

    std::from_chars(filesize_buf, filesize_buf + currsize, filesize);
    recvbuf = new char[filesize];
    currsize = 0;

    cout << "recv: file size: " << filesize << endl;
    
    Timer timer;
    timer.start();

    while (recvsize < filesize) {
        currsize = UDT::recv(recver, (char *)(recvbuf + recvsize), filesize - recvsize, 0);

        if (UDT::ERROR == currsize)
        {
            cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
            return -1;
        }

        recvsize += currsize;
    }
    timer.stop();
    std::cout << "time: " << timer << " secs" << '\n';
 

    UDT::close(recver);
    UDT::close(serv);

    std::ofstream binFile("test_out.dat", std::ios::out | std::ios::binary);
    if (!binFile.is_open())
    {
        cout << "recv: ofstream not open" << endl;
        return -1;
    }

    binFile.write(recvbuf, filesize);

    delete filesize_buf;
    delete recvbuf;

    return 0;
}

