#include "vcHTTP.h"
#include "udStringUtil.h"

udResult vcHTTP_StatusToudResult(vcHTTPStatus statusCode)
{
  switch (statusCode)
  {
  case vcHTTP_OK: return udR_Success;
  case vcHTTP_ServerError: return udR_ServerError;
  case vcHTTP_Unauthorized: return udR_NotAllowed;
  case vcHTTP_Forbidden: return udR_NotAllowed;
  case vcHTTP_ProxyAuthenticationRequired: return udR_AuthError;
  case vcHTTP_RequestTimeout: return udR_SessionExpired;
  case vcHTTP_ServiceUnavailable: return udR_Pending;
  }

  if (statusCode >= 500 && statusCode <= 599)
    return udR_ServerError;
  return udR_Failure_;
}

void vcHTTP_SplitAndInject(udJSON *pValue, char *pData, char delimiter, char equals, bool doTrim)
{
  char *pCarat = pData;

  do
  {
    char *pStrStart = pCarat;
    char *pStrMid = nullptr;

    while (*pCarat != delimiter && *pCarat != '\0')
    {
      if (*pCarat == equals && pStrMid == nullptr)
        pStrMid = pCarat;
      ++pCarat;
    }

    bool last = (*pCarat == '\0');

    if (pStrMid != nullptr)
    {
      //Null terminate strings
      char endToken = *pCarat;
      *pStrMid = '\0';
      *pCarat = '\0';

      char *pStrData = pStrMid + 1;

      //Trim
      while (doTrim && (*pStrData == ' ' || *pStrData == '\t') && pStrData < pCarat)
        ++pStrData;

      //Add key to headers
      pValue->Set("%s = '%s'", pStrStart, pStrData);

      //Remove null terminators
      *pStrMid = equals;
      *pCarat = endToken;
    }

    if (last)
      break;
    else
      ++pCarat;
  } while (true);
}

void vcHTTP_ProcessHeaders(vcHTTPHeaders *pHeaders, char *pHTTPData, char **ppBodyData /*= nullptr*/)
{
  if (pHeaders == nullptr) //Cannot do anything if out is nullptr
    return;

  memset(pHeaders, 0, sizeof(vcHTTPHeaders));
  pHeaders->status = vcHTTP_OK;
  pHeaders->method = vcHTTPMethod_Unknown;
  pHeaders->tokens.SetObject();
  udStrcpy(pHeaders->URI, "/");

  if (pHTTPData == nullptr) //The data is now "zero" for more handling
  {
    pHeaders->status = vcHTTP_BadRequest;
    return;
  }

  char *pCarat = pHTTPData;

  //Extract first line
  while (*pCarat != ' ' && *pCarat != '\0' && *pCarat != '\r' && *pCarat != '\n')
    ++pCarat;

  if (*pCarat == ' ')
  {
    ptrdiff_t len = pCarat - pHTTPData;
    if (len == 3 && udStrncmp("GET", pHTTPData, len) == 0)
      pHeaders->method = vcHTTPMethod_Get;
    else if (len == 4 && udStrncmp("POST", pHTTPData, len) == 0)
      pHeaders->method = vcHTTPMethod_Post;
    else if (len == 3 && udStrncmp("PUT", pHTTPData, len) == 0)
      pHeaders->method = vcHTTPMethod_Put;
    else if (len == 5 && udStrncmp("PATCH", pHTTPData, len) == 0)
      pHeaders->method = vcHTTPMethod_Patch;
    else if (len == 6 && udStrncmp("DELETE", pHTTPData, len) == 0)
      pHeaders->method = vcHTTPMethod_Delete;
    else if (len == 4 && udStrncmp("HEAD", pHTTPData, len) == 0)
      pHeaders->method = vcHTTPMethod_Head;
    else
      pHeaders->status = vcHTTP_BadRequest;

    ++pCarat;
    char *pURIStart = pCarat;
    char *pGetStart = nullptr;

    while (*pCarat != ' ' && *pCarat != '\0' && *pCarat != '\r' && *pCarat != '\n')
    {
      if (pGetStart == nullptr && *pCarat == '?')
      {
        pGetStart = pCarat;
        *pGetStart = '\0';
      }

      ++pCarat;
    }

    if (*pCarat == ' ')
    {
      *pCarat = '\0';

      // Clip extra slashes
      while (*pURIStart && *pURIStart == '/')
        pURIStart++;
      udStrcat(pHeaders->URI, pURIStart);

      if (pGetStart != nullptr)
      {
        pHeaders->tokens.Set("Get = {}");

        udJSON *pGetToken;
        pHeaders->tokens.Get(&pGetToken, "Get");

        vcHTTP_SplitAndInject(pGetToken, &pGetStart[1], '&', '=', false);

        *pGetStart = '?'; //Reset the character
      }

      *pCarat = ' ';
    }
    else
    {
      pHeaders->status = vcHTTP_BadRequest;
    }
  }
  else
  {
    pHeaders->status = vcHTTP_BadRequest;
  }

  //Extract the rest of the headers
  do
  {
    char *pLineStart = pCarat;
    char *pColon = nullptr;
    char *pLineEnd = nullptr;

    while (*pCarat != '\r' && *pCarat != '\n' && *pCarat != '\0')
    {
      if (*pCarat == ':' && pColon == nullptr)
        pColon = pCarat;
      ++pCarat;
    }

    pLineEnd = pCarat;

    if (pLineEnd - pLineStart < 2)
      break; //Finished processing

    if (pColon != nullptr)
    {
      //Null terminate strings
      char endToken = *pLineEnd;
      *pColon = '\0';
      *pLineEnd = '\0';

      char *pColonStart = pColon + 1;

      while ((*pColonStart == ' ' || *pColonStart == '\t') && pColonStart < pLineEnd)
        ++pColonStart;

      pHeaders->tokens.Set("%s = '%s'", pLineStart, pColonStart);

      if (udStrEqual(pLineStart, "User-Agent"))
        pHeaders->pUserAgentString = pHeaders->tokens.Get("User-Agent").AsString(); //The string is owned by the value

      //Remove null terminators
      *pLineEnd = endToken;
      *pColon = ':';
    }

    //Remove extra whitespace
    int total = 0;
    while (*pCarat == '\r' || *pCarat == '\n')
    {
      ++pCarat;
      ++total;
    }

    //Exit if the body's been reached, signified by "\r\n\r\n", or if the end of the header has been reached and there's no body, signified by '\0'.
    if (*pCarat == '\0' || total == 4)
      break;
  } while (true);

  //Copy body pointer out
  if (ppBodyData != nullptr && *pCarat != '\0')
    *ppBodyData = pCarat;

  //Handle the Cookies strings
  if (pHeaders->tokens.Get("Cookie").IsString())
  {
    udJSON *pCookieValue;
    pHeaders->tokens.Get(&pCookieValue, "Cookie");

    char *pCookieStr;
    pCookieValue->ExtractAndVoid((const char**)&pCookieStr);

    vcHTTP_SplitAndInject(&pHeaders->cookies, pCookieStr, ';', '=', true);

    udFree(pCookieStr);
  }

  if (!pHeaders->tokens.Get("Cookie").IsObject())
    pHeaders->tokens.Set("Cookie = {}");
}

