#ifndef __UDT_CHANNEL_H__
#define __UDT_CHANNEL_H__

#include "udt.h"
#include "packet.h"

class CChannel
{
public:
   CChannel();
   CChannel(int version);
   ~CChannel();

   // Functionality:
   //    Open a UDP channel.
   // Parameters:
   //    0) [in] addr: The local address that UDP will use.
   // Returned value:
   //    None.

   void open(const sockaddr *addr = NULL);

   // Functionality:
   //    Open a UDP channel based on an existing UDP socket.
   // Parameters:
   //    0) [in] udpsock: UDP socket descriptor.
   // Returned value:
   //    None.

   void open(UDPSOCKET udpsock);

   // Functionality:
   //    Disconnect and close the UDP entity.
   // Parameters:
   //    None.
   // Returned value:
   //    None.

   void close() const;

   // Functionality:
   //    Get the UDP sending buffer size.
   // Parameters:
   //    None.
   // Returned value:
   //    Current UDP sending buffer size.

   int getSndBufSize();

   // Functionality:
   //    Get the UDP receiving buffer size.
   // Parameters:
   //    None.
   // Returned value:
   //    Current UDP receiving buffer size.

   int getRcvBufSize();

   // Functionality:
   //    Set the UDP sending buffer size.
   // Parameters:
   //    0) [in] size: expected UDP sending buffer size.
   // Returned value:
   //    None.

   void setSndBufSize(int size);

   // Functionality:
   //    Set the UDP receiving buffer size.
   // Parameters:
   //    0) [in] size: expected UDP receiving buffer size.
   // Returned value:
   //    None.

   void setRcvBufSize(int size);

   // Functionality:
   //    Query the socket address that the channel is using.
   // Parameters:
   //    0) [out] addr: pointer to store the returned socket address.
   // Returned value:
   //    None.

   void getSockAddr(sockaddr *addr) const;

   // Functionality:
   //    Query the peer side socket address that the channel is connect to.
   // Parameters:
   //    0) [out] addr: pointer to store the returned socket address.
   // Returned value:
   //    None.

   void getPeerAddr(sockaddr *addr) const;

   // Functionality:
   //    Send a packet to the given address.
   // Parameters:
   //    0) [in] addr: pointer to the destination address.
   //    1) [in] packet: reference to a CPacket entity.
   // Returned value:
   //    Actual size of data sent.

   int sendto(const sockaddr *addr, CPacket &packet) const;

   // Functionality:
   //    Receive a packet from the channel and record the source address.
   // Parameters:
   //    0) [in] addr: pointer to the source address.
   //    1) [in] packet: reference to a CPacket entity.
   // Returned value:
   //    Actual size of data received.

   int recvfrom(sockaddr *addr, CPacket &packet) const;

private:
   void setUDPSockOpt();

private:
   int m_iIPversion;    // IP version
   int m_iSockAddrSize; // socket address structure size (pre-defined to avoid run-time test)

   UDPSOCKET m_iSocket; // socket descriptor

   int m_iSndBufSize; // UDP sending buffer size
   int m_iRcvBufSize; // UDP receiving buffer size
};

#endif
