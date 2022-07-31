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

#include "core.h"
#include "ccc.h"
#include <cmath>
#include <cstring>
#include <float.h>
#include <iostream>

CircularBuffer::CircularBuffer(int bufferSize) : m_iBufferSize(bufferSize),
                                                 m_iLength(0),
                                                 m_iIndex(0),
                                                 lastValue(NULL)
{
   m_dBuffer = new double[m_iBufferSize];
}

CircularBuffer::~CircularBuffer()
{
   delete[] m_dBuffer;
}

double CircularBuffer::append(double value)
{
   double poppedValue = NULL;
   if (m_iLength == m_iBufferSize)
   {
      poppedValue = m_dBuffer[m_iIndex];
   }
   else
   {
      m_iLength++;
   }
   m_dBuffer[m_iIndex] = value;
   m_iIndex = (m_iIndex + 1) % m_iBufferSize;
   lastValue = poppedValue;
   return poppedValue;
}

bool CircularBuffer::check(double threshold)
{
   for (int i = 0; i < m_iLength; i++)
   {
      if (m_dBuffer[i] < threshold)
      {
         return false;
      }
   }
   return true;
}

RunningStats::RunningStats()
{
   Clear();
}

void RunningStats::Clear()
{
   m_n = 0;
}

void RunningStats::Push(double x)
{
   m_n++;

   // See Knuth TAOCP vol 2, 3rd edition, page 232
   if (m_n == 1)
   {
      m_oldM = m_newM = x;
      m_oldS = 0.0;
   }
   else
   {
      m_newM = m_oldM + (x - m_oldM) / m_n;
      m_newS = m_oldS + (x - m_oldM) * (x - m_newM);

      // set up for next iteration
      m_oldM = m_newM;
      m_oldS = m_newS;
   }
}

int RunningStats::NumDataValues() const
{
   return m_n;
}

double RunningStats::Mean() const
{
   return (m_n > 0) ? m_newM : 0.0;
}

double RunningStats::Variance() const
{
   return ((m_n > 1) ? m_newS / (m_n - 1) : 0.0);
}

double RunningStats::StandardDeviation() const
{
   return sqrt(Variance());
}

CCC::CCC() : m_iSYNInterval(CUDT::m_iSYNInterval),
             m_dPktSndPeriod(1.0),
             m_dCWndSize(16.0),
             m_iBandwidth(),
             m_maxShare(),
             m_maxShareReal(),
             m_maxShareUpdateTime(),
             m_lineVsShareThresh(80000000),
             m_dMaxCWndSize(),
             m_iMSS(),
             m_iSndCurrSeqNo(),
             m_iRcvRate(),
             m_iRTT(),
             m_pcParam(NULL),
             m_iPSize(0),
             m_UDT(),
             m_iACKPeriod(0),
             m_iACKInterval(0),
             m_bUserDefinedRTO(false),
             m_iRTO(-1),
             m_PerfInfo(),
             m_iAvgNAKNum(), // lifted up
             m_iDecCount(),  // lifted up
             m_iNAKCount(),  // lifeted up
             m_bSlowStart(), // lifted up
             congBool(),
             m_LastDecreaseTime(),
             m_iDecCountTwo(),
             minRtt(INFINITY),
             minRttWindow(1000),
             minCongRtt(0),
             rttDif(0),
             debugWelfordSd(0),
             rttCounter(1),
             ambientLossRate(pow(10, -6)),
             lossCount(0),
             sentPackets(100),
             m_congestionThresh(1.25),
             m_dSigmaThreshold(2) // Adjustable
{
   m_RTTs = new CircularBuffer(2);
}

CCC::~CCC()
{
   delete[] m_pcParam;
}

void CCC::setACKTimer(int msINT)
{
   m_iACKPeriod = msINT > m_iSYNInterval ? m_iSYNInterval : msINT;
}

void CCC::setACKInterval(int pktINT)
{
   m_iACKInterval = pktINT;
}

void CCC::setRTO(int usRTO)
{
   m_bUserDefinedRTO = true;
   m_iRTO = usRTO;
}

