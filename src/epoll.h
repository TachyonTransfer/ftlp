#ifndef __UDT_EPOLL_H__
#define __UDT_EPOLL_H__

#include <map>
#include <set>
#include "udt.h"

struct CEPollDesc
{
   int m_iID;                          // epoll ID
   std::set<UDTSOCKET> m_sUDTSocksOut; // set of UDT sockets waiting for write events
   std::set<UDTSOCKET> m_sUDTSocksIn;  // set of UDT sockets waiting for read events
   std::set<UDTSOCKET> m_sUDTSocksEx;  // set of UDT sockets waiting for exceptions

   int m_iLocalID;                // local system epoll ID
   std::set<SYSSOCKET> m_sLocals; // set of local (non-UDT) descriptors

   std::set<UDTSOCKET> m_sUDTWrites;  // UDT sockets ready for write
   std::set<UDTSOCKET> m_sUDTReads;   // UDT sockets ready for read
   std::set<UDTSOCKET> m_sUDTExcepts; // UDT sockets with exceptions (connection broken, etc.)
};

class CEPoll
{
   friend class CUDT;
   friend class CRendezvousQueue;

public:
   CEPoll();
   ~CEPoll();

public: // for CUDTUnited API
   // Functionality:
   //    create a new EPoll.
   // Parameters:
   //    None.
   // Returned value:
   //    new EPoll ID if success, otherwise an error number.

   int create();

   // Functionality:
   //    add a UDT socket to an EPoll.
   // Parameters:
   //    0) [in] eid: EPoll ID.
   //    1) [in] u: UDT Socket ID.
   //    2) [in] events: events to watch.
   // Returned value:
   //    0 if success, otherwise an error number.

   int add_usock(const int eid, const UDTSOCKET &u, const int *events = NULL);

   // Functionality:
   //    add a system socket to an EPoll.
   // Parameters:
   //    0) [in] eid: EPoll ID.
   //    1) [in] s: system Socket ID.
   //    2) [in] events: events to watch.
   // Returned value:
   //    0 if success, otherwise an error number.

   int add_ssock(const int eid, const SYSSOCKET &s, const int *events = NULL);

   // Functionality:
   //    remove a UDT socket event from an EPoll; socket will be removed if no events to watch
   // Parameters:
   //    0) [in] eid: EPoll ID.
   //    1) [in] u: UDT socket ID.
   // Returned value:
   //    0 if success, otherwise an error number.

   int remove_usock(const int eid, const UDTSOCKET &u);

   // Functionality:
   //    remove a system socket event from an EPoll; socket will be removed if no events to watch
   // Parameters:
   //    0) [in] eid: EPoll ID.
   //    1) [in] s: system socket ID.
   // Returned value:
   //    0 if success, otherwise an error number.

   int remove_ssock(const int eid, const SYSSOCKET &s);

   // Functionality:
   //    wait for EPoll events or timeout.
   // Parameters:
   //    0) [in] eid: EPoll ID.
   //    1) [out] readfds: UDT sockets available for reading.
   //    2) [out] writefds: UDT sockets available for writing.
   //    3) [in] msTimeOut: timeout threshold, in milliseconds.
   //    4) [out] lrfds: system file descriptors for reading.
   //    5) [out] lwfds: system file descriptors for writing.
   // Returned value:
   //    number of sockets available for IO.

   int wait(const int eid, std::set<UDTSOCKET> *readfds, std::set<UDTSOCKET> *writefds, int64_t msTimeOut, std::set<SYSSOCKET> *lrfds, std::set<SYSSOCKET> *lwfds);

   // Functionality:
   //    close and release an EPoll.
   // Parameters:
   //    0) [in] eid: EPoll ID.
   // Returned value:
   //    0 if success, otherwise an error number.

   int release(const int eid);

public: // for CUDT to acknowledge IO status
   // Functionality:
   //    Update events available for a UDT socket.
   // Parameters:
   //    0) [in] uid: UDT socket ID.
   //    1) [in] eids: EPoll IDs to be set
   //    1) [in] events: Combination of events to update
   //    1) [in] enable: true -> enable, otherwise disable
   // Returned value:
   //    0 if success, otherwise an error number

   int update_events(const UDTSOCKET &uid, std::set<int> &eids, int events, bool enable);

private:
   int m_iIDSeed; // seed to generate a new ID
   pthread_mutex_t m_SeedLock;

   std::map<int, CEPollDesc> m_mPolls; // all epolls
   pthread_mutex_t m_EPollLock;
};

#endif
