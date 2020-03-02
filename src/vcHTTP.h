#ifndef vcHTTP_H__
#define vcHTTP_H__

#include "udJSON.h"

enum vcHTTPMethods
{
  vcHTTPMethod_Unknown, //Default when haven't processed
  vcHTTPMethod_Get,
  vcHTTPMethod_Post,
  vcHTTPMethod_Put,
  vcHTTPMethod_Patch,
  vcHTTPMethod_Delete,
  vcHTTPMethod_Head,

  vcHTTPMethod_Mail, // Used for SMTP

  vcHTTPMethod_Count
};

enum vcHTTPStatus //This is not a full list of HTTP status codes, we use vHTTP_BadRequest for all unlisted status's
{
  vcHTTP_Unknown = 0,

  vcHTTP_OK = 200, //Default return code
  vcHTTP_Created = 201, //Use when a resource is created
  vcHTTP_Accepted = 202, //Use when a request is queued but has not been completed
  vcHTTP_NoContent = 204, //Use when a request is processed but nothing needs to be returned
  vcHTTP_PartialContent = 206, //Use when request headers request a range of a file
  vcHTTP_MailOK = 250, //Use when requested mail action okay, completed
  vcHTTP_Moved = 301, //The content the browser is looking for is permanently at another URI
  vcHTTP_Redirect = 302, //The content the browser is looking for is temporarily at another URI
  vcHTTP_NotModified = 304, //Use when request headers are the same details as the local version to use the cached
  vcHTTP_BadRequest = 400, //Use when the request headers are malformed
  vcHTTP_Unauthorized = 401, //Use when expecting Auth headers
  vcHTTP_Forbidden = 403, //Use when the current session does not have priviledges to access the resource
  vcHTTP_NotFound = 404, //Resource not found
  vcHTTP_MethodNotAllowed = 405, //Use when expecting POST and GET and vice versa
  vcHTTP_ProxyAuthenticationRequired = 407, //Use when proxy authentication required
  vcHTTP_RequestTimeout = 408, //Use when the client hasn't sent the required information within a given timeout
  vcHTTP_Conflict = 409, //Use when this request conflicts directly with another request already being processed
  vcHTTP_LengthRequired = 411, //Use when a content-length is required (e.g. uploaded)
  vcHTTP_PreconditionFailed = 412, //Precondition failed, used when a plugin cannot be found
  vcHTTP_PayloadTooLarge = 413, //Use when the post data is over the servers limits
  vcHTTP_URITooLong = 414, //Use when the URI is too long
  vcHTTP_UnsupportedFormat = 415, //Use when the requested media isn't of a supported type
  vcHTTP_RangeNotSatisfiable = 416, //Use when the requested file portion isn't available
  vcHTTP_ServerError = 500, //Internal error that was able to be handled as such
  vcHTTP_NotImplemented = 501, //Either not recognized request or features coming soon
  vcHTTP_ServiceUnavailable = 503, //Server is overloaded
  vcHTTP_BadHTTPVersion = 505, //Unsupported version of HTTP protocol
};

enum vcHTTPLimits
{
  vcHTTP_MaxURILen = 256
};

struct vcHTTPHeaders
{
  vcHTTPStatus status;
  vcHTTPMethods method;
  char URI[vcHTTP_MaxURILen];
  udJSON apiTokens;
  udJSON tokens;
  udJSON cookies;
  const char *pUserAgentString;
};

udResult vcHTTP_StatusToudResult(vcHTTPStatus statusCode);
void vcHTTP_SplitAndInject(udJSON *pValue, char *pData, char delimiter, char equals, bool doTrim);
void vcHTTP_ProcessHeaders(vcHTTPHeaders *pHeaders, char *pHTTPData, char **ppBodyData = nullptr);
const char* vcHTTP_StatusAsText(vcHTTPStatus status); //Converted status code to the human readable name; note that you don't own the ptr after this call
char* vcHTTP_Decode(const char *data, size_t dataSize); //Returns a new string that you need to udFree

#endif //vcHTTP_H__