void CCC::sendCustomMsg(CPacket &pkt) const
{
   CUDT *u = CUDT::getUDTHandle(m_UDT);

   if (NULL != u)
   {
      pkt.m_iID = u->m_PeerID;
      u->m_pSndQueue->sendto(u->m_pPeerAddr, pkt);
   }
}

const CPerfMon *CCC::getPerfInfo()
{
   try
   {
      CUDT *u = CUDT::getUDTHandle(m_UDT);
      if (NULL != u)
         u->sample(&m_PerfInfo, false);
   }
   catch (...)
   {
      return NULL;
   }

   return &m_PerfInfo;
}

void CCC::setMSS(int mss)
{
   m_iMSS = mss;
}

void CCC::setBandwidth(int bw)
{
   m_iBandwidth = bw;
   if (rttCounter < minRttWindow)
   {
   }
   else
   {
   }
}

void CCC::setSndCurrSeqNo(int32_t seqno)
{
   m_iSndCurrSeqNo = seqno;
}

void CCC::setRcvRate(int rcvrate)
{
   m_iRcvRate = rcvrate;
}

void CCC::setMaxCWndSize(int cwnd)
{
   m_dMaxCWndSize = cwnd;
}

void CCC::setVar(int rttVar)
{
   m_iRTTVar = rttVar;
}

void CCC::setRTT(int rtt)
{
   m_iRTT = rtt;
   if (m_bSlowStart)
   {
      minRtt = m_iRTT;
      return;
   }

   if (rttCounter < minRttWindow)
   {
      rttCounter = rttCounter + 1;
      minRtt = fmin(minRtt, m_iRTT);
      rttDif = m_iRTT - minRtt;
      m_RTTs->append(m_iRTTVar > 0 ? (rttDif / m_iRTTVar) : 0);
   }
   else
   {
      rttDif = m_iRTT - minRtt;
      if (congBool)
      {
      }
      m_RTTs->append(m_iRTTVar > 0 ? (rttDif / m_iRTTVar) : 0);
      rttCounter = 1;
      minRtt = rtt;
   }
}

void CCC::setLossRate(double lossRate)
{
   sentPackets = lossRate;
   ambientLossRate = (double)lossCount / (double)sentPackets;
   if (ambientLossRate < pow(10, -5))
   {
      ambientLossRate = pow(10, -5);
   }
}

void CCC::setUserParam(const char *param, int size)
{
   delete[] m_pcParam;
   m_pcParam = new char[size];
   memcpy(m_pcParam, param, size);
   m_iPSize = size;
}

//
CUDTCC::CUDTCC() : m_iRCInterval(),
                   m_LastRCTime(),
                   m_iLastAck(),
                   m_bLoss(),
                   m_iLastDecSeq(),
                   m_dLastDecPeriod(),
                   m_iDecRandom()
{
}

void CUDTCC::init()
{
   m_iRCInterval = m_iSYNInterval;
   m_LastRCTime = CTimer::getTime();
   setACKTimer(m_iRCInterval);

   m_bSlowStart = true;
   congBool = false;
   m_iLastAck = m_iSndCurrSeqNo;
   m_bLoss = false;
   m_iLastDecSeq = CSeqNo::decseq(m_iLastAck);
   m_dLastDecPeriod = 1;
   m_iAvgNAKNum = 0;
   m_iNAKCount = 0;
   m_iDecRandom = 1;

   m_dCWndSize = 16;
   m_dPktSndPeriod = 1;
}

