// shrstuff.spp -- implements the linked list operations

#include <stdafx.h>
#include <windows.h>

#include "comclass.h"

#include "cmutex.h"
#include "dbcode.h"


ChainedQueue::ChainedQueue(void *pSharedMemory, int iLength,CMutex *hM)
{
 hM->Claim();
 COMPLETECHAINLIST *ccp= (COMPLETECHAINLIST *)pSharedMemory;
 if (!ccp->bValid)
// this is a little whacky: We set the first record aside to keep status information.
// this requires that the data structure has at least two integer members at the beginning.
 {
  ccp->bValid = FALSE;
  ccp->cl.iInsecuredElement=1;
  ccp->cl.iSecuredElement=iLength/sizeof(COMPLETECHAINLIST)-1;
 };
 hM->Release();
 cpBase = (COMPLETECHAINLIST *)pSharedMemory;
 m_hMutex = hM;
};


ChainedQueue::ChainedQueue()
{
};

ChainedQueue::~ChainedQueue()
{
};

BOOL ChainedQueue::Remove(int iIndex)
{
 if (!cpBase[iIndex].bValid)
 {
  m_iErrorCode = ERROR_INVALID_INDEX;
  return FALSE;
 };
 m_hMutex->Claim();
 cpBase[iIndex].bValid = FALSE;
 if (iIndex && (iIndex+1==cpBase[0].cl.iInsecuredElement)) cpBase[0].cl.iInsecuredElement--;
 m_hMutex->Release();
 return TRUE;
};

BOOL ChainedQueue::Insert(int *iIndex,CHAINLIST *cpElement)
{
 m_hMutex->Claim();
 if (cpBase[0].cl.iInsecuredElement >= cpBase[0].cl.iSecuredElement) 
 {
  m_iErrorCode = ERROR_NOT_ENOUGH_MEMORY;
  m_hMutex->Release();
  return FALSE;
 };
 int iLoop;
 for (iLoop=1;iLoop<=cpBase[0].cl.iInsecuredElement;iLoop++)
  if (!cpBase[iLoop].bValid) break;
 memcpy(&cpBase[iLoop],cpElement,sizeof(CHAINLIST));
 cpBase[iLoop].bValid=TRUE;
 *iIndex = iLoop;
 if (iLoop == cpBase[0].cl.iInsecuredElement) cpBase[0].cl.iInsecuredElement++;
 m_hMutex->Release();
 return TRUE;
};

BOOL ChainedQueue::Retrieve(int iIndex,CHAINLIST *cpResult)
{
 m_hMutex->Claim();
 if (iIndex > cpBase[0].cl.iSecuredElement || !cpBase[iIndex].bValid)
 {
  m_iErrorCode = ERROR_INVALID_INDEX;
  m_hMutex->Release();
  return FALSE;
 };
 memcpy(cpResult,&cpBase[iIndex],sizeof(CHAINLIST));
 m_hMutex->Release();
 return TRUE;
};


// server object class

#ifdef SERVER

#include "npipe.h"


ServerChainedQueue::ServerChainedQueue(void *pSharedMemory, int iLength,CMutex *hM) : ChainedQueue(pSharedMemory,iLength,hM)
{
}

#endif