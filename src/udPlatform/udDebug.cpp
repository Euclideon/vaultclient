#define _CRT_SECURE_NO_WARNINGS
#include "udDebug.h"
#include "udPlatformUtil.h"
#include <stdio.h>
#include <stdarg.h>

void (*gpudDebugPrintfOutputCallback)(const char *pString) = nullptr;

// *********************************************************************
void udDebugPrintf(const char *format, ...)
{
  va_list args;
  char bufferStackMem[80];
  int prefix = 0;
  char *pBuffer = bufferStackMem;
  int bufferLen = (int)sizeof(bufferStackMem);
  int required = 0;

  static bool multiThreads = false;
  static int lastThread = -1;

  if (!multiThreads)
  {
    multiThreads = (lastThread!=-1) && (lastThread != udTrace::GetThreadId());
    lastThread = udTrace::GetThreadId();
  }

  if (multiThreads)
  {
    udSprintf(pBuffer, bufferLen, "%02d>", udTrace::GetThreadId());
    prefix = (int)strlen(pBuffer);
  }

  va_start(args, format);
  required = udSprintfVA(pBuffer + prefix, bufferLen-prefix, format, args);
  va_end(args);
  if (required >= (bufferLen - prefix))
  {
    // The string is bigger than the temp on-stack buffer, so allocate a bigger one
    pBuffer[prefix] = 0;
    bufferLen = required + prefix + 1;
    char *pNewBuf = udAllocStack(char, bufferLen, udAF_None);
    if (!pNewBuf)
      return;
    udStrcpy(pNewBuf, bufferLen, pBuffer);
    pBuffer = pNewBuf;

    va_start(args, format);
    udSprintfVA(pBuffer + prefix, bufferLen - prefix, format, args);
    va_end(args);
  }

  if (gpudDebugPrintfOutputCallback)
  {
    gpudDebugPrintfOutputCallback(pBuffer);
  }
  else
  {
#ifdef _WIN32
    OutputDebugStringA(pBuffer);
#else
    fprintf(stderr, "%s", pBuffer);
#endif
  }
}

UDTHREADLOCAL udTrace *udTrace::head = NULL;
UDTHREADLOCAL int udTrace::depth = 0;
UDTHREADLOCAL int udTrace::threadId = -1;
static udInterlockedInt32 nextThreadId;

// ***************************************************************************************
int udTrace::GetThreadId()
{
  if (threadId==-1)
    threadId = nextThreadId++;

  return threadId;
}

// ***************************************************************************************
udTrace::udTrace(const char *a_functionName, int traceLevel)
{
  if (threadId==-1)
  {
    threadId = nextThreadId++;
    if (traceLevel)
      udDebugPrintf("Thread %d started with %s\n", threadId, a_functionName);
  }
  functionName = a_functionName;
  next = head;
  head = this;
  entryPrinted = false;
  if (traceLevel > 1)
  {
    udDebugPrintf("%*.s Entering %s\n", depth*2, "", functionName);
    entryPrinted = true;
  }
  ++depth;
}

// ***************************************************************************************
udTrace::~udTrace()
{
  --depth;
  head = next;
  if (entryPrinted)
    udDebugPrintf("%*.s Exiting  %s\n", depth*2, "", functionName);
}

// ***************************************************************************************
void udTrace::Message(const char *pFormat, ...)
{
  va_list args;
  char buffer[300];

  va_start(args, pFormat);
  udSprintfVA(buffer, sizeof(buffer), pFormat, args);
  if (head && !head->entryPrinted)
  {
    const char *pParent0 = "";
    const char *pParent1 = "";
    const char *pParent2 = "";
    if (head->next)
    {
      pParent0 = head->next->functionName;
      if (head->next->next)
      {
        pParent1 = head->next->next->functionName;
        if (head->next->next->next)
          pParent1 = head->next->next->next->functionName;
      }
    }
    udDebugPrintf("%*.s Within  %s->%s->%s->%s\n", (depth-1)*2, "", pParent2, pParent1, pParent0, head->functionName);
    head->entryPrinted = true;
  }
  udDebugPrintf("%*.s %s\n", head ? head->depth*2 : 0, "", buffer);
}


// ***************************************************************************************
void udTrace::ShowCallstack()
{
  udDebugPrintf("Callstack:\n");
  for (udTrace *p = head; p; p = p->next)
    udDebugPrintf("  %s\n", p->functionName);
}

// ***************************************************************************************
void udTrace_Memory(const char *pName, const void *pMem, int length, int line)
{
  char format[100];
  if (pName)
    udTrace::Message("Dump of memory for %s (%d bytes at %p, line #%d)", pName, length, pMem, line);
  unsigned char p[16];
  udStrcpy(format, sizeof(format), "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x");
  while (length > 0)
  {
    int n = (length > 16) ? 16 : length;
    memset(p, 0, sizeof(p));
    memcpy(p, pMem, n);
    format[n * 5] = 0; // nul terminate in the correct spot
    udTrace::Message(format, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    pMem = ((const char*)pMem)+n;
    length -= n;
  }
}