void CUDTCC::onACK(int32_t ack)
{

   int64_t B = 0;
   double inc = 0;
   // Note: 1/24/2012
   // The minimum increase parameter is increased from "1.0 / m_iMSS" to 0.01
   // because the original was too small and caused sending rate to stay at low level
   // for long time.
   const double min_inc = 0.02;

   uint64_t currtime = CTimer::getTime();
   if (currtime - m_LastRCTime < (uint64_t)m_iRCInterval)
      return;

   m_LastRCTime = currtime;

   if (m_bSlowStart)
   {
      m_dCWndSize += CSeqNo::seqlen(m_iLastAck, ack);
      m_iLastAck = ack;

      if (m_dCWndSize > m_dMaxCWndSize)
      {
         m_bSlowStart = false;
         if (m_iRcvRate > 0)
            m_dPktSndPeriod = 1000000.0 / m_iRcvRate;
         else
            m_dPktSndPeriod = (m_iRTT + m_iRCInterval) / m_dCWndSize;
      }

      m_maxShare = m_iBandwidth * (m_iMSS)*8.0;
   }
   // During Slow Start, no rate increase
   if (m_bSlowStart)
      return;

   m_dCWndSize = m_iRcvRate / 1000000.0 * (m_iRTT + m_iRCInterval) + 16;

   // In congestion state currently
   if (congBool)
   {

      if (m_RTTs->lastValue == NULL)
      {
         // this should not be happening
         return;
      }
      // check if we should not be in congestion
      if (m_RTTs->m_dBuffer[m_RTTs->m_iIndex] < m_RTTs->lastValue)
      {
         if (m_iRTT < minRtt * 1.015)
         {
            if ((m_RTTs->m_dBuffer[m_RTTs->m_iIndex] < m_dSigmaThreshold) && (m_RTTs->lastValue < m_dSigmaThreshold))
            {
               congBool = false;
               // add check for last decrease
               int64_t tempTime = CTimer::getTime();
               if (tempTime >= m_LastDecreaseTime + 4 * m_iRTT)
               {
                  onNoCongestion();
               }
               else
               {
                  onCongestion(m_RTTs->m_dBuffer[m_RTTs->m_iIndex]);
               }
               // test
               // onCongestion(m_RTTs->m_dBuffer[m_RTTs->m_iIndex]);
               return;
            }
            // do nothing in this case
            return;
         }
         // do nothing in this case
         onCongestion(m_RTTs->m_dBuffer[m_RTTs->m_iIndex]);
         return;
      }
      else
      {
         onCongestion(m_RTTs->m_dBuffer[m_RTTs->m_iIndex]);
         return;
      }
   }
   // In no congestion state
   else
   {
      // check if we should be in congestion
      if (m_iRTT > minRtt * 1.12)
      {
         congBool = true;
         minCongRtt = m_iRTT - minRtt;
         onCongestion(m_RTTs->m_dBuffer[m_RTTs->m_iIndex]);
         return;
      }
      else
      {
         onNoCongestion();
         return;
      }
   }

   if (m_bLoss)
   {
      m_bLoss = false;
      return;
   }

   B = (int64_t)(m_iBandwidth - 1000000.0 / m_dPktSndPeriod);
   if ((m_dPktSndPeriod > m_dLastDecPeriod) && ((m_iBandwidth / 9) < B))
      B = m_iBandwidth / 9;
   if (B <= 0)
      inc = min_inc;
   else
   {
      // inc = max(10 ^ ceil(log10( B * MSS * 8 ) * Beta / MSS, 1/MSS)
      // Beta = 1.5 * 10^(-6)

      inc = pow(10.0, ceil(log10(B * m_iMSS * 8.0))) * 0.0000015 / m_iMSS;

      if (inc < min_inc)
         inc = min_inc;
   }

   m_dPktSndPeriod = (m_dPktSndPeriod * m_iRCInterval) / (m_dPktSndPeriod * inc + m_iRCInterval);
}

void CUDTCC::onCongestion(double congestionSize)
{
   int64_t B = 0;
   double inc = 0;
   const double min_inc = 0.005;
   B = (int64_t)(m_iBandwidth - 1000000.0 / m_dPktSndPeriod);

   double Bprime = m_iBandwidth * (m_iMSS)*8.0;
   double currentSendingRate = (1000000.0 / m_dPktSndPeriod) * (m_iMSS)*8.0;
   double availableBandwidth = Bprime - currentSendingRate;

   if (availableBandwidth <= 1000)
      availableBandwidth = 0;
   // want to check if we need to update
   else
   {
      // small deduction to not overshoot later
      availableBandwidth = availableBandwidth - 2;
   }

   // want to check if we need to update
   double incPrime = availableBandwidth / 8;
   double mathisConstant = (double)m_iMSS * sqrt(1.5) * 8.0;
   double incPrimeTwo = mathisConstant / (((double)m_iRTT * pow(10, -6) * sqrt(ambientLossRate)));

   if (incPrimeTwo > Bprime)
   {
      incPrimeTwo = Bprime;
   }
   incPrimeTwo = Bprime;
   incPrimeTwo = incPrimeTwo * ((double)(m_iRTT - minRtt) * pow(10, -6));
   if (incPrime < min_inc)
   {
      incPrime = min_inc;
   }

   double additionalTerm = (double)minCongRtt / (double)m_iRTT;
   double coefficient = (minRtt / (double)m_iRTT);
   if (coefficient > 1)
   {
      coefficient = 1;
   }

   double newSendingRate = (0.5 * currentSendingRate * coefficient) + (0.5 * (currentSendingRate + incPrimeTwo));

   m_dPktSndPeriod = 1000000.0 / (newSendingRate / ((m_iMSS)*8.0));
   return;

   if (((m_iBandwidth / 9) < B))
      B = m_iBandwidth / 9;
   if (B <= 0)
      inc = min_inc;
   else
   {
      inc = pow(10.0, ceil(log10(B * m_iMSS * 8.0))) * 0.0000015 / m_iMSS;

      if (inc < min_inc)
      {
         inc = min_inc;
      }
   }
   m_dPktSndPeriod = (m_dPktSndPeriod * m_iRCInterval) / (m_dPktSndPeriod * inc + m_iRCInterval);
}

void CUDTCC::onNoCongestion()
{
   int64_t B = 0;
   double inc = 0;
   const double min_inc = 800000;
   B = (int64_t)(m_iBandwidth - 1000000.0 / m_dPktSndPeriod);

   B = m_iBandwidth / 10;

   double Bprime = m_iBandwidth * (m_iMSS)*8.0;
   double currentSendingRate = (1000000.0 / m_dPktSndPeriod) * (m_iMSS)*8.0;
   double availableBandwidth = Bprime - currentSendingRate;

   m_maxShare = getShareOfBandwidth(Bprime, currentSendingRate, min_inc);

   // double availableShare = m_maxShare - currentSendingRate;
   double availableShare = availableBandwidth;

   if (availableBandwidth <= 1000)
      availableBandwidth = 0;
   // want to check if we need to update
   else
   {
      // small deduction to not overshoot later
      availableBandwidth = availableBandwidth - 2;
   }
   if (availableShare <= 1000)
      availableShare = 0;
   // want to check if we need to update
   double incPrime = availableBandwidth / 4;
   if (availableShare < availableBandwidth)
   {
      incPrime = availableShare / 8;
   }
   if (incPrime < min_inc)
   {
      incPrime = min_inc;
   }
   double newSendingRate = (0.5 * currentSendingRate * (minRtt / (double)m_iRTT)) + (0.5 * currentSendingRate + incPrime);

   m_dPktSndPeriod = 1000000.0 / (newSendingRate / ((m_iMSS)*8.0));
}

