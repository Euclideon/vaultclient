# udJSON

udJSON class itself simply defines a poly-type variable, with one of the types being an Object, which is a list of key/value pairs, allowing a tree of values to be defined.
The internal representation is that of JSON, however it imports XML and represents many XML files without information loss, via a convention: tag attributes are treated
as JSON members, and the content of a tag is considered to be the member "content" and is of type string.
The tree of values can be exported as JSON or XML, and generally all combinations of export, import, export, import result in the same data.

## Basic usage

To load an existing JSON or XML file:

```cpp
  const char *pText = nullptr;
  udFile_Load("TestData.txt", &pText);
  udJSON v;
  v.Parse(pText);
  udFree(pText);
```

Expression syntax for Get/Set:

* The Get and Set functions take a variable argument list to form a udSprintf'd string prior to parsing, 
  this is purely for convenience. Any legal printf formatting can be used, it is totally irrelevant to
  the Get and Set functions as they only deal with the resulting string.
* The . operator references the member of an object
```cpp
  v.Get("Settings.ProjectsPath").AsString();
```
* The [] operator indexes arrays, or references members of an object in special ways
```cpp
  v.Get("Settings.TestArray[2]").AsInt(); // Simple index of an array accessing the 3rd element
  v.Get("Settings.TestArray[-2]").AsInt(); // Reference the second last element of the array
  v.Set("Settings.TestArray[] = 5"); // Append the value 5 to an array
  v.Get("Settings['TerrainIndex']).AsInt(); // Reference the member name in quotes, useful if it has special characters
  v.Get("Settings[,2]).AsInt(); // Index the members, the 3rd member is TerrainIndex, so this is equivalent to the above line
```
* The = operator is only valid on Set calls, with the type of the data inferred.
  Importantly, valid JSON is allowed on the right also, allowing simple objects to be created very conveniently.
```cpp
  v.Set("Settings.ProjectsPath = '%s'", "C:/Temp/"); // A string type
  v.Set("Settings.ImportAtFullScale = true"); // A boolean type (note the absence of quotes)
  v.Set("Settings.TestArray[] = 2"); // Append a numeric type to an array
  v.Set("Settings.TestArray[] = { 'member': 100, 'something': 200 }"); // Append an object to an array
```
* To remove a key, use set without setting to anything (any children/subkeys are also removed)
```cpp
  v.Set("Settings"); // Remove the Settings key and all it's children
```

To create a udJSON tree from scratch:

```cpp
  udJSON v;
  v.Set("Settings.ProjectsPath = '%s'", "C:/Temp/"); <-- Note the use of single quotes, this is mainly for convenience to avoid having to escape double-quote characters
  v.Set("Settings.ImportAtFullScale = true"); <-- NOTE: true without quotes is boolean if it were in quotes it would be a string (but still work)
  v.Set("Settings.TerrainIndex = %d", 2);
  v.Set("Settings.Inside.Count = %d", 5);
  v.Set("Settings.TestArray[] = 0");
  v.Set("Settings.TestArray[] = 1");
  v.Set("Settings.TestArray[] = 2");
```

To export:
```cpp
  const char *pText = nullptr;
  v.Export(pText, udJEO_JSON); // Add | udJEO_FormatWhiteSpace for optional human-readable output
  // .. write text to a file or whatever
  udFree(pText);

  // Alternatively, to write XML
  v.Export(pText, udJEO_XML);
```

To retrieve a value:

```cpp
  printf("The projects path is %s\n", v.Get("Settings.ProjectsPath").AsString());
  if (v.Get("Settings.ImportAtFullScale").AsBool())
    printf("Importing at full scale\n");
```

Note that Get and Set internally use udSprintf. So v.Get("%s.%s", "hello", "world") is identical to v.Get("hello.world"),
however being able to construct Get/Set expressions with variable argument printf syntax is extremely convenient.

For example (note this is inefficient for brief demonstration):

```cpp
for (size_t i = 0; i < v.Get("Settings.TestArray").ArrayLength(); ++i)
  printf("TestArray[%d] = %d\n", v.Get("Settings.TestArray[%d]", i).AsInt());
```

Keys are created when they are assigned to, so the following is valid:

```cpp
udJSON v;
v.Set("array[0].person.name = 'bob'");
v.Set("array[0].person.age = 21");
```

## Types

Internally a udJSON will be stored as one of the following types:

  * T_Void (a JSON null maps to this)
  * T_Bool
  * T_Int64
  * T_Double
  * T_String (const char *)
  * T_Array (udChunkedArray of udJSONs)
  * T_Object (udChunkdArray of key/value pairs)

### T_Void

The void type is the default type of a constructed udJSON. When exported to JSON it will be "null", or to XML it will be a self-closed tag with no attributes.

Get/Set syntax:

```cpp
v.Set("key = null");
v.Get("key").IsVoid() == true
```

Relevant methods:

  * udJSON::IsVoid() - Test is type is T_Void
  * udJSON::SetVoid() - Destroy the udJSON and set to void type
  * udJSON::Destroy() - Free any memory associated with type (eg a string) and set type back to void
  * udJSON::Clear() - Initialise the type to void assuming the class is currently uninitialised
  * udJSON::ToString() - Will yield "null"

### T_Bool

When exported to JSON it will be either true or false, unquoted. For XML it will be quoted in an attribute of the tag.

Get/Set syntax:

```cpp
v.Set("key = false");
v.Set("key = true");
v.Get("key").AsBool() == true
```

Relevant methods:

  * udJSON::Set(bool v)
  * udJSON::AsBool() - Numeric types will return true if >= 1.0, string types will return true if equal to the string
                        "true" (ignoring case) or the string is entirely a number >= 1.0 (eg "1.5")
  * udJSON::ToString() - Will yield "true" or "false"

### T_Int64

A signed numeric 64-bit value. There is currently no support for an unsigned 64-bit number.

Get/Set syntax:

```cpp
v.Set("key = -999");
v.Get("key").AsInt64() == -999
```

Relevant methods:

  * udJSON::Set(int64_t v)
  * udJSON::AsInt() - Downcast to the int type for convenience
  * udJSON::AsInt64() - Returns the value
  * udJSON::ToString() - Will yield the equivalent of udStrItoa64

### T_Double

A 64-bit double precision floating point value.

Get/Set syntax:

```cpp
v.Set("key = %f", 3.14159);
v.Get("key").AsDouble() == 3.14159
```

Relevant methods:

  * udJSON::Set(double v)
  * udJSON::AsFloat() - Downcast to the float type for convenience
  * udJSON::AsDouble() - Returns the value
  * udJSON::ToString() - Will yield the equivalent of udStrFtoa

### T_String

A standard C string. There is no legal API to have this string be NULL. Attempting to set the string to NULL will
result in an error code return and the udJSON being of type T_Void. When setting the string a duplicate of the passed
string is taken, and the destructor / Destroy method of udJSON will handle freeing the memory. Taking references to
the string inside a udJSON is fine so long as the caller understands it will be freed when the udJSON changes.

Get/Set syntax:

```cpp
v.Set("key = '3.14159'");
v.Get("key").AsString() == "3.14159"
v.Get("key").AsBool() == true
v.Get("key").AsInt() == 3
v.Get("key").AsFloat() == 3.14159
```

Relevant methods:

  * udJSON::SetString(const char *pStr) - A copy of pStr is taken, if pStr is NULL this will fail and set to T_Void
  * udJSON::AsBool() - Returns true if the string is "true" (ignores case) or is a numeric value >= 1.0
  * udJSON::AsInt() - Returns the result of udStrAtoi
  * udJSON::AsInt64() - Returns the result of udStrAtoi64
  * udJSON::AsFloat() - Returns the result of udStrAtof
  * udJSON::AsDouble() - Returns the result of udStrAtoi64
  * udJSON::ToString() - Will yield the string, optionally providing escape character support
  * udJSON::HasMemory() - Returns true for T_String, T_Array and T_Object (types that allocate memory)

### T_Array

An array of typed values, analoguous to a JSON array. In XML, a series of tags with the same name are parsed as an array.

Currently out-of-bounds assignments are not permitted with the exception being the next element that would be appended. 
This limitation can be lifted easily if use-case requires.

Get/Set syntax:

```cpp
v.Set("key = []"); // Assign the key to be an empty array
v.Set("key[] = 'first');
v.Set("key[] = 'second');    // [ ] is used to append the element to the array
v.Get("key").IsArray() == true
v.Get("key").ArrayLength() == 2
v.Get("key[0]").AsString() == "first"
v.Get("key[1]").AsString() == "first"
```

Relevant methods:

  * udJSON::SetArray() - Force the udJSON object to be a zero-length array
  * udJSON::AsArray() - Returns the udJSONArray pointer, or NULL if not an array
  * udJSON::ArrayLength() - Returns the length of the array, zero if not an array
  * udJSON::HasMemory() - Returns true for T_String, T_Array and T_Object (types that allocate memory)

To iterate an array:

```cpp
// A simple but inefficient way:

for (size_t i = 0; i < v.Get("key").ArrayLength(); ++i)
  printf("key[%d] = %s\n", i, v.Get("key[%d]", i).AtString());

// A more efficient way that accesses the underlying udChunkedArray methods directly:

const udJSONArray *pArray = v.Get("key").AsArray();
for (size_t i = 0; pArray && i < pArray->length; ++i)
  printf("key[%d] = %s\n", i, pArray->GetElement()->AtString());
```

### T_Object

The typical JSON object. Due to the way XML arrays are parsed, a single object can also be treated as an array of length 1.
The object is a udChunkedArray of udJSONKVPair structures, which are just a key string, and a udJSON. Note that in the
Get/Set syntax members can be referenced by index as well as by name.

Get/Set syntax:

```cpp
v.Set("person.name = 'first');
v.Set("person.age = 21);
v.Set("person.dob.day = 25);
v.Set("person.dob.month = 12);
v.Set("person.dob.year = 1980);
v.Get("person.name").AsString() == "first"
v.Get("person.dob.year").AsInt() == 1980
v.Get("person").MemberCount() == 3
// Members can be referenced by index within the structure too
v.Get("person[,0]").AsString() == "first" // First member is index 0
v.Get("person[,2].year").AsInt() == 1980  // Third member is index 2 (which is an object so year member is dereferenced)
// A member can be another object, so it can be returned on its own
v.Get("person.dob").IsObject() == true
v.Get("person.dob").Get("day").AsInt() == 25
// Finally, because in XML arrays of objects are just repeated tags, all objects are able to be treated as an array of length 1
v.Get("person[0].dob[0].month").AsInt() == 12;
```

Relevant methods:

  * udJSON::SetObject() - Force the udJSON object to be an empty object
  * udJSON::AsObject() - Returns the udJSONArray pointer, or NULL if not an array
  * udJSON::MemberCount() - Returns the number of members in the object
  * udJSON::HasMemory() - Returns true for T_String, T_Array and T_Object (types that allocate memory)

To iterate the members of an object:

```cpp
// A simple but not totally efficient way (recommended when performance is not critical)

for (size_t i = 0; i < person.MemberCount(); ++i)
{
  const char *pStr = nullptr;
  person.Get("person.%d", i).ToString(&pStr)
  printf("person.%s = %s\n", v.Get("person").GetMemberName(i), pStr);
  udFree(pStr); // ToString allocates a string that we must free
}

// A slightly more efficient way that accesses the underlying udChunkedArray methods directly:

const udJSONObject *pPerson = v.Get("person").AsObject();
for (size_t i = 0; pPerson && i < pPerson->length; ++i)
{
  udJSONKVPair *pElement = pPerson->GetElement(i);
  const char *pStr = nullptr;
  pElement->value.ToString(&pStr)
  printf("person.%s = %s\n", pElement->pKey, pStr);
  udFree(pStr); // ToString allocates a string that we must free
}

```

## Helpers

* AsDouble3() will return a udDouble3 for an array of 3 (or more) double values.
* AsDouble4() will return a udDouble4 for an array of 4 (or more) double values.
* AsQuaternion() will return a udQuaternion<double> for an array of 4 (or more) double values.
* AsDouble4x4() will return a udDouble4x4 for an array of 9, 12, or 16 doubles. Any other number returns identity.
  9 doubles will populate the inner 3x3, 12 doubles form a 3x4 and 16 forms the full 4x4.
* CalculateHMAC is used to digitally sign the tree
* ExtractAndVoid allows you to take ownership of a string from a udJSON object, setting it to void

