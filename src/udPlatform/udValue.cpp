#include "udValue.h"
#include "udPlatform.h"

#define CONTENT_MEMBER "content"
#define DEFAULT_DOUBLE_TOSTRING_PRECISION 6  // This is the printf default

const udValue udValue::s_void;
const size_t udValue::s_udValueTypeSize[T_Count] =
{
  0, // T_Void = 0,      // Guaranteed to be zero, thus a non-zero type indicates value exists
  1, // T_Bool,
  8, // T_Int64,
  8, // T_Double,
  sizeof(char*), // T_String,
  sizeof(udValueArray*), // T_Array,
  sizeof(udValueObject*), // T_Object,
};

// The XML escape character set. Note: apos MUST come first, it is ignored when writing strings that use double quotes
static const char *s_pXMLEscStrings[] = { "&apos;", "&amp;" , "&quot;", "&lt;", "&gt;"};
static const char *s_xmlEscChars = "\'&\"<>";

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
// Very small expression parsing helper
class udJSONExpression
{
public:
  char op, nextOp;
  const char *pKey;
  char *pRemainingExpression;
  bool isExpression;

  void Init(char *pKeyExpression)
  {
    pRemainingExpression = const_cast<char*>(udStrSkipWhiteSpace(pKeyExpression));
    nextOp = 0;
    if (*pRemainingExpression == '[' || *pRemainingExpression == '.')
    {
      nextOp = *pRemainingExpression;
      pRemainingExpression = const_cast<char*>(udStrSkipWhiteSpace(pRemainingExpression + 1));
    }
    Next();
  }
  void InitKeyOnly(const char *pKeyExpression)
  {
    nextOp = op = 0;
    pKey = pKeyExpression; // We know in this case we won't modify
    pRemainingExpression = nullptr;
  }
  void Next()
  {
    op = nextOp;
    nextOp = 0;
    char *pTemp = pRemainingExpression;
    if (pTemp)
    {
      pRemainingExpression = const_cast<char*>(udStrchr(pTemp, ".["));
      if (pRemainingExpression)
      {
        nextOp = *pRemainingExpression;
        *pRemainingExpression++ = 0;
      }
      // Trim trailing space
      size_t i = udStrlen(pTemp);
      while (i > 0 && (pTemp[i - 1] == ' ' || pTemp[i - 1] == '\t' || pTemp[i - 1] == '\r' || pTemp[i - 1] == '\n'))
        pTemp[--i] = 0;
    }
    // Skip leading space and assign to pKey
    pKey = udStrSkipWhiteSpace(pTemp); // udStrSkipWhiteSpace handles nulls gracefully
  }
};


