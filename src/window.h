#ifndef __UDT_WINDOW_H__
#define __UDT_WINDOW_H__

#ifndef WIN32
#include <sys/time.h>
#include <time.h>
#endif
#include "udt.h"

class CACKWindow
{
public:
   CACKWindow(int size = 1024);
   ~CACKWindow();

   // Functionality:
   //    Write an ACK record into the window.
   // Parameters:
   //    0) [in] seq: ACK seq. no.
   //    1) [in] ack: DATA ACK no.
   // Returned value:
   //    None.

   void store(int32_t seq, int32_t ack);

   // Functionality:
   //    Search the ACK-2 "seq" in the window, find out the DATA "ack" and caluclate RTT .
   // Parameters:
   //    0) [in] seq: ACK-2 seq. no.
   //    1) [out] ack: the DATA ACK no. that matches the ACK-2 no.
   // Returned value:
   //    RTT.

   int acknowledge(int32_t seq, int32_t &ack);

private:
   int32_t *m_piACKSeqNo;  // Seq. No. for the ACK packet
   int32_t *m_piACK;       // Data Seq. No. carried by the ACK packet
   uint64_t *m_pTimeStamp; // The timestamp when the ACK was sent

   int m_iSize; // Size of the ACK history window
   int m_iHead; // Pointer to the lastest ACK record
   int m_iTail; // Pointer to the oldest ACK record

private:
   CACKWindow(const CACKWindow &);
   CACKWindow &operator=(const CACKWindow &);
};

////////////////////////////////////////////////////////////////////////////////

class CPktTimeWindow
{
public:
   CPktTimeWindow(int asize = 16, int psize = 16);
   ~CPktTimeWindow();

   // Functionality:
   //    read the minimum packet sending interval.
   // Parameters:
   //    None.
   // Returned value:
   //    minimum packet sending interval (microseconds).

   int getMinPktSndInt() const;

   // Functionality:
   //    Calculate the packes arrival speed.
   // Parameters:
   //    None.
   // Returned value:
   //    Packet arrival speed (packets per second).

   int getPktRcvSpeed() const;

   // Functionality:
   //    Estimate the bandwidth.
   // Parameters:
   //    None.
   // Returned value:
   //    Estimated bandwidth (packets per second).

   int getBandwidth() const;

   // Functionality:
   //    Record time information of a packet sending.
   // Parameters:
   //    0) currtime: timestamp of the packet sending.
   // Returned value:
   //    None.

   void onPktSent(int currtime);

   // Functionality:
   //    Record time information of an arrived packet.
   // Parameters:
   //    None.
   // Returned value:
   //    None.

   void onPktArrival();

   // Functionality:
   //    Record the arrival time of the first probing packet.
   // Parameters:
   //    None.
   // Returned value:
   //    None.

   void probe1Arrival();

   // Functionality:
   //    Record the arrival time of the second probing packet and the interval between packet pairs.
   // Parameters:
   //    None.
   // Returned value:
   //    None.

   void probe2Arrival();

private:
   int m_iAWSize;      // size of the packet arrival history window
   int *m_piPktWindow; // packet information window
   int *m_piPktReplica;
   int m_iPktWindowPtr; // position pointer of the packet info. window.

   int m_iPWSize;        // size of probe history window size
   int *m_piProbeWindow; // record inter-packet time for probing packet pairs
   int *m_piProbeReplica;
   int m_iProbeWindowPtr; // position pointer to the probing window

   int m_iLastSentTime; // last packet sending time
   int m_iMinPktSndInt; // Minimum packet sending interval

   uint64_t m_LastArrTime; // last packet arrival time
   uint64_t m_CurrArrTime; // current packet arrival time
   uint64_t m_ProbeTime;   // arrival time of the first probing packet

private:
   CPktTimeWindow(const CPktTimeWindow &);
   CPktTimeWindow &operator=(const CPktTimeWindow &);
};

#endif
