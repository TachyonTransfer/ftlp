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
#include <udt.h>

using namespace std;

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
   if ((4 > argc) || (0 == atoi(argv[2])))
   {
      cout << "usage: appclient server_ip server_port result_filename max_bw" << endl;
      return 0;
   }

   struct addrinfo hints, *local, *peer;

   memset(&hints, 0, sizeof(struct addrinfo));

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   // hints.ai_socktype = SOCK_DGRAM;

   if (0 != getaddrinfo(NULL, "9000", &hints, &local))
   {
      cout << "incorrect network address.\n"
           << endl;
      return 0;
   }

   UDTSOCKET client = UDT::socket(
       local->ai_family, local->ai_socktype, local->ai_protocol);

   if (argc == 5)
   {
      if (UDT::ERROR == UDT::setsockopt(client, 0, UDT_MAXBW, new int64_t(atoi(argv[4])), sizeof(int64_t)))
      {
         cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
         return 0;
      }
   }

   if (UDT::ERROR == UDT::setsockopt(client, 0, UDT_SNDBUF, new int(40960000), sizeof(int)))
   {
      cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }
   if (UDT::ERROR == UDT::setsockopt(client, 0, UDT_RCVBUF, new int(40960000), sizeof(int)))
   {
      cout << "setsockopt: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

// Windows UDP issue
// For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
   UDT::setsockopt(client, 0, UDT_MSS, new int(1052), sizeof(int));
#endif

   freeaddrinfo(local);

   string filename(argv[3]);
   if (0 != getaddrinfo(argv[1], argv[2], &hints, &peer))
   {
      cout << "incorrect server/peer address. " << argv[1] << ":" << argv[2] << endl;
      return 0;
   }

   UDT::setTLS(client, 1);

   // connect to the server, implict bind
   if (UDT::ERROR == UDT::connect(client, peer->ai_addr, peer->ai_addrlen))
   {
      cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   freeaddrinfo(peer);

   int size = 100000;
   char *data = new char[size];

   thread_data *my_data = new thread_data();
   my_data->sock = &client;
   my_data->filename = filename;

#ifndef WIN32
   pthread_create(new pthread_t, NULL, monitor, my_data);
#else
   CreateThread(NULL, 0, monitor, &client, 0, NULL);
#endif

   for (int i = 0; i < 1000000; i++)
   {
      int ssize = 0;
      int ss;
      while (ssize < size)
      {
         if (UDT::ERROR == (ss = UDT::send(client, data + ssize, size - ssize, 0)))
         {
            cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
            break;
         }

         ssize += ss;
      }
      // std::cout << "i is " << i << std::endl;
      if (ssize < size)
         break;
   }

   UDT::close(client);
   delete[] data;
   return 0;
}

#ifndef WIN32
void *monitor(void *s)
#else
DWORD WINAPI monitor(LPVOID s)
#endif
{
   thread_data dat = *(thread_data *)s;
   UDTSOCKET u = *dat.sock;

   UDT::TRACEINFO perf;

   std::ofstream myfile;
   myfile.open(dat.filename);

   myfile << "Elapsed Time(ms),Total Packets Sent,Total Number of Lost Packets(Sender),Total Retransmitted Packets(Sender),Total ACKs Sent,Total Recieved ACKs,Total Sent NACKs,Total Recieved NACKs,Total Duration of Send(Idle time exclusive),Packets Sent in Interval,Recieved Packets in Interval,Number of Lost Packets in Interval (Sender),Number of Retransmitted Packets in Interval (Sender),Number of Sent ACKs in Interval,Received ACKs in Interval,Sent NACKs in Interval,Received NACKs in Interval,Send Rate in Interval(Mb/s),Recieve Rate in Interval(Mbps),Sending Time in the Interval (idle time exclusive),Current Packet Sending Period(microseconds),Current Flow Window Size,Current Congestion Window Size,Current Number of Packets in Flight,Current Calculated RTT(ms),Current Estimated Bandwidth,Available Send Buffer Size,Available Recv Buffer Size,sendCurrSeqNo, sendLastAck, recCurrSeqNo, recLastAck, recLastAckAck, lastAckTime, ackSeqNo, cwndSize, pktSndPeriod, ccConWin, lossLengthSizeRec, lossLengthSizeSend, bufferSize, sendDeliveryRate, RTTs, sigmaThresh, minRtt, sd, slowstart, rttvar, unack, bandwidths, sentAdjusted, ambientLoss, lostAdjusted, sendLastDataAck" << endl;

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
             << perf.sendDeliveryRate << "," // the deliveryrate in mbps
             << print_arr(perf.rtts, 7) << ","
             << perf.sigmaThreshold << ","
             << perf.minR << ","
             << perf.sd << ","
             << perf.slowStart << ","
             << perf.rttVar << ","
             << perf.unAckRec << ","
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
