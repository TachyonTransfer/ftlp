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

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <udt.h>

/* This routine returns the size of the file it is called with. */

static unsigned
get_file_size(const char *file_name)
{
    struct stat sb;
    if (stat(file_name, &sb) != 0)
    {
        fprintf(stderr, "'stat' failed for '%s': %s.\n",
                file_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return sb.st_size;
}

/* This routine reads the entire file into memory. */

static void
read_file(const char *file_name, char **out_buf, long *out_size)
{
    FILE *f;
    long bytes_read;
    int status;

    *out_size = get_file_size(file_name);
    *out_buf = (char *)malloc(*out_size);
    
    if (!*out_buf)
    {
        fprintf(stderr, "Not enough memory.\n");
        exit(EXIT_FAILURE);
    }

    f = fopen(file_name, "r");
    if (!f)
    {
        fprintf(stderr, "Could not open '%s': %s.\n", file_name,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    bytes_read = fread(*out_buf, sizeof(char), *out_size, f);
    if (bytes_read != *out_size)
    {
        fprintf(stderr, "Short read of '%s': expected %ld bytes "
                        "but got %ld: %s.\n",
                file_name, *out_size, bytes_read,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    status = fclose(f);
    if (status != 0)
    {
        fprintf(stderr, "Error closing '%s': %s.\n", file_name,
                strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    using namespace std;
    using namespace UDT;

    UDTSOCKET client = UDT::socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9000);
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    memset(&(serv_addr.sin_zero), '\0', 8);

    // connect to the server, implict bind
    if (UDT::ERROR == UDT::connect(client, (sockaddr *)&serv_addr, sizeof(serv_addr)))
    {
        cout << "connect: " << UDT::getlasterror().getErrorMessage();
        return -1;
    }

    char *file_contents;
    string filesize_str;
    long filesize = 0;
    long currsize = 0;
    long sentsize = 0; 

    read_file("test.dat", &file_contents, &filesize);
    filesize_str = to_string(filesize);

    currsize = UDT::send(
        client, 
        (const char *)(filesize_str.c_str()), 
        filesize_str.length(),
        0);
    if (UDT::ERROR == currsize) 
    {
        cout << "send filesize: " << UDT::getlasterror().getErrorMessage();
        return -1;
    }
    
    currsize = 0;

    while (sentsize < filesize) {
        currsize = UDT::send(
            client, (char *)(file_contents + sentsize), filesize - sentsize, 0);

        if (UDT::ERROR == currsize)
        {
            cout << "send file: " << UDT::getlasterror().getErrorMessage();
            return -1;
        }

        sentsize += currsize;
    }

    delete file_contents;

    UDT::close(client);

    return 0;
}

