#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef LEGACY_WIN32
#include <wspiapi.h>
#endif
#endif

#include <cstring>
#include "cache.h"
#include "core.h"

using namespace std;

CInfoBlock &CInfoBlock::operator=(const CInfoBlock &obj)
{
   std::copy(obj.m_piIP, obj.m_piIP + 3, m_piIP);
   m_iIPversion = obj.m_iIPversion;
   m_ullTimeStamp = obj.m_ullTimeStamp;
   m_iRTT = obj.m_iRTT;
   m_iBandwidth = obj.m_iBandwidth;
   m_iLossRate = obj.m_iLossRate;
   m_iReorderDistance = obj.m_iReorderDistance;
   m_dInterval = obj.m_dInterval;
   m_dCWnd = obj.m_dCWnd;

   return *this;
}

bool CInfoBlock::operator==(const CInfoBlock &obj)
{
   if (m_iIPversion != obj.m_iIPversion)
      return false;

   else if (m_iIPversion == AF_INET)
      return (m_piIP[0] == obj.m_piIP[0]);

   for (int i = 0; i < 4; ++i)
   {
      if (m_piIP[i] != obj.m_piIP[i])
         return false;
   }

   return true;
}

CInfoBlock *CInfoBlock::clone()
{
   CInfoBlock *obj = new CInfoBlock;

   std::copy(m_piIP, m_piIP + 3, obj->m_piIP);
   obj->m_iIPversion = m_iIPversion;
   obj->m_ullTimeStamp = m_ullTimeStamp;
   obj->m_iRTT = m_iRTT;
   obj->m_iBandwidth = m_iBandwidth;
   obj->m_iLossRate = m_iLossRate;
   obj->m_iReorderDistance = m_iReorderDistance;
   obj->m_dInterval = m_dInterval;
   obj->m_dCWnd = m_dCWnd;

   return obj;
}

int CInfoBlock::getKey()
{
   if (m_iIPversion == AF_INET)
      return m_piIP[0];

   return m_piIP[0] + m_piIP[1] + m_piIP[2] + m_piIP[3];
}

void CInfoBlock::convert(const sockaddr *addr, int ver, uint32_t ip[])
{
   if (ver == AF_INET)
   {
      ip[0] = ((sockaddr_in *)addr)->sin_addr.s_addr;
      ip[1] = ip[2] = ip[3] = 0;
   }
   else
   {
      memcpy((char *)ip, (char *)((sockaddr_in6 *)addr)->sin6_addr.s6_addr, 16);
   }
}