double CUDTCC::getShareOfBandwidth(double Bprime, double currentSendingRate, double min_inc)
{
   // check here for current sending rate, rtt and availshare
   // idea is maxShare should increase incrementally if we reach last maxShare and see no increase in rtt
   // we can only increase once per rtt
   if (m_maxShare - currentSendingRate <= m_lineVsShareThresh)
   {
      uint64_t tempTime = CTimer::getTime();
      if (tempTime > m_maxShareUpdateTime + (m_iRTT * 4))
      {
         // should account for small rtt with or > minrtt + 1millisecond here
         if (m_iRTT < minRtt * 1.01)
         {
            if ((m_RTTs->m_dBuffer[m_RTTs->m_iIndex] < m_dSigmaThreshold) && (m_RTTs->lastValue < m_dSigmaThreshold))
            {
               if (Bprime >= m_maxShare)
               {
                  // m_maxShare = (0.5 * m_maxShare * (minRtt / (double) m_iRTT)) + (0.5 *  m_maxShare + ((Bprime - m_maxShare)/ 10));
                  m_maxShare = (0.5 * m_maxShare) + (0.5 * m_maxShare + ((Bprime - m_maxShare) / 10));
                  if (Bprime <= m_maxShare)
                  {
                     m_maxShare = Bprime;
                  }
                  m_maxShareUpdateTime = tempTime;
               }
               else
               {
                  // shouldn't really get here;
                  m_maxShare = Bprime;
               }
            }
         }
         else
         {
            if (m_iRTT > minRtt * 1.07)
            {
               if (Bprime >= m_maxShare)
               {
                  m_maxShare = (0.5 * m_maxShare) + (0.5 * m_maxShare - ((Bprime - m_maxShare) / 10));
                  if (m_maxShare < m_maxShareReal)
                  {
                     m_maxShare = m_maxShareReal;
                  }
                  m_maxShareUpdateTime = tempTime;
               }
               else
               {
                  // shouldn't really get here;
                  m_maxShare = Bprime;
                  m_maxShare = m_maxShare - min_inc;
                  if (m_maxShare < m_maxShareReal)
                  {
                     m_maxShare = m_maxShareReal;
                  }
                  m_maxShareUpdateTime = tempTime;
               }
            }
         }
      }
   }
   return m_maxShare;
}

void CUDTCC::onLoss(const int32_t *losslist, int)
{
   lossCount = lossCount + 1;
   // Slow Start stopped, if it hasn't yet
   if (m_bSlowStart)
   {
      m_bSlowStart = false;
      if (m_iRcvRate > 0)
      {
         // Set the sending rate to the receiving rate.
         m_dPktSndPeriod = 1000000.0 / m_iRcvRate;
         return;
      }
      // If no receiving rate is observed, we have to compute the sending
      // rate according to the current window size, and decrease it
      // using the method below.
      m_dPktSndPeriod = m_dCWndSize / (m_iRTT + m_iRCInterval);
   }

   // we check losses in new way
   //   m_bLoss = true;

   if (m_RTTs->check(m_dSigmaThreshold))
   {
      m_bLoss = true;
      lossCount = lossCount - 1;

      // may want to reset additional coefficient here?
      minCongRtt = m_iRTT - minRtt;

      if (CSeqNo::seqcmp(losslist[0] & 0x7FFFFFFF, m_iLastDecSeq) > 0)
      {
         // added lastDecrease time
         m_LastDecreaseTime = CTimer::getTime();

         m_dLastDecPeriod = m_dPktSndPeriod;
         m_dPktSndPeriod = ceil(m_dPktSndPeriod * 1.25);

         m_iAvgNAKNum = (int)ceil(m_iAvgNAKNum * 0.875 + m_iNAKCount * 0.125);
         m_iNAKCount = 1;
         m_iDecCount = 1;

         m_iLastDecSeq = m_iSndCurrSeqNo;

         // remove global synchronization using randomization
         srand(m_iLastDecSeq);
         m_iDecRandom = (int)ceil(m_iAvgNAKNum * (double(rand()) / RAND_MAX));
         if (m_iDecRandom < 1)
            m_iDecRandom = 1;
      }
      else if ((m_iDecCount++ < 3) && (0 == (++m_iNAKCount % m_iDecRandom)))
      {
         // 0.875^5 = 0.51, rate should not be decreased by more than half within a congestion period
         m_LastDecreaseTime = CTimer::getTime();
         m_dPktSndPeriod = ceil(m_dPktSndPeriod * 1.25);
         m_iLastDecSeq = m_iSndCurrSeqNo;
      }
   }
}

void CUDTCC::onTimeout()
{
   if (m_bSlowStart)
   {
      m_bSlowStart = false;
      if (m_iRcvRate > 0)
         m_dPktSndPeriod = 1000000.0 / m_iRcvRate;
      else
         m_dPktSndPeriod = m_dCWndSize / (m_iRTT + m_iRCInterval);
   }
   else
   {
      /*
      m_dLastDecPeriod = m_dPktSndPeriod;
      m_dPktSndPeriod = ceil(m_dPktSndPeriod * 2);
      m_iLastDecSeq = m_iLastAck;
      */
   }
}
