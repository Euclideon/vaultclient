//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Dave Pevreal, March 2014
//
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "udPlatform.h"

#if !UDPLATFORM_NACL

#include "udPlatformUtil.h"
#include "udFileHandler.h"
#include "udMath.h"
#if UDPLATFORM_WINDOWS
# include <winsock2.h>
#define snprintf _snprintf
#elif UDPLATFORM_LINUX || UDPLATFORM_OSX || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_IOS || UDPLATFORM_ANDROID
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# define INVALID_SOCKET -1
# define SOCKET_ERROR -1
#endif
#include <stdio.h>

static udFile_OpenHandlerFunc                     udFileHandler_HTTPOpen;
static udFile_SeekReadHandlerFunc                 udFileHandler_HTTPSeekRead;
static udFile_BlockForPipelinedRequestHandlerFunc udFileHandler_HTTPBlockForPipelinedRequest;
static udFile_CloseHandlerFunc                    udFileHandler_HTTPClose;

// Register the HTTP handler (optional as it requires networking libraries, WS2_32.lib on Windows platform)
udResult udFile_RegisterHTTP()
{
  return udFile_RegisterHandler(udFileHandler_HTTPOpen, "http:");
}


static char s_HTTPHeaderString[] = "HEAD %s HTTP/1.1\r\nHost: %s\r\nConnection: Keep-Alive\r\nUser-Agent: Euclideon udSDK/2.0\r\n\r\n";
static char s_HTTPGetString[] = "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Euclideon udSDK/2.0\r\nConnection: Keep-Alive\r\nRange: bytes=%lld-%lld\r\n\r\n";


// The udFile derivative for supporting HTTP
struct udFile_HTTP : public udFile
{
  udMutex *pMutex;                        // Used only when the udFOF_Multithread flag is used to ensure safe access from multiple threads
  udURL url;
  bool wsInitialised;
  char recvBuffer[1024];
  struct sockaddr_in server;
#if UDPLATFORM_WINDOWS
  WSADATA wsaData;
  SOCKET sock;
#else
  int sock;
#endif
  int sockID; // Each time a socket it created we increment this number, this way pipelined requests from a dead socket can be identified as dead
};


// ----------------------------------------------------------------------------
// Open the socket
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPOpenSocket(udFile_HTTP *pFile)
{
  udResult result;

  if (pFile->sock == INVALID_SOCKET)
  {
    pFile->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    UD_ERROR_IF(pFile->sock == INVALID_SOCKET, udR_SocketError);

    UD_ERROR_IF(connect(pFile->sock, (struct sockaddr*)&pFile->server, sizeof(pFile->server)) == SOCKET_ERROR, udR_SocketError);
  }

  // TODO: TCP_NODELAY disables the Nagle algorithm.
  //       SO_KEEPALIVE enables periodic 'liveness' pings, if supported by the OS.

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    udDebugPrintf("Error %s opening socket\n", udResultAsString(result));
  return result;
}


// ----------------------------------------------------------------------------
// Close the socket
// Author: Dave Pevreal, March 2014
static void udFileHandler_HTTPCloseSocket(udFile_HTTP *pFile)
{
  if (pFile->sock != INVALID_SOCKET)
  {
#if UDPLATFORM_WINDOWS
    shutdown(pFile->sock, SD_BOTH);
#else
    close(pFile->sock);
#endif
    pFile->sock = INVALID_SOCKET;
  }
  ++pFile->sockID;
}


// ----------------------------------------------------------------------------
// Send a request
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPSendRequest(udFile_HTTP *pFile, int len)
{
  udResult result;

  UD_ERROR_CHECK(udFileHandler_HTTPOpenSocket(pFile));
  if (send(pFile->sock, pFile->recvBuffer, len, 0) == SOCKET_ERROR)
  {
    // On error, first try closing and re-opening the socket before giving up
    udFileHandler_HTTPCloseSocket(pFile);
    udFileHandler_HTTPOpenSocket(pFile);
    if (send(pFile->sock, pFile->recvBuffer, len, 0) == SOCKET_ERROR)
    {
      udDebugPrintf("http send returned socket error\n");
      UD_ERROR_SET(udR_SocketError);
    }
  }

  result = udR_Success;

epilogue:
  if (result != udR_Success)
    udDebugPrintf("Error %s sending request:\n%s\n--end--\n", udResultAsString(result), pFile->recvBuffer);
  return result;
}


