#ifndef __UDT_LIST_H__
#define __UDT_LIST_H__

#include "udt.h"
#include "common.h"

class CSndLossList
{
public:
   CSndLossList(int size = 1024);
   ~CSndLossList();

   // Functionality:
   //    Insert a seq. no. into the sender loss list.
   // Parameters:
   //    0) [in] seqno1: sequence number starts.
   //    1) [in] seqno2: sequence number ends.
   // Returned value:
   //    number of packets that are not in the list previously.

   int insert(int32_t seqno1, int32_t seqno2);

   // Functionality:
   //    Remove ALL the seq. no. that are not greater than the parameter.
   // Parameters:
   //    0) [in] seqno: sequence number.
   // Returned value:
   //    None.

   void remove(int32_t seqno);

   // Functionality:
   //    Read the loss length.
   // Parameters:
   //    None.
   // Returned value:
   //    The length of the list.

   int getLossLength();

   // Functionality:
   //    Read the first (smallest) loss seq. no. in the list and remove it.
   // Parameters:
   //    None.
   // Returned value:
   //    The seq. no. or -1 if the list is empty.

   int32_t getLostSeq();

private:
   int32_t *m_piData1; // sequence number starts
   int32_t *m_piData2; // seqnence number ends
   int *m_piNext;      // next node in the list

   int m_iHead;          // first node
   int m_iLength;        // loss length
   int m_iSize;          // size of the static array
   int m_iLastInsertPos; // position of last insert node

   pthread_mutex_t m_ListLock; // used to synchronize list operation

private:
   CSndLossList(const CSndLossList &);
   CSndLossList &operator=(const CSndLossList &);
};

////////////////////////////////////////////////////////////////////////////////

class CRcvLossList
{
public:
   CRcvLossList(int size = 1024);
   ~CRcvLossList();

   // Functionality:
   //    Insert a series of loss seq. no. between "seqno1" and "seqno2" into the receiver's loss list.
   // Parameters:
   //    0) [in] seqno1: sequence number starts.
   //    1) [in] seqno2: seqeunce number ends.
   // Returned value:
   //    None.

   void insert(int32_t seqno1, int32_t seqno2);

   // Functionality:
   //    Remove a loss seq. no. from the receiver's loss list.
   // Parameters:
   //    0) [in] seqno: sequence number.
   // Returned value:
   //    if the packet is removed (true) or no such lost packet is found (false).

   bool remove(int32_t seqno);

   // Functionality:
   //    Remove all packets between seqno1 and seqno2.
   // Parameters:
   //    0) [in] seqno1: start sequence number.
   //    1) [in] seqno2: end sequence number.
   // Returned value:
   //    if the packet is removed (true) or no such lost packet is found (false).

   bool remove(int32_t seqno1, int32_t seqno2);

   // Functionality:
   //    Find if there is any lost packets whose sequence number falling seqno1 and seqno2.
   // Parameters:
   //    0) [in] seqno1: start sequence number.
   //    1) [in] seqno2: end sequence number.
   // Returned value:
   //    True if found; otherwise false.

   bool find(int32_t seqno1, int32_t seqno2) const;

   // Functionality:
   //    Read the loss length.
   // Parameters:
   //    None.
   // Returned value:
   //    the length of the list.

   int getLossLength() const;

   // Functionality:
   //    Read the first (smallest) seq. no. in the list.
   // Parameters:
   //    None.
   // Returned value:
   //    the sequence number or -1 if the list is empty.

   int getFirstLostSeq() const;

   // Functionality:
   //    Get a encoded loss array for NAK report.
   // Parameters:
   //    0) [out] array: the result list of seq. no. to be included in NAK.
   //    1) [out] physical length of the result array.
   //    2) [in] limit: maximum length of the array.
   // Returned value:
   //    None.

   void getLossArray(int32_t *array, int &len, int limit);

private:
   int32_t *m_piData1; // sequence number starts
   int32_t *m_piData2; // sequence number ends
   int *m_piNext;      // next node in the list
   int *m_piPrior;     // prior node in the list;

   int m_iHead;   // first node in the list
   int m_iTail;   // last node in the list;
   int m_iLength; // loss length
   int m_iSize;   // size of the static array

private:
   CRcvLossList(const CRcvLossList &);
   CRcvLossList &operator=(const CRcvLossList &);
};

#endif