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

#ifndef WIN32
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#endif
#include <iostream>
#include <fstream>
#include <udt.h>

using namespace std;

#ifndef WIN32
void *recvdata(void *);
#else
DWORD WINAPI recvdata(LPVOID);
#endif

#ifndef WIN32
void *monitor(void *);
#else
DWORD WINAPI monitor(LPVOID);
#endif

struct thread_data
{
   UDTSOCKET *sock;
   string filename;
};

std::string print_arr(int *arr, int length)
{
   std::string str;
   for (int i = 0; i < length; i++)
   {
      str += std::to_string(arr[i]) + " ";
   }
   return str;
}

std::string print_arr(double *arr, int length)
{
   std::string str;
   for (int i = 0; i < length; i++)
   {
      str += std::to_string(arr[i]) + " ";
   }
   return str;
}

int main(int argc, char *argv[])
{
   if ((3 != argc) || (0 == atoi(argv[1])))
   {
      cout << "usage: appserver server_port result_filename" << endl;
      return 0;
   }

   addrinfo hints;
   addrinfo *res;

   memset(&hints, 0, sizeof(struct addrinfo));

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   // hints.ai_socktype = SOCK_DGRAM;

   string service(argv[1]);
   string filename(argv[2]);

   if (0 != getaddrinfo(NULL, service.c_str(), &hints, &res))
   {
      cout << "illegal port number or port is busy.\n"
           << endl;
      return 0;
   }

   UDTSOCKET serv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

   if (UDT::ERROR == UDT::setsockopt(serv, 0, UDT_SNDBUF, new int(40960000), sizeof(int)))
   {
      cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   if (UDT::ERROR == UDT::setsockopt(serv, 0, UDT_RCVBUF, new int(40960000), sizeof(int)))
   {
      cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen))
   {
      cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   freeaddrinfo(res);

   cout << "server is ready at port: " << service << endl;

   if (UDT::ERROR == UDT::listen(serv, 10))
   {
      cout << "listen: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   sockaddr_storage clientaddr;
   int addrlen = sizeof(clientaddr);

   UDTSOCKET recver;

   while (true)
   {
      if (UDT::INVALID_SOCK == (recver = UDT::accept(serv, (sockaddr *)&clientaddr, &addrlen)))
      {
         cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
         return 0;
      }

      thread_data *my_data = new thread_data();
      my_data->sock = &recver;
      my_data->filename = filename;

      char clienthost[NI_MAXHOST];
      char clientservice[NI_MAXSERV];
      getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST | NI_NUMERICSERV);
      cout << "new connection: " << clienthost << ":" << clientservice << endl;

#ifndef WIN32
      pthread_t rcvthread;
      pthread_create(&rcvthread, NULL, recvdata, new UDTSOCKET(recver));
      pthread_create(new pthread_t, NULL, monitor, my_data);
      pthread_detach(rcvthread);
#else
      CreateThread(NULL, 0, recvdata, new UDTSOCKET(recver), 0, NULL);
      CreateThread(NULL, 0, monitor, &recver, 0, NULL);
#endif
   }

   UDT::close(serv);

   return 0;
}

#ifndef WIN32
void *recvdata(void *usocket)
#else
DWORD WINAPI recvdata(LPVOID usocket)
#endif
{
   UDTSOCKET recver = *(UDTSOCKET *)usocket;
   delete (UDTSOCKET *)usocket;

   char *data;
   int size = 100000;
   data = new char[size];

   while (true)
   {
      int rsize = 0;
      int rs;
      while (rsize < size)
      {
         int rcv_size;
         int var_size = sizeof(int);
         UDT::getsockopt(recver, 0, UDT_RCVDATA, &rcv_size, &var_size);
         if (UDT::ERROR == (rs = UDT::recv(recver, data + rsize, size - rsize, 0)))
         {
            cout << "recv:" << UDT::getlasterror().getErrorMessage() << endl;
            break;
         }

         rsize += rs;
      }

      if (rsize < size)
         break;
   }

   delete[] data;

   UDT::close(recver);

#ifndef WIN32
   return NULL;
#else
   return 0;
#endif
}

#ifndef WIN32
void *monitor(void *s)
#else
DWORD WINAPI monitor(LPVOID s)
#endif
{
   thread_data dat = *(thread_data *)s;
   UDTSOCKET u = *dat.sock;
   // string filename = dat.filename;

   UDT::TRACEINFO perf;

   std::ofstream myfile;
   myfile.open(dat.filename);
   myfile << "Elapsed Time(ms),Total Packets Sent,Total Number of Lost Packets(Sender),Total Retransmitted Packets(Sender),Total ACKs Sent,Total Recieved ACKs,Total Sent NACKs,Total Recieved NACKs,Total Duration of Send(Idle time exclusive),Packets Sent in Interval,Recieved Packets in Interval,Number of Lost Packets in Interval (Sender),Number of Retransmitted Packets in Interval (Sender),Number of Sent ACKs in Interval,Received ACKs in Interval,Sent NACKs in Interval,Received NACKs in Interval,Send Rate in Interval(Mb/s),Recieve Rate in Interval(Mbps),Sending Time in the Interval (idle time exclusive),Current Packet Sending Period(microseconds),Current Flow Window Size,Current Congestion Window Size,Current Number of Packets in Flight,Current Calculated RTT(ms),Current Estimated Bandwidth,Available Send Buffer Size,Available Recv Buffer Size,sendCurrSeqNo, sendLastAck, recCurrSeqNo, recLastAck, recLastAckAck, lastAckTime, ackSeqNo, cwndSize, pktSndPeriod, ccConWin, lossLengthSizeRec, lossLengthSizeSend, bufferSize, sendDeliveryRate, RTTs, sigmaThresh, minRtt, sd, slowstart, rttvar, unack, bandwidths, sendLastDataAck" << endl;

   timespec ts;
   ts.tv_sec = 0;
   ts.tv_nsec = 50000000;

   while (true)
   {
#ifndef WIN32
      nanosleep(&ts, NULL);
#else
      Sleep(1000);
#endif

      if (UDT::ERROR == UDT::perfmon(u, &perf))
      {
         cout << "perfmon: " << UDT::getlasterror().getErrorMessage() << endl;
         break;
      }

      // global measurements
      myfile << perf.msTimeStamp << ","        // time since the UDT entity is started, in milliseconds
             << perf.pktSentTotal << ","       // total number of sent data packets, including retransmissions
             << perf.pktRecvTotal << ","       // total number of received data packets, including retransmissions
             << perf.pktSndLossTotal << ","    // total number of lost packets (sender side)
             << perf.pktRetransTotal << ","    // total number of retransmitted packets
             << perf.pktSentACKTotal << ","    // total number of sent ACK packets
             << perf.pktRecvACKTotal << ","    // total number of received ACK packets
             << perf.pktSentNAKTotal << ","    // total number of sent NAK packets
             << perf.pktRecvNAKTotal << ","    // total number of received NAK packets
             << perf.usSndDurationTotal << "," // total time duration when UDT is sending data (idle time exclusive)

             // local measurements
             << perf.pktSent << ","       // number of sent data packets, including retransmissions
             << perf.pktRecv << ","       // number of received packets
             << perf.pktSndLoss << ","    // number of lost packets (sender side)
             << perf.pktRetrans << ","    // number of retransmitted packets
             << perf.pktSentACK << ","    // number of sent ACK packets
             << perf.pktRecvACK << ","    // number of received ACK packets
             << perf.pktSentNAK << ","    // number of sent NAK packets
             << perf.pktRecvNAK << ","    // number of received NAK packets
             << perf.mbpsSendRate << ","  // sending rate in Mb/s
             << perf.mbpsRecvRate << ","  // receiving rate in Mb/s
             << perf.usSndDuration << "," // busy sending time (i.e., idle time exclusive)

             // instant measurements
             << perf.usPktSndPeriod << ","      // packet sending period, in microseconds
             << perf.pktFlowWindow << ","       // flow window size, in number of packets
             << perf.pktCongestionWindow << "," // congestion window size, in number of packets
             << perf.pktFlightSize << ","       // number of packets on flight
             << perf.msRTT << ","               // RTT, in milliseconds
             << perf.mbpsBandwidth << ","       // estimated bandwidth, in Mb/s
             << perf.byteAvailSndBuf << ","     // available UDT sender buffer size
             << perf.byteAvailRcvBuf << ","     // available UDT receiver buffer size
             << perf.sendCurrSeqNo << ","       // curr seq no
             << perf.sendLastAck << ","         // sendlastack
             << perf.recCurrSeqNo << ","        // recCurrSeqNo
             << perf.recLastAck << ","          // available UDT receiver buffer size
             << perf.recLastAckAck << ","       // available UDT receiver buffer size
             << perf.lastAckTime << ","         // available UDT receiver buffer size
             << perf.ackSeqNo << ","
             << perf.cWndSize << ","
             << perf.pktSndPeriod << ","
             << perf.ccConWin << ","
             << perf.lossLengthSizeRec << ","
             << perf.lossLengthSizeSend << ","
             << perf.bufferSize << ","
             << print_arr(perf.bandwidths, 64) << ","
             << perf.sendLastDataAck << endl
             << flush;
   }

   myfile.close();
#ifndef WIN32
   return NULL;
#else
   return 0;
#endif
}