// ****************************************************************************
// Author: Dave Pevreal, April 2017
void udValue::Destroy()
{
  if (type == T_String)
  {
    udFree(u.pStr);
  }
  else if (type == T_Object)
  {
    for (size_t i = 0; i < u.pObject->length; ++i)
    {
      udValueKVPair *pItem = u.pObject->GetElement(i);
      udFree(pItem->pKey);
      pItem->value.Destroy();
    }
    u.pObject->Deinit();
    udFree(u.pObject);
  }
  else if (type == T_Array)
  {
    for (size_t i = 0; i < u.pArray->length; ++i)
    {
      udValue *pChild = u.pArray->GetElement(i);
      if (pChild)
        pChild->Destroy();
    }
    u.pArray->Deinit();
    udFree(u.pArray);
  }

  type = T_Void;
  u.i64Val = 0;
  dPrec = 0;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::SetString(const char *pStr, size_t charCount)
{
  Destroy();
  u.pStr = (charCount) ? udStrndup(pStr, charCount) : udStrdup(pStr);
  if (u.pStr)
  {
    type = T_String;
    return udR_Success;
  }
  return udR_MemoryAllocationFailure;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::SetArray()
{
  udResult result;
  udValueArray *pTempArray = nullptr;
  Destroy();

  pTempArray = udAllocType(udValueArray, 1, udAF_Zero);
  UD_ERROR_NULL(pTempArray, udR_MemoryAllocationFailure);
  result = pTempArray->Init(32);
  UD_ERROR_HANDLE();

  type = T_Array;
  u.pArray = pTempArray;
  pTempArray = nullptr;
  result = udR_Success;

epilogue:
  udFree(pTempArray);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::SetObject()
{
  udResult result;
  udValueObject *pTempObject = nullptr;
  Destroy();

  pTempObject = udAllocType(udValueObject, 1, udAF_Zero);
  UD_ERROR_NULL(pTempObject, udR_MemoryAllocationFailure);
  result = pTempObject->Init(32);
  UD_ERROR_HANDLE();

  type = T_Object;
  u.pObject = pTempObject;
  pTempObject = nullptr;
  result = udR_Success;

epilogue:
  udFree(pTempObject);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::Set(const udDouble3 &v)
{
  udResult result;
  Destroy();
  result = SetArray();
  UD_ERROR_HANDLE();
  for (size_t i = 0; i < v.ElementCount; ++i)
    u.pArray->PushBack(udValue(v[i]));

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::Set(const udDouble4 &v)
{
  udResult result;
  Destroy();
  result = SetArray();
  UD_ERROR_HANDLE();
  for (size_t i = 0; i < v.ElementCount; ++i)
    u.pArray->PushBack(udValue(v[i]));

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::Set(const udQuaternion<double> &v)
{
  udResult result;
  Destroy();
  result = SetArray();
  UD_ERROR_HANDLE();
  for (size_t i = 0; i < v.ElementCount; ++i)
    u.pArray->PushBack(udValue(v[i]));

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::Set(const udDouble4x4 &v, bool shrink)
{
  udResult result = udR_Success;
  size_t elements = 16;
  if (type != T_Array || u.pArray->length != 0)
  {
    Destroy();
    result = SetArray();
    UD_ERROR_HANDLE();
  }
  if (shrink)
  {
    if ((v.axis.x.w == 0.0) && (v.axis.y.w == 0.0) && (v.axis.z.w == 0.0) && (v.axis.t.w == 1.0))
    {
      elements = 12;
      if ((v.axis.t.x == 0.0) && (v.axis.t.y == 0.0) && (v.axis.t.z == 0.0))
        elements = 9;
    }
  }
  switch (elements)
  {
    case 9:
    case 12:
      for (size_t i = 0; i < elements; ++i)
      {
        result = u.pArray->PushBack(udValue(v.a[(i / 3) * 4 + (i % 3)]));
        UD_ERROR_HANDLE();
      }
      break;
    default:
      for (size_t i = 0; i < elements; ++i)
      {
        result = u.pArray->PushBack(udValue(v.a[i]));
        UD_ERROR_HANDLE();
      }
      break;
  }
epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, June 2017
const udValue *udValue::FindMember(const char *pMemberName, size_t *pIndex) const
{
  size_t i;
  udValueObject *pObject = AsObject();
  for (i = 0; pObject && i < pObject->length; ++i)
  {
    const udValueKVPair *pItem = pObject->GetElement(i);
    if (udStrEqual(pItem->pKey, pMemberName))
    {
      if (pIndex)
        *pIndex = i;
      return &pItem->value;
    }
  }
  return nullptr;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
bool udValue::IsEqualTo(const udValue &other) const
{
  // Simple case of actual complete binary equality
  if (type == other.type && u.i64Val == other.u.i64Val)
    return true;
  if (IsNumeric() && other.IsNumeric())
    return AsDouble() == other.AsDouble();
  if (IsString())
    return udStrEqual(AsString(), other.AsString());
  // TODO: Consider whether we want to add test for Array and Object types
  return false;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
bool udValue::AsBool(bool defaultValue) const
{
  switch (type)
  {
    case T_Bool:    return u.bVal;
    case T_Int64:   return u.i64Val != 0;
    case T_Double:  return u.dVal >= 1.0;
    case T_String:  return udStrEquali(u.pStr, "true") || udStrAtof(u.pStr) >= 1.0;
    default:
      return defaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
int udValue::AsInt(int defaultValue) const
{
  switch (type)
  {
    case T_Bool:    return (int)u.bVal;
    case T_Int64:   return (int)u.i64Val;
    case T_Double:  return (int)u.dVal;
    case T_String:  return      udStrAtoi(u.pStr);
    default:
      return defaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
int64_t udValue::AsInt64(int64_t defaultValue) const
{
  switch (type)
  {
    case T_Bool:    return (int64_t)u.bVal;
    case T_Int64:   return          u.i64Val;
    case T_Double:  return (int64_t)u.dVal;
    case T_String:  return          udStrAtoi64(u.pStr);
    default:
      return defaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
float udValue::AsFloat(float defaultValue) const
{
  switch (type)
  {
    case T_Bool:    return (float)u.bVal;
    case T_Int64:   return (float)u.i64Val;
    case T_Double:  return (float)u.dVal;
    case T_String:  return        udStrAtof(u.pStr);
    default:
      return defaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
double udValue::AsDouble(double defaultValue) const
{
  switch (type)
  {
    case T_Bool:    return (double)u.bVal;
    case T_Int64:   return (double)u.i64Val;
    case T_Double:  return         u.dVal;
    case T_String:  return         udStrAtof64(u.pStr);
    default:
      return defaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
const char *udValue::AsString(const char *pDefaultValue) const
{
  switch (type)
  {
    case T_Bool:    return u.bVal ? "true" : "false";
    case T_String:  return u.pStr;
    default:
      return pDefaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udDouble3 udValue::AsDouble3() const
{
  udDouble3 ret = udDouble3::zero();
  if (type == T_Array && u.pArray->length >= ret.ElementCount)
  {
    for (size_t i = 0; i < ret.ElementCount; ++i)
      ret[i] = u.pArray->GetElement(i)->AsDouble();
  }
  return ret;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udDouble4 udValue::AsDouble4() const
{
  udDouble4 ret = udDouble4::zero();
  if (type == T_Array && u.pArray->length >= ret.ElementCount)
  {
    for (size_t i = 0; i < ret.ElementCount; ++i)
      ret[i] = u.pArray->GetElement(i)->AsDouble();
  }
  return ret;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udQuaternion<double> udValue::AsQuaternion() const
{
  udQuaternion<double> ret = udQuaternion<double>::identity();
  if (type == T_Array && u.pArray->length >= ret.ElementCount)
  {
    for (size_t i = 0; i < ret.ElementCount; ++i)
      ret[i] = u.pArray->GetElement(i)->AsDouble();
  }
  return ret;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udDouble4x4 udValue::AsDouble4x4() const
{
  udDouble4x4 ret = udDouble4x4::identity();
  switch (ArrayLength())
  {
    case 9:
    case 12:
      for (size_t i = 0; i < u.pArray->length; ++i)
        ret.a[(i / 3) * 4 + (i % 3)] = u.pArray->GetElement(i)->AsDouble();
      return ret;
    case 16:
      for (size_t i = 0; i < u.pArray->length; ++i)
        ret.a[i] = u.pArray->GetElement(i)->AsDouble();
      return ret;
    default:
      return ret.identity();
  }
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
static size_t FindMatch(const udValueArray *pArray, const udValueObject *pSearchObj, int resultNumber, size_t i = 0)
{
  size_t attributesMatched = 0;
  for (; i < pArray->length; ++i) // For each element in the array
  {
    attributesMatched = 0;
    const udValueObject *pCurrent = const_cast<udValue*>(pArray->GetElement(i))->AsObject();
    if (!pCurrent)
      continue;
    size_t j;
    for (j = 0; (attributesMatched < pSearchObj->length) && (j < pCurrent->length); ++j) // For each attribute in each element
    {
      size_t k;
      for (k = 0; k < pSearchObj->length; ++k) // For each attribute in the search expression
      {
        if (udStrEqual(pSearchObj->GetElement(k)->pKey, pCurrent->GetElement(j)->pKey)
          && pSearchObj->GetElement(k)->value.IsEqualTo(pCurrent->GetElement(j)->value))
        {
          ++attributesMatched;
          break;
        }
      }
    }
    if ((attributesMatched == pSearchObj->length) && (resultNumber-- == 0))
    {
      // All attributes in the search object were matched, we have our element
      break;
    }
  }
  return i;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, July 2017
static udResult udValue_ProcessArrayOperator(const udValue *pRoot, const udValue **ppValue, size_t *pIndex, const char *pKey, int *pCharCount, bool createIfNotExist)
{
  // Common code for all array operator access within expressions
  // This function handles object[0], object["memberName"], object[{memberIndex}], array[index]
  udResult result = udR_Success;
  int charCount = 0;

  // First parse the search expression, and the resultNumber. Both are optional.
  udValue searchExp;
  bool resultNumberParsed = false;
  int resultNumber = 0;
  searchExp.Parse(pKey, &charCount);
  if (pKey[charCount] == ',')
  {
    int tempCharCount;
    resultNumber = udStrAtoi(pKey + charCount + 1, &tempCharCount);
    resultNumberParsed = true;
    charCount += tempCharCount + 1;
    UD_ERROR_IF(resultNumber < 0, udR_ParseError);
  }
  UD_ERROR_IF(pKey[charCount] != ']', udR_ParseError);

  if (searchExp.IsObject())
  {
    // Search expression is a JSON object, so we search arrays looking for the key/value pair supplied
    const udValueArray *pArray = pRoot->AsArray();
    UD_ERROR_NULL(pArray, udR_ObjectTypeMismatch);
    size_t i = FindMatch(pArray, searchExp.AsObject(), resultNumber);
    if (i < pArray->length)
    {
      if (ppValue)
        *ppValue = pArray->GetElement(i);
      if (pIndex)
        *pIndex = i;
    }
    else
    {
      *ppValue = nullptr;
      UD_ERROR_SET(udR_ObjectNotFound);
    }
  }
  else if (searchExp.IsString())
  {
    // Search expression is a string, currently this is only support for finding a member of an object
    UD_ERROR_IF(resultNumberParsed, udR_ParseError);
    if (pRoot->IsVoid() && createIfNotExist)
    {
      result = const_cast<udValue*>(pRoot)->SetObject();
      UD_ERROR_HANDLE();
    }
    if (pRoot->IsObject())
    {
      const udValue *pV = pRoot->FindMember(searchExp.AsString(), pIndex);
      if (!pV && createIfNotExist)
      {
        udValueKVPair *pKVP = pRoot->AsObject()->PushBack();
        UD_ERROR_NULL(pKVP, udR_MemoryAllocationFailure);
        pKVP->pKey = searchExp.AsString();
        searchExp.Clear(); // We're taking the string memory
        pKVP->value.Clear();
        pV = &pKVP->value;
      }
      if (ppValue)
        *ppValue = pV;
      UD_ERROR_NULL(pV, udR_ObjectNotFound);
    }
    else
    {
      UD_ERROR_SET(udR_ObjectTypeMismatch); // Don't currently support ["string"] on arrays
    }
  }
  else if (searchExp.IsVoid())
  {
    // Search expression is void, this means either appending to an array, or indexing a member numerically
    if (resultNumberParsed)
    {
      // [,n] which indicates member index
      UD_ERROR_IF(!pRoot->IsObject(), udR_ObjectTypeMismatch);
      UD_ERROR_IF(resultNumber < 0 || (size_t)resultNumber >= pRoot->AsObject()->length, udR_ObjectNotFound);
      const udValue *pV = &pRoot->AsObject()->GetElement(resultNumber)->value;
      if (ppValue)
        *ppValue = pV;
      if (pIndex)
        *pIndex = resultNumber;
    }
    else
    {
      // [], which means append to the array
      if (pRoot->IsVoid() && createIfNotExist) // If the entry is void, we can safely make it an array now
      {
        result = const_cast<udValue*>(pRoot)->SetArray();
        UD_ERROR_HANDLE();
      }
      UD_ERROR_IF(!pRoot->IsArray() || !createIfNotExist, udR_ParseError);
      udValue *pV = pRoot->AsArray()->PushBack();
      UD_ERROR_NULL(pV, udR_MemoryAllocationFailure);
      pV->Clear();
      if (ppValue)
        *ppValue = pV;
      if (pIndex)
        *pIndex = pRoot->AsArray()->length - 1;
    }
  }
  else if (searchExp.IsNumeric())
  {
    if (pRoot->IsVoid() && createIfNotExist) // If the entry is void, we can safely make it an array now
    {
      result = const_cast<udValue*>(pRoot)->SetArray();
      UD_ERROR_HANDLE();
    }
    // Search expression is numeric, which is typically an array index, but we also support [0] on an object which returns itself
    UD_ERROR_IF(resultNumberParsed, udR_ParseError);
    int searchIndex  = searchExp.AsInt();
    if (pRoot->IsObject())
    {
      UD_ERROR_IF(searchIndex != 0, udR_ParseError); // We accept objects being accessed as object[0] for xml parsing reasons
      if (ppValue)
        *ppValue = pRoot;
      if (pIndex)
        *pIndex = 0;
    }
    else if (pRoot->IsArray())
    {
      if (searchIndex < 0)
        searchIndex += (int)pRoot->AsArray()->length;
      // TODO: Simplify this if/when udChunkedArray does range checking
      udValue *pV = (size_t(searchIndex) < pRoot->AsArray()->length) ? pRoot->AsArray()->GetElement(searchIndex) : nullptr;
      if (!pV && createIfNotExist)
      {
        while (pRoot->AsArray()->length <= (size_t)searchIndex)
        {
          pV = pRoot->AsArray()->PushBack();
          UD_ERROR_NULL(pV, udR_MemoryAllocationFailure);
          pV->Clear();
        }
      }
      if (ppValue)
        *ppValue = pV;
      if (pIndex)
        *pIndex = searchIndex;
      UD_ERROR_NULL(pV, udR_CountExceeded);
    }
    else
    {
      UD_ERROR_SET(udR_ParseError);
    }
  }
  else
  {
    UD_ERROR_SET(udR_ParseError);
  }

epilogue:
  if (pCharCount)
    *pCharCount = charCount;
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
static udResult udJSON_GetVA(const udValue *pRoot, udValue **ppValue, const char *pKeyExpression, va_list ap)
{
  udResult result;
  char *pDup = nullptr; // Allocated string for sprintf'd expression
  udJSONExpression exp;

  UD_ERROR_NULL(pRoot, udR_InvalidParameter_);
  UD_ERROR_NULL(pKeyExpression, udR_InvalidParameter_);
  if (udStrchr(pKeyExpression, "%.["))
  {
    va_list apTemp;
    va_copy(apTemp, ap);
    size_t expressionLength = udSprintfVA(nullptr, 0, pKeyExpression, apTemp);
    va_end(apTemp);
    pDup = udAllocType(char, expressionLength + 1, udAF_None);
    UD_ERROR_NULL(pDup, udR_MemoryAllocationFailure);
    udSprintfVA(pDup, expressionLength + 1, pKeyExpression, ap);
    exp.Init(pDup);
  }
  else
  {
    exp.InitKeyOnly(pKeyExpression);
  }

  for (; exp.pKey; exp.Next())
  {
    switch (exp.op)
    {
      case '[':
        {
          int charCount;
          size_t index;
          result = udValue_ProcessArrayOperator(pRoot, &pRoot, &index, exp.pKey, &charCount, false);
          UD_ERROR_HANDLE();
        }
        break;
      case 0:
        // Fall-thru to normal member-of code if root isn't XML
      case '.':
        {
          pRoot = pRoot->FindMember(exp.pKey);
          if (!pRoot)
            UD_ERROR_SET_NO_BREAK(udR_ObjectNotFound);
        }
        break;
    }
  }

  if (ppValue)
    *ppValue = const_cast<udValue*>(pRoot);
  result = udR_Success;

epilogue:
  udFree(pDup);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Get(udValue **ppValue, const char *pKeyExpression, ...)
{
  va_list ap;
  va_start(ap, pKeyExpression);
  udResult result = udJSON_GetVA(this, ppValue, pKeyExpression, ap);
  va_end(ap);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
const udValue &udValue::Get(const char * pKeyExpression, ...) const
{
  udValue *pValue = nullptr;
  va_list ap;
  va_start(ap, pKeyExpression);
  udResult result = udJSON_GetVA(this, &pValue, pKeyExpression, ap);
  va_end(ap);
  return (result == udR_Success) ? *pValue : udValue::s_void;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
static udResult udJSON_SetVA(udValue *pRoot, udValue *pSetToValue, const char *pKeyExpression, va_list ap)
{
  udResult result;
  char *pDup = nullptr; // Allocated string for sprintf'd expression
  udJSONExpression exp;
  udValue setToValueFromOperator;

  UD_ERROR_NULL(pRoot, udR_InvalidParameter_);
  UD_ERROR_NULL(pKeyExpression, udR_InvalidParameter_);

  if (udStrchr(pKeyExpression, "%.[="))
  {
    va_list apTemp;
    va_copy(apTemp, ap);
    size_t expressionLength = udSprintfVA(nullptr, 0, pKeyExpression, apTemp);
    va_end(apTemp);
    pDup = udAllocType(char, expressionLength + 1, udAF_None);
    UD_ERROR_NULL(pDup, udR_MemoryAllocationFailure);
    udSprintfVA(pDup, expressionLength + 1, pKeyExpression, ap);
    // Parse the assignment result here before initialising exp
    char *pEquals = const_cast<char*>(udStrchr(pDup, "="));
    if (pEquals)
    {
      UD_ERROR_IF(pSetToValue != nullptr, udR_InvalidConfiguration);
      *pEquals++ = 0;
      if (*pEquals)
      {
        result = setToValueFromOperator.Parse(pEquals);
        UD_ERROR_HANDLE();
        pSetToValue = &setToValueFromOperator;
      }
    }
    exp.Init(pDup);
  }
  else
  {
    exp.InitKeyOnly(pKeyExpression);
  }

  for (; pRoot && exp.pKey; exp.Next())
  {
    switch (exp.op)
    {
      case '[':
        {
          int charCount;
          udValue *pV = nullptr;
          size_t index;
          result = udValue_ProcessArrayOperator(pRoot, const_cast<const udValue**>(&pV), &index, exp.pKey, &charCount, pSetToValue != nullptr);
          if (!pSetToValue && !exp.pRemainingExpression)
          {
            UD_ERROR_NULL(pV, udR_ObjectNotFound);
            // End of the expression with no set, means remove the item
            if (pRoot->IsObject())
            {
              udValueKVPair *pItem = pRoot->AsObject()->GetElement(index);
              udFree(pItem->pKey);
              pItem->value.Destroy();
              pRoot->AsObject()->RemoveAt(index);
            }
            else if (pRoot->IsArray())
            {
              pV->Destroy();
              pRoot->AsArray()->RemoveAt(index);
            }
          }
          else
          {
            pRoot = pV;
            UD_ERROR_NULL(pRoot, udR_ParseError);
          }
        }
        break;
      case '.':
        // Fall through to default case
      default: // op == 0 here
      {
        if (pRoot->IsVoid() && pSetToValue)
        {
          // A void key can be converted into an object automatically
          result = pRoot->SetObject();
          UD_ERROR_HANDLE();
        }
        // Check that the type is an object allowing a dot dereference to be valid
        UD_ERROR_IF(!pRoot->IsObject(), udR_ObjectTypeMismatch);
        udValueObject *pObject = pRoot->AsObject();
        const udValue *pV = nullptr;
        size_t index;
        pV = pRoot->FindMember(exp.pKey, &index);
        if (!pV && pSetToValue)
        {
          // A key doesn't exist, and we've got a value to set so create it here
          udValueKVPair *pKVP = pObject->PushBack();
          UD_ERROR_NULL(pKVP, udR_MemoryAllocationFailure);
          pKVP->value.Clear();
          pKVP->pKey = udStrdup(exp.pKey);
          UD_ERROR_NULL(pKVP->pKey, udR_MemoryAllocationFailure);
          pV = &pKVP->value;
        }
        if (!pSetToValue && !exp.pRemainingExpression)
        {
          UD_ERROR_NULL(pV, udR_ObjectNotFound);
          // Found the member, and it needs to be removed
          udValueKVPair *pKVP = pObject->GetElement(index);
          udFree(pKVP->pKey);
          pKVP->value.Destroy();
          pObject->RemoveAt(index);
          pV = nullptr;
        }
        pRoot = const_cast<udValue *>(pV);
        break;
      }
    }
  }
  UD_ERROR_IF(!pRoot && exp.pRemainingExpression, udR_ParseError);
  if (pSetToValue)
  {
    UD_ERROR_NULL(pRoot, udR_InternalError);
    pRoot->Destroy();
    *pRoot = *pSetToValue;
    pSetToValue->Clear(); // Clear it out without destroying
  }
  result = udR_Success;

epilogue:
  udFree(pDup);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Set(const char *pKeyExpression, ...)
{
  va_list ap;
  va_start(ap, pKeyExpression);
  udResult result = udJSON_SetVA(this, nullptr, pKeyExpression, ap);
  va_end(ap);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Set(udValue *pValue, const char *pKeyExpression, ...)
{
  va_list ap;
  va_start(ap, pKeyExpression);
  udResult result = udJSON_SetVA(this, pValue, pKeyExpression, ap);
  va_end(ap);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Parse(const char *pString, int *pCharCount, int *pLineNumber)
{
  udResult result;
  int tempLineNumber;
  int totalCharCount = 0;
  UD_ERROR_NULL(pString, udR_InvalidParameter_);
  if (!pLineNumber)
    pLineNumber = &tempLineNumber;
  if (pLineNumber)
    *pLineNumber = 1;

  Destroy();
  pString = udStrSkipWhiteSpace(pString, nullptr, &totalCharCount);
  if (*pString == '{' || *pString == '[')
  {
    int charCount;
    result = ParseJSON(pString, &charCount, pLineNumber);
    UD_ERROR_HANDLE();
    totalCharCount += charCount;
  }
  else if (*pString == '<')
  {
    int charCount;
    result = ParseXML(pString, &charCount, pLineNumber);
    UD_ERROR_HANDLE();
    totalCharCount += charCount;
  }
  else if (*pString == '\"' || *pString == '\'') // Allow single quotes
  {
    size_t endPos = udStrMatchBrace(pString, '\\');
    // Force a parse error if the string isn't quoted properly
    UD_ERROR_IF(pString[endPos - 1] != pString[0], udR_ParseError);
    char *pStr = udAllocType(char, endPos, udAF_None);
    UD_ERROR_NULL(pStr, udR_MemoryAllocationFailure);
    size_t di = 0;
    for (size_t si = 1; si < (endPos - 1); ++si)
    {
      if (pString[si] == '\\')
      {
        switch (pString[++si])
        {
          case 'a': pStr[di++] = '\a'; break;
          case 'b': pStr[di++] = '\b'; break;
          case 'e': pStr[di++] =   27; break; // GCC extension for escape character
          case 'f': pStr[di++] = '\f'; break;
          case 'n': pStr[di++] = '\n'; break;
          case 'r': pStr[di++] = '\r'; break;
          case 't': pStr[di++] = '\t'; break;
          case 'v': pStr[di++] = '\v'; break;
          case '\\': pStr[di++] = '\\'; break;
          case '\'': pStr[di++] = '\''; break;
          case '\"': pStr[di++] = '\"'; break;
          default:
            // Any escape sequence not recognised is output verbatim (eg \P remains \P)
            pStr[di++] = '\\';
            pStr[di++] = pString[si];
        }
      }
      else
      {
        pStr[di++] = pString[si];
      }
      UDASSERT(di < endPos, "string length miscalculation");
    }
    pStr[di] = 0;
    type = T_String;
    u.pStr = pStr;
    totalCharCount += (int)endPos;
  }
  else
  {
    int charCount = 0;
    int64_t i = udStrAtoi64(pString, &charCount);
    if (charCount)
    {
      if (pString[charCount] == '.')
      {
        int integralCharCount = charCount;
        u.dVal = udStrAtof64(pString, &charCount);
        dPrec = (uint8_t)(charCount - (integralCharCount+1));
        type = T_Double;
      }
      else
      {
        u.i64Val = i;
        type = T_Int64;
      }
    }
    else if (udStrBeginsWithi(pString, "true"))
    {
      charCount = 4;
      u.bVal = true;
      type = T_Bool;
    }
    else if (udStrBeginsWithi(pString, "false"))
    {
      charCount = 5;
      u.bVal = false;
      type = T_Bool;
    }
    else if (udStrBeginsWithi(pString, "null"))
    {
      charCount = 4;
      u.i64Val = 0;
      type = T_Void;
    }
    else
    {
      UD_ERROR_SET(udR_ParseError);
    }
    totalCharCount += charCount;
  }
  result = udR_Success;

epilogue:
  if (pCharCount)
    *pCharCount = totalCharCount;
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
udResult udValue::ToString(const char **ppStr, int indent, const char *pPre, const char *pPost, const char *pQuote, int escape) const
{
  udResult result;
  char *pEscaped = nullptr;

  switch (type)
  {
    case T_Void:    result = udSprintf(ppStr, "%*s%snull%s",     indent, "", pPre, pPost); break;
    case T_Bool:    result = udSprintf(ppStr, "%*s%s%s%s%s%s",   indent, "", pPre, pQuote, u.bVal ? "true" : "false", pQuote, pPost); break;
    case T_Int64:   result = udSprintf(ppStr, "%*s%s%s%lld%s%s", indent, "", pPre, pQuote, u.i64Val, pQuote, pPost); break;
    case T_Double:  result = udSprintf(ppStr, "%*s%s%s%.*lf%s%s",  indent, "", pPre, pQuote, dPrec ? dPrec : DEFAULT_DOUBLE_TOSTRING_PRECISION, u.dVal, pQuote, pPost); break;
    case T_String:
      if (escape == 1) // JSON level escape (just backslashes)
      {
        pEscaped = const_cast<char*>(udStrEscape(u.pStr, "\"\\", false)); // Escape quotes and slashes
        UD_ERROR_NULL(pEscaped, udR_MemoryAllocationFailure);
      }
      else if (escape == 2) // XML escapes
      {
        size_t newSize = udStrlen(u.pStr) + 1;
        size_t strCharIndex = 0; // Index in the string of the special character
        size_t escCharIndex = 0; // Index in the escaped character list string
        const char *p = u.pStr;
        do
        {
          // NOTE: xmlEscChars are +1 in places to ignore the &apos; (single quote)
          udStrchr(u.pStr, s_xmlEscChars+1, &strCharIndex, &escCharIndex);
          if (p[strCharIndex])
          {
            newSize += udStrlen(s_pXMLEscStrings[1+escCharIndex]) - 1;
            ++strCharIndex; // Skip the actual character we just escaped
          }
          p += strCharIndex;
        } while (*p);

        pEscaped = udAllocType(char, newSize, udAF_None);
        UD_ERROR_NULL(pEscaped, udR_MemoryAllocationFailure);
        newSize = 0;
        p = u.pStr;
        do
        {
          udStrchr(p, s_xmlEscChars+1, &strCharIndex, &escCharIndex);
          memcpy(pEscaped + newSize, p, strCharIndex);
          newSize += strCharIndex;
          if (p[strCharIndex])
          {
            size_t l = udStrlen(s_pXMLEscStrings[1+escCharIndex]);
            memcpy(pEscaped + newSize, s_pXMLEscStrings[1+escCharIndex], l);
            newSize += l;
            ++strCharIndex; // Skip the actual character we just escaped
          }
          p += strCharIndex;
        } while (*p);
        pEscaped[newSize] = 0; // Terminate
      }
      result = udSprintf(ppStr, "%*s%s%s%s%s%s", indent, "", pPre, pQuote, pEscaped ? pEscaped : u.pStr, pQuote, pPost);
      break;
    default:
      result = udR_InvalidConfiguration;
  }

epilogue:
  udFree(pEscaped);
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
udResult udValue::ExportJSON(const char *pKey, udValue::LineList *pLines, int indent, bool strip, bool comma) const
{
  udResult result = udR_Success;
  const char *pStr = nullptr;
  static char pEmpty[] = "";
  const char *pComma = (comma) ? "," : pEmpty;
  const char *pKeyText = nullptr;

  if (strip)
    indent = 0;

  if (pKey)
      result = udSprintf(&pKeyText, "\"%s\":%s", pKey, (strip) ? "" : " ");
  else
    pKeyText = pEmpty;

  switch (type)
  {
    case T_Void:
    case T_Bool:
    case T_Int64:
    case T_Double:
    case T_String:
      result = ToString(&pStr, indent, pKeyText, pComma, (type == T_String) ? "\"" : "", 1);
      break;

    case T_Array:
      {
        udValueArray *pArray = AsArray();
        result = udSprintf(&pStr, "%*s%s%s", indent, "", pKeyText, pArray->length ? "[" : comma ? "[]," : "[]");
        UD_ERROR_HANDLE();
        result = pLines->PushBack(pStr);
        UD_ERROR_HANDLE();
        pStr = nullptr;

        if (pArray->length)
        {
          for (size_t i = 0; i < pArray->length; ++i)
          {
            result = pArray->GetElement(i)->ExportJSON(nullptr, pLines, indent + 2, strip, i < (pArray->length - 1));
            UD_ERROR_HANDLE();
          }
          result = udSprintf(&pStr, "%*s]%s", indent, "", pComma);
        }
      }
      break;

    case T_Object:
      {
        udValueObject *pObject = AsObject();
        result = udSprintf(&pStr, "%*s%s{", indent, "", pKeyText);
        UD_ERROR_HANDLE();
        result = pLines->PushBack(pStr);
        UD_ERROR_HANDLE();
        pStr = nullptr;
        for (size_t i = 0; i < pObject->length; ++i)
        {
          udValueKVPair *pItem = pObject->GetElement(i);
          result = pItem->value.ExportJSON(pItem->pKey, pLines, indent + 2, strip, i < (pObject->length - 1));
          UD_ERROR_HANDLE();
        }
        result = udSprintf(&pStr, "%*s}%s", indent, "", pComma);
      }
      break;

    default:
      UD_ERROR_SET(udR_InternalError);
  }
  UD_ERROR_HANDLE();
  if (pStr)
  {
    result = pLines->PushBack(pStr);
    UD_ERROR_HANDLE();
    pStr = nullptr;
  }
  result = udR_Success;

epilogue:
  udFree(pStr);
  if (pKeyText != pEmpty)
    udFree(pKeyText);
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
udResult udValue::ExportXML(const char *pKey, udValue::LineList *pLines, int indent, bool strip) const
{
  udResult result = udR_Success;
  const char *pStr = nullptr;
  static char pEmpty[] = "";
  const char *pKeyText = nullptr;
  const char *pContentString = nullptr;

  if (strip)
    indent = 0;

  if (pKey)
    result = udSprintf(&pKeyText, "%s%s", pKey, (type == T_Object || type == T_Void) ? "" : "=");
  else
    pKeyText = pEmpty;

  switch (type)
  {
    case T_Void:
      result = udSprintf(&pStr, "%*s<%s/>", indent, "", pKeyText);
      break;
    case T_Bool:
    case T_Int64:
    case T_Double:
    case T_String:
      result = ToString(&pStr, indent, pKeyText, "", "\"", 2);
      break;

    case T_Array:
      {
        udValueArray *pArray = AsArray();
        if (pArray->length == 0)
        {
          // Export empty arrays as an empty tag
          result = udSprintf(&pStr, "%*s<%s></%s>", indent, "", pKey, pKey); break;
        }
        for (size_t i = 0; i < pArray->length; ++i)
        {
          const udValue *pValue = pArray->GetElement(i);
          switch (pValue->type)
          {
            case T_Void:    result = udSprintf(&pStr, "%*s<%s></%s>",     indent, "", pKey, pKey); break;
            case T_Bool:    result = udSprintf(&pStr, "%*s<%s>%s</%s>",   indent, "", pKey, pValue->u.bVal ? "true" : "false", pKey); break;
            case T_Int64:   result = udSprintf(&pStr, "%*s<%s>%lld</%s>", indent, "", pKey, pValue->u.i64Val, pKey); break;
            case T_Double:  result = udSprintf(&pStr, "%*s<%s>%lf</%s>",  indent, "", pKey, pValue->u.dVal, pKey); break;
            case T_String:  result = udSprintf(&pStr, "%*s<%s>%s</%s>", indent, "", pKey, pValue->u.pStr, pKey); break;
            case T_Array:
            case T_Object:
              result = pArray->GetElement(i)->ExportXML(pKey, pLines, indent, strip);
              break;
            default:
              UD_ERROR_SET(udR_Failure_);
          }
          UD_ERROR_HANDLE();
          if (pStr)
          {
            result = pLines->PushBack(pStr);
            UD_ERROR_HANDLE();
            pStr = nullptr;
          }
        }
      }
      break;

    case T_Object:
      {
        udValueObject *pObject = AsObject();
        int subObjectCount = 0, attributeCount = 0;
        const char *pTempStr;

        // First, find out how many simple attributes are present versus subobjects (exported as children)
        for (size_t i = 0; i < pObject->length; ++i)
        {
          const udValueKVPair *pAttribute = pObject->GetElement(i);
          if (pAttribute->value.type == T_Object || pAttribute->value.type == T_Array || pAttribute->value.type == T_Void)
            ++subObjectCount;
          else if (!pContentString && udStrEqual(pAttribute->pKey, CONTENT_MEMBER))
            pAttribute->value.ToString(&pContentString, 0, "", "", "", 2);
          else
            ++attributeCount;
        }

        // Create opening tag, optionally self-closing it if there's nothing else to add to it
        result = udSprintf(&pStr, "%*s<%s%s", indent, "", pKeyText, (!attributeCount && !subObjectCount && !pContentString) ? "/>" : "");
        UD_ERROR_HANDLE();
        // Export all the attributes to a separate list to be combined to a single line
        LineList attributeLines;
        attributeLines.Init(32);
        for (size_t i = 0; i < pObject->length && attributeCount; ++i)
        {
          const udValueKVPair *pAttribute = pObject->GetElement(i);
          if (pAttribute->value.type == T_Object || pAttribute->value.type == T_Array || pAttribute->value.type == T_Void) // Children exported after the attributes
            continue;
          else if (udStrEqual(pAttribute->pKey, CONTENT_MEMBER))
            continue;
          UD_ERROR_IF(pAttribute->value.type < T_Bool || pAttribute->value.type > T_String, udR_ObjectTypeMismatch);
          result = pAttribute->value.ExportXML(pAttribute->pKey, &attributeLines, 0, strip);
          UD_ERROR_HANDLE();
          const char *pAttrText;
          if (attributeLines.PopBack(&pAttrText))
          {
            --attributeCount;
            // Combine the element onto the tag line, appending a closing or self-closing tag as required
            // Complicated a little by injecting the content string if required
            result = udSprintf(&pTempStr, "%s %s%s", pStr, pAttrText,
                                attributeCount ? "" : ((subObjectCount || pContentString) ? ">" : "/>"));
            udFree(pAttrText);
            UD_ERROR_HANDLE();
            udFree(pStr);
            pStr = pTempStr;
            pTempStr = nullptr;

            // Append ">pContentString</close> if no subojects following
            if (!attributeCount && !subObjectCount && pContentString)
            {
              result = udSprintf(&pTempStr, "%s%s</%s>", pStr, pContentString, pKey);
              UD_ERROR_HANDLE();
              udFree(pStr);
              pStr = pTempStr;
            }
          }
        }
        attributeLines.Deinit();
        result = pLines->PushBack(pStr);
        UD_ERROR_HANDLE();
        pStr = nullptr;

        if (subObjectCount)
        {
          for (size_t i = 0; i < pObject->length; ++i)
          {
            const udValueKVPair *pAttribute = pObject->GetElement(i);
            if (pAttribute->value.type != T_Object && pAttribute->value.type != T_Array && pAttribute->value.type != T_Void) // Attributes already exported
              continue;
            result = pAttribute->value.ExportXML(pAttribute->pKey, pLines, indent + 2, strip);
            UD_ERROR_HANDLE();
          }
          if (pContentString)
          {
            // If a content string present with subobjects, tab in in same as they are
            result = udSprintf(&pStr, "%*s%s", indent + 2, "", pContentString);
            pLines->PushBack(pStr);
            pStr = nullptr;
          }
          // Closing tag
          result = udSprintf(&pStr, "%*s</%s>", indent, "", pKeyText);
          UD_ERROR_HANDLE();
        }
      }
      break;

    default:
      UD_ERROR_SET(udR_InternalError);
  }
  UD_ERROR_HANDLE();
  if (pStr)
  {
    result = pLines->PushBack(pStr);
    UD_ERROR_HANDLE();
    pStr = nullptr;
  }
  result = udR_Success;

epilogue:
  udFree(pStr);
  udFree(pContentString);
  if (pKeyText != pEmpty)
    udFree(pKeyText);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Export(const char **ppText, udValueExportOption option) const
{
  udResult result;
  udValue::LineList lines;
  size_t totalChars;
  char *pText = nullptr;

  lines.Init(32);
  UD_ERROR_NULL(ppText, udR_InvalidParameter_);

  if ((option & udVEO_XML))
  {
    if (IsObject())
    {
      for (size_t i = 0; i < AsObject()->length; ++i)
      {
        const udValueKVPair *pItem = AsObject()->GetElement(i);
        result = pItem->value.ExportXML(pItem->pKey, &lines, 0, !!(option & udVEO_StripWhiteSpace));
        UD_ERROR_HANDLE();
      }
    }
    else
    {
      UD_ERROR_SET(udR_InvalidConfiguration);
    }
  }
  else
  {
    result = ExportJSON(nullptr, &lines, 0, !!(option & udVEO_StripWhiteSpace), false);
    UD_ERROR_HANDLE();
  }

  totalChars = 0;
  for (size_t i = 0; i < lines.length; ++i)
    totalChars += udStrlen(lines[i]) + ((option & udVEO_StripWhiteSpace) ? 0 : 2);
  pText = udAllocType(char, totalChars + 1, udAF_Zero);
  totalChars = 0;
  for (size_t i = 0; i < lines.length; ++i)
  {
    size_t len = udStrlen(lines[i]);
    memcpy(pText + totalChars, lines[i], len);
    totalChars += len;
    if (!(option & udVEO_StripWhiteSpace))
    {
      memcpy(pText + totalChars, "\r\n", 2);
      totalChars += 2;
    }
  }
  *ppText = pText;
  pText = nullptr;
  result = udR_Success;

epilogue:
  for (size_t i = 0; i < lines.length; ++i)
    udFree(lines[i]);
  lines.Deinit();
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
udResult udValue::ParseJSON(const char *pJSON, int *pCharCount, int *pLineNumber)
{
  udResult result = udR_Success;
  const char *pStartPointer = pJSON; // Just used to calculate and assign pCharCount
  int charCount;
  int tempLineNumber = 1;
  if (!pLineNumber)
    pLineNumber = &tempLineNumber;

  pJSON = udStrSkipWhiteSpace(pJSON, nullptr, pLineNumber);
  if (*pJSON == '{')
  {
    // Handle an embedded JSON object
    result = SetObject();
    UD_ERROR_HANDLE();
    pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);
    while (*pJSON != '}')
    {
      udValueKVPair *pItem;
      result = AsObject()->PushBack(&pItem);
      UD_ERROR_HANDLE();
      pItem->pKey = nullptr;
      pItem->value.Clear();

      udValue k; // Temporaries
      UD_ERROR_IF(*pJSON != '"' && *pJSON != '\'', udR_ParseError);
      result = k.Parse(pJSON, &charCount); // Use parser to get the key string for convenience
      UD_ERROR_HANDLE();
      UD_ERROR_IF(!k.IsString(), udR_ParseError);
      pJSON = udStrSkipWhiteSpace(pJSON + charCount, nullptr, pLineNumber);
      // Now add the string, taking ownership of the memory from the k temporary
      pItem->pKey = k.AsString();
      k.Clear(); // Clear k without freeing the memory, as it belongs to pItem now

      // Check and move past the colon following the key
      UD_ERROR_IF(*pJSON != ':', udR_ParseError);
      pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);

      // Parse the type, it could be an object, array, or simple type
      result = pItem->value.ParseJSON(pJSON, &charCount, pLineNumber);
      UD_ERROR_HANDLE();
      pJSON = udStrSkipWhiteSpace(pJSON + charCount, nullptr, pLineNumber);

      if (*pJSON != '}' && *pJSON != ',')
        udDebugPrintf("JSON Parse warning: line %d, an extraneous comma found at end of object\n", *pLineNumber);
      if (*pJSON == ',')
        pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);
    }
    ++pJSON; // Skip the final close brace

  }
  else if (*pJSON == '[')
  {
    // Handle an array of values
    result = SetArray();
    UD_ERROR_HANDLE();
    pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);
    while (*pJSON != ']')
    {
      udValue *pNextItem;
      result = AsArray()->PushBack(&pNextItem);
      UD_ERROR_HANDLE();
      pNextItem->Clear();
      result = pNextItem->ParseJSON(pJSON, &charCount, pLineNumber);
      UD_ERROR_HANDLE();
      pJSON = udStrSkipWhiteSpace(pJSON + charCount, nullptr, pLineNumber);
      if (*pJSON != ']' && *pJSON != ',')
        udDebugPrintf("JSON Parse warning: line %d, an extraneous comma found at end of array\n", *pLineNumber);
      if (*pJSON == ',')
        pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);
    }
    pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber); // Skip the closing square bracket
  }
  else
  {
    // Case where the JSON is actually just a value
    result = Parse(pJSON, &charCount);
    if (result == udR_ParseError)
      udDebugPrintf("Error parsing JSON text, line %d: ...%.30s...\n", *pLineNumber, pJSON);
    UD_ERROR_HANDLE();
    pJSON = udStrSkipWhiteSpace(pJSON + charCount, nullptr, pLineNumber);
    UD_ERROR_SET(udR_Success);
  }

epilogue:
  if (pCharCount)
    *pCharCount = int(pJSON - pStartPointer);
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, June 2017
static udResult ParseXMLString(const char **ppStr, const char *pXML, int *pCharCount)
{
  udResult result;
  int charCount = 0;
  char *pStr = nullptr;

  if (*pXML == '\'' || *pXML == '\"')
  {
    charCount = (int)udStrMatchBrace(pXML);
    UD_ERROR_IF(charCount < 2, udR_ParseError);
    pStr = udAllocType(char, charCount - 2 + 1, udAF_None);
    UD_ERROR_NULL(pStr, udR_MemoryAllocationFailure);
    int di = 0;
    for (int si = 1; pXML[si] != *pXML;)
    {
      bool escaped = false;
      for (int e = 0; pXML[si] == '&' && !escaped && e < (int)UDARRAYSIZE(s_pXMLEscStrings); ++e)
      {
        if (udStrBeginsWith(pXML+si, s_pXMLEscStrings[e]))
        {
          pStr[di++] = s_xmlEscChars[e];
          si += (int)udStrlen(s_pXMLEscStrings[e]);
          escaped = true;
        }
      }
      if (!escaped)
        pStr[di++] = pXML[si++];
    }
    pStr[di] = 0; // Terminate
  }
  *ppStr = pStr;
  pStr = nullptr;
  result = udR_Success;

epilogue:
  if (pCharCount)
    *pCharCount = charCount;
  udFree(pStr);
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, May 2017
udResult udValue::ParseXML(const char *pXML, int *pCharCount, int *pLineNumber)
{
  udResult result = udR_Success;
  const char *pStartPointer = pXML; // Just used to calculate and assign pCharCount
  int charCount;
  size_t len;
  const char *pElementName = nullptr;
  udValue *pElement = nullptr;
  bool selfClosingTag;
  int tempLineNumber = 1;
  udValue tempValue; // A temporary here so that in the event of error it will be destroyed
  if (!pLineNumber)
    pLineNumber = &tempLineNumber;

  pXML = udStrSkipWhiteSpace(pXML, nullptr, pLineNumber);
  // Just skip the xml version tag, this is ignored and not retained
  if (udStrBeginsWith(pXML, "<?xml"))
    pXML = udStrSkipWhiteSpace(pXML + udStrMatchBrace(pXML), nullptr, pLineNumber);
  // Skip comment
  if (udStrBeginsWith(pXML, "<!--"))
  {
    pXML = udStrstr(pXML, 0, "-->");
    UD_ERROR_NULL(pXML, udR_ParseError);
    pXML = udStrSkipWhiteSpace(pXML + 3, nullptr, pLineNumber);
  }

  UD_ERROR_IF(*pXML != '<', udR_ParseError);
  if (!IsObject())
  {
    result = SetObject();
    UD_ERROR_HANDLE();
  }
  pXML = udStrSkipWhiteSpace(pXML + 1, nullptr, pLineNumber);

  // Get the element name
  udStrchr(pXML, " />\r\n", &len);
  UD_ERROR_IF(len < 1, udR_ParseError); // Not going to entertain unnamed elements
  pElementName = udStrndup(pXML, len);
  pXML = udStrSkipWhiteSpace(pXML + len, nullptr, pLineNumber);
  // So here we have the name of the element, but it might already exist or not
  pElement = const_cast<udValue*>(FindMember(pElementName));
  if (!pElement)
  {
    // Case where the tag hasn't been encountered before, so create an object for it
    udValueKVPair *pKVP = AsObject()->PushBack();
    UD_ERROR_NULL(pKVP, udR_MemoryAllocationFailure);
    pKVP->pKey = udStrdup(pElementName);
    pKVP->value.Clear();  // Initialise without prior destruction
    pKVP->value.SetObject();
    pElement = &pKVP->value;
  }
  else
  {
    if (!pElement->IsArray())
    {
      // Exactly one of the element has been encountered already, so we must convert it to an array
      tempValue = *pElement;
      pElement->Clear(); // Clear rather than destroy because tempValue is now owning the memory
      result = pElement->SetArray();
      UD_ERROR_HANDLE();
      result = pElement->AsArray()->PushBack(tempValue);
      UD_ERROR_HANDLE();
      tempValue.Clear(); // Clear rather than destroy as it has been pushed onto the array
    }
    // Multiple already encountered, just add another to the end of the array
    pElement = pElement->AsArray()->PushBack();
    UD_ERROR_NULL(pElement, udR_MemoryAllocationFailure);
    pElement->Clear();
    pElement->SetObject();
    UD_ERROR_HANDLE();
  }
  UD_ERROR_IF(!pElement->IsObject(), udR_InternalError); // Just to be sure

  selfClosingTag = false;
  while (*pXML != '>')
  {
    // While still inside the element tag, accept and parse attributes
    udStrchr(pXML, " =/>", &len);
    if (pXML[len] == '=')
    {
      // Found an attribute
      udValueKVPair *pAttr = pElement->AsObject()->PushBack();
      UD_ERROR_NULL(pAttr, udR_MemoryAllocationFailure);
      pAttr->pKey = udStrndup(pXML, len);
      pXML = udStrSkipWhiteSpace(pXML + len + 1, nullptr, pLineNumber);
      pAttr->value.Clear();
      result = ParseXMLString(&pAttr->value.u.pStr, pXML, &charCount);
      UD_ERROR_HANDLE();
      pAttr->value.type = T_String;
      pXML = udStrSkipWhiteSpace(pXML + charCount, nullptr, pLineNumber);
    }
    else if (*pXML == '/')
    {
      selfClosingTag = true;
      pXML = udStrSkipWhiteSpace(pXML + 1, nullptr, pLineNumber);
    }
    else
    {
      UD_ERROR_SET(udR_ParseError);
    }
  }
  pXML = udStrSkipWhiteSpace(pXML + 1, nullptr, pLineNumber); // Skip the >
  if (!selfClosingTag)
  {
    while (*pXML && !udStrBeginsWith(pXML, "</"))
    {
      bool cData = udStrBeginsWith(pXML, "<![CDATA[");
      if (*pXML == '<' && !cData)
      {
        // An embedded tag (a subobject)
        int lineCount;
        // Call parseXML directly because Parse would destroy the element first
        result = pElement->ParseXML(pXML, &charCount, &lineCount);
        UD_ERROR_HANDLE();
        if (pLineNumber)
          *pLineNumber += lineCount - 1;
        pXML = udStrSkipWhiteSpace(pXML + charCount, nullptr, pLineNumber); // Skip </
      }
      else
      {
        // Content string
        if (cData)
        {
          pXML += 9;
          udStrstr(pXML, 0, "]]>", &len);
        }
        else
        {
          udStrchr(pXML, "<>", &len);
          while (len > 1 && (pXML[len] == ' ' || pXML[len] == '\t' || pXML[len] == '\r' || pXML[len] == '\n'))
            --len;
        }
        udValueKVPair *pAttr = pElement->AsObject()->PushBack();
        UD_ERROR_NULL(pAttr, udR_MemoryAllocationFailure);
        pAttr->pKey = udStrdup(CONTENT_MEMBER);
        UD_ERROR_NULL(pAttr->pKey, udR_MemoryAllocationFailure);
        pAttr->value.Clear();
        pAttr->value.u.pStr = udStrndup(pXML, len);
        UD_ERROR_NULL(pAttr->value.u.pStr, udR_MemoryAllocationFailure);
        pAttr->value.type = T_String;
        if (cData)
          len += 3; // Skip the closing ]]>
        pXML = udStrSkipWhiteSpace(pXML + len, nullptr, pLineNumber);
      }
    }
    UD_ERROR_IF(!*pXML, udR_ParseError);
    pXML = udStrSkipWhiteSpace(pXML + 2, nullptr, pLineNumber); // Skip </
    UD_ERROR_IF(!udStrBeginsWith(pXML, pElementName), udR_ParseError); // Check for closing element name
    pXML = udStrSkipWhiteSpace(pXML + udStrlen(pElementName), nullptr, pLineNumber); // Skip </
    UD_ERROR_IF(*pXML != '>', udR_ParseError);
    pXML = udStrSkipWhiteSpace(pXML + 1, nullptr, pLineNumber); // Skip >

    if (pElement->MemberCount() == 1 && udStrEqual(pElement->GetMemberName(0), CONTENT_MEMBER))
    {
      // If the element ended up being only a content string with nothing else, convert it to a value
      udValueKVPair *pItem = pElement->AsObject()->GetElement(0);
      tempValue = pItem->value;
      pItem->value.Clear(); // Clear not destroy as memory now owned by tempValue
      pElement->Destroy();
      *pElement = tempValue;
      tempValue.Clear(); // Clear not destroy as memory now owned by *pElement
    }
    else if (pElement->MemberCount() == 0)
    {
      // Empty arrays are exported as empty tags, so here we apply the reverse to retain the information
      pElement->SetArray();
    }
  }
  else if (pElement->MemberCount() == 0)
  {
    // null members are exported as empty self-closed tags, so here we apply the reverse to retain the information
    pElement->SetVoid();
  }

epilogue:
  tempValue.Destroy();
  udFree(pElementName);
  if (pCharCount)
    *pCharCount = int(pXML - pStartPointer);
  return result;
}