const char* vcHTTP_StatusAsText(vcHTTPStatus status)
{
  switch (status)
  {
  case vcHTTP_OK:
    return "OK";
  case vcHTTP_Created:
    return "Created";
  case vcHTTP_Accepted:
    return "Accepted";
  case vcHTTP_NoContent:
    return "No Content";
  case vcHTTP_PartialContent:
    return "Partial Content";
  case vcHTTP_Moved:
    return "Moved Permanently";
  case vcHTTP_Redirect:
    return "Found";
  case vcHTTP_NotModified:
    return "Not Modified";
  case vcHTTP_BadRequest:
    return "Bad Request";
  case vcHTTP_Forbidden:
    return "Forbidden";
  case vcHTTP_NotFound:
    return "Not Found";
  case vcHTTP_MethodNotAllowed:
    return "Method Not Allowed";
  case vcHTTP_RequestTimeout:
    return "Request Time - out";
  case vcHTTP_Conflict:
    return "Conflict";
  case vcHTTP_LengthRequired:
    return "Length Required";
  case vcHTTP_PreconditionFailed:
    return "Precondition Failed";
  case vcHTTP_PayloadTooLarge:
    return "Request Entity Too Large";
  case vcHTTP_URITooLong:
    return "Request - URI Too Large";
  case vcHTTP_UnsupportedFormat:
    return "Unsupported Media Type";
  case vcHTTP_RangeNotSatisfiable:
    return "Requested range not satisfiable";
  case vcHTTP_ServerError:
    return "Internal Server Error";
  case vcHTTP_NotImplemented:
    return "Not Implemented";
  case vcHTTP_ServiceUnavailable:
    return "Service Unavailable";
  case vcHTTP_BadHTTPVersion:
    return "HTTP Version not supported";
  default:
    return "Internal Server Error";
  }
}

// Convert URL-encoding
char* vcHTTP_Decode(const char *data, size_t dataSize)
{
  char *decodedPOST = udAllocType(char, dataSize + 1, udAF_None);
  size_t decodedPOSTLength = 0;
  for (size_t i = 0; i < dataSize; ++i)
  {
    // Handle encoded character
    if (data[i] == '%')
    {
      // Ensure we only use the two hex characters
      char buffer[3] = { data[i + 1], data[i + 2], '\0' };

      decodedPOST[decodedPOSTLength] = (char)udStrAtoi(buffer, nullptr, 16);
      ++decodedPOSTLength;
      i += 2; // Skip the two hex characters
    }
    else
    {
      decodedPOST[decodedPOSTLength] = data[i];
      ++decodedPOSTLength;
    }
  }

  decodedPOST[decodedPOSTLength] = '\0';

  return decodedPOST;
}