// ----------------------------------------------------------------------------
// Receive a response for a GET packet, parsing the string header before
// delivering the payload
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPRecvGET(udFile_HTTP *pFile, void *pBuffer, size_t bufferLength, size_t *pActualRead)
{
  udResult result;
  size_t bytesReceived = 0;      // Number of bytes received from this packet
  int code;
  size_t headerLength; // length of the response header, before any payload
  const char *s;
  bool closeConnection = false;
  int64_t contentLength;
  int recvCode;

  result = udFileHandler_HTTPOpenSocket(pFile);
  if (result != udR_Success)
    udDebugPrintf("Unable to open socket\n");
  UD_ERROR_HANDLE();

  recvCode = recv(pFile->sock, pFile->recvBuffer, sizeof(pFile->recvBuffer), 0);
  if (recvCode == SOCKET_ERROR)
  {
    udFileHandler_HTTPCloseSocket(pFile);
    if (udFileHandler_HTTPOpenSocket(pFile) == udR_Success)
      recvCode = recv(pFile->sock, pFile->recvBuffer, sizeof(pFile->recvBuffer), 0);
    if (recvCode == SOCKET_ERROR)
    {
      udDebugPrintf("recv returned SOCKET_ERROR\n");
      UD_ERROR_SET(udR_SocketError);
    }
  }
  bytesReceived += recvCode;

  while (udStrstr(pFile->recvBuffer, bytesReceived, "\r\n\r\n", &headerLength) == nullptr && (size_t)bytesReceived < sizeof(pFile->recvBuffer))
  {
    recvCode = recv(pFile->sock, pFile->recvBuffer + bytesReceived, (int)(sizeof(pFile->recvBuffer) - bytesReceived), 0);
    if (recvCode == SOCKET_ERROR)
    {
      udDebugPrintf("recv returned SOCKET_ERROR\n");
      UD_ERROR_SET(udR_SocketError);
    }
    bytesReceived += recvCode;
  }

  // First, check the top line for HTTP version and error code
  code = 0;
  sscanf(pFile->recvBuffer, "HTTP/1.1 %d", &code);
  if ((code != 200 && code != 206) || (headerLength == (size_t)bytesReceived)) // if headerLength is bytesReceived, never found the \r\n\r\n
  {
    udDebugPrintf("Fail on packet: code = %d headerLength = %d (bytesReceived = %d)\n", code, (int)headerLength, (int)bytesReceived);
    UD_ERROR_SET(udR_SocketError);
  }

  headerLength += 4;
  pFile->recvBuffer[headerLength-1] = 0; // null terminate the header part
  //udDebugPrintf("Received:\n%s--end--\n", pFile->recvBuffer);

  // Check for a request from the server to close the connection after dealing with this
  closeConnection = udStrstr(pFile->recvBuffer, headerLength, "Connection: close") != nullptr;
  if (closeConnection)
    udDebugPrintf("Server requesting connection close\n");

  s = udStrstr(pFile->recvBuffer, headerLength, "Content-Length:");
  if (!s)
  {
    udDebugPrintf("http: No content-length field found\n");
    UD_ERROR_SET(udR_SocketError);
  }
  contentLength = udStrAtoi64(s + 15);

  if (!pBuffer)
  {
    // Parsing response to the HEAD to get size of overall file
    pFile->fileLength = contentLength;
  }
  else
  {
    // Parsing response to a GET
    if (contentLength > (int64_t)bufferLength)
    {
      udDebugPrintf("contentLength=%" PRId64 " bufferLength=%zu\n", contentLength, bufferLength);
      UD_ERROR_SET(udR_SocketError);
    }

    // Some servers send more data after the content, we should throw it out
    bytesReceived = udMin((int64_t)bytesReceived - (int64_t)headerLength, contentLength);
    memcpy(pBuffer, pFile->recvBuffer + headerLength, bytesReceived);

    while (bytesReceived < (size_t)contentLength)
    {
      recvCode = recv(pFile->sock, ((char*)pBuffer) + bytesReceived, udMin(int(bufferLength - bytesReceived), int(contentLength - bytesReceived)), 0);
      if (recvCode == SOCKET_ERROR)
        UD_ERROR_SET(udR_SocketError);
      bytesReceived += recvCode;
    }
    if (pActualRead)
      *pActualRead = bytesReceived;
  }

  result = udR_Success;

epilogue:
  if (result != udR_Success || closeConnection)
    udFileHandler_HTTPCloseSocket(pFile);
  if (result != udR_Success)
    udDebugPrintf("Error receiving request:\n%s\n--end--\n", pFile->recvBuffer);

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of OpenHandler via HTTP
// Author: Dave Pevreal, March 2014
udResult udFileHandler_HTTPOpen(udFile **ppFile, const char *pFilename, udFileOpenFlags flags)
{
  udResult result;
  udFile_HTTP *pFile = nullptr;
  struct hostent *hp;

  int actualHeaderLen;

  // Automatically fail if trying to write to files on http
  if (flags & (udFOF_Write|udFOF_Create))
    UD_ERROR_SET(udR_OpenFailure);

  pFile = udAllocType(udFile_HTTP, 1, udAF_Zero);
  UD_ERROR_NULL(pFile, udR_MemoryAllocationFailure);

  if (flags & udFOF_Multithread)
  {
    pFile->pMutex = udCreateMutex();
    UD_ERROR_NULL(pFile->pMutex, udR_InternalError);
  }

  pFile->url.Construct();
  pFile->wsInitialised = false;
  pFile->sock = INVALID_SOCKET;

  UD_ERROR_CHECK(pFile->url.SetURL(pFilename));

  UD_ERROR_IF(!udStrEqual(pFile->url.GetScheme(), "http"), udR_OpenFailure);

#if UDPLATFORM_WINDOWS
  // TODO: Only do this once and reference count
  if (WSAStartup(MAKEWORD(2, 2), &pFile->wsaData))
  {
    udDebugPrintf("WSAStartup failed\n");
    UD_ERROR_SET(udR_SocketError);
  }
  pFile->wsInitialised = true;
#endif

  //udDebugPrintf("Resolving %s to ip address...", pFile->url.GetDomain());
  hp = gethostbyname(pFile->url.GetDomain());
  if (!hp)
  {
    udDebugPrintf("gethostbyname failed to resolve url domain\n");
    UD_ERROR_SET(udR_SocketError);
  }
  pFile->server.sin_addr.s_addr = *((unsigned long*)hp->h_addr); // TODO: This is bad. use h_addr_list instead
  pFile->server.sin_family = AF_INET;
  pFile->server.sin_port = htons((u_short) pFile->url.GetPort());

  // TODO: Support for udFOF_FastOpen
  result = udR_Failure_;
  actualHeaderLen = snprintf(pFile->recvBuffer, sizeof(pFile->recvBuffer)-1, s_HTTPHeaderString, pFile->url.GetPathWithQuery(), pFile->url.GetDomain());
  UD_ERROR_IF(actualHeaderLen < 0, udR_Failure_);

  //udDebugPrintf("Sending:\n%s", pFile->recvBuffer);
  UD_ERROR_CHECK(udFileHandler_HTTPSendRequest(pFile, (int)actualHeaderLen));
  UD_ERROR_CHECK(udFileHandler_HTTPRecvGET(pFile, nullptr, 0, nullptr));

  pFile->fpRead = udFileHandler_HTTPSeekRead;
  pFile->fpBlockPipedRequest = udFileHandler_HTTPBlockForPipelinedRequest;
  pFile->fpClose = udFileHandler_HTTPClose;

  *ppFile = pFile;
  pFile = nullptr;
  result = udR_Success;

epilogue:
  if (pFile)
    udFileHandler_HTTPClose((udFile**)&pFile);

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of SeekReadHandler via HTTP
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPSeekRead(udFile *pBaseFile, void *pBuffer, size_t bufferLength, int64_t seekOffset, size_t *pActualRead, udFilePipelinedRequest *pPipelinedRequest)
{
  udResult result;
  udFile_HTTP *pFile = static_cast<udFile_HTTP *>(pBaseFile);

  if (pFile->pMutex)
    udLockMutex(pFile->pMutex);

  //udDebugPrintf("\nSeekRead: %lld bytes at offset %lld\n", bufferLength, offset);
  size_t actualHeaderLen = snprintf(pFile->recvBuffer, sizeof(pFile->recvBuffer)-1, s_HTTPGetString, pFile->url.GetPathWithQuery(), pFile->url.GetDomain(), seekOffset, seekOffset + bufferLength-1);

  UD_ERROR_CHECK(udFileHandler_HTTPSendRequest(pFile, (int)actualHeaderLen));

  if (pPipelinedRequest)
  {
    pPipelinedRequest->reserved[0] = (uint64_t)(pBuffer);
    pPipelinedRequest->reserved[1] = (uint64_t)(bufferLength);
    pPipelinedRequest->reserved[2] = (uint64_t)pFile->sockID;
    pPipelinedRequest->reserved[3] = 0;
    if (pActualRead)
      *pActualRead = bufferLength; // Being optimistic
  }
  else
  {
    UD_ERROR_CHECK(udFileHandler_HTTPRecvGET(pFile, pBuffer, bufferLength, pActualRead));
  }

epilogue:

  if (pFile->pMutex)
    udReleaseMutex(pFile->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of BlockForPipelinedRequest via HTTP
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPBlockForPipelinedRequest(udFile *pBaseFile, udFilePipelinedRequest *pPipelinedRequest, size_t *pActualRead)
{
  udResult result;
  udFile_HTTP *pFile = static_cast<udFile_HTTP *>(pBaseFile);

  if (pFile->pMutex)
    udLockMutex(pFile->pMutex);

  void *pBuffer = (void*)(pPipelinedRequest->reserved[0]);
  size_t bufferLength = (size_t)(pPipelinedRequest->reserved[1]);
  int sockID = (int)pPipelinedRequest->reserved[2];
  if (sockID != pFile->sockID)
  {
    udDebugPrintf("Pipelined request failed due to socket close/reopen. Expected %d, socket id is now %d\n", sockID, pFile->sockID);
    UD_ERROR_SET(udR_SocketError);
  }
  else
  {
    UD_ERROR_CHECK(udFileHandler_HTTPRecvGET(pFile, pBuffer, bufferLength, pActualRead));
  }
  result = udR_Success;

epilogue:
  if (pFile->pMutex)
    udReleaseMutex(pFile->pMutex);

  return result;
}


// ----------------------------------------------------------------------------
// Implementation of CloseHandler via HTTP
// Author: Dave Pevreal, March 2014
static udResult udFileHandler_HTTPClose(udFile **ppFile)
{
  udFile_HTTP *pFile = nullptr;

  if (ppFile)
  {
    pFile = static_cast<udFile_HTTP *>(*ppFile);
    *ppFile = nullptr;
    if (pFile)
    {
      udFileHandler_HTTPCloseSocket(pFile);
#if UDPLATFORM_WINDOWS
      if (pFile->wsInitialised)
      {
        WSACleanup();
        pFile->wsInitialised = false;
      }
#endif
      if (pFile->pMutex)
        udDestroyMutex(&pFile->pMutex);
      pFile->url.~udURL();
      udFree(pFile);
    }
  }

  return udR_Success;
}

#endif // !UDPLATFORM_NACL
