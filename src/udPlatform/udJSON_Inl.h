#ifndef UDJSON_INL_H
#define UDJSON_INL_H

inline udJSON::udJSON()           { Clear(); }
inline udJSON::udJSON(int64_t v)  { type = T_Int64;  u.i64Val = v; dPrec = 0; }
inline udJSON::udJSON(double v)   { type = T_Double; u.dVal   = v; dPrec = 0; }
inline void udJSON::Clear()        { type = T_Void;   u.i64Val = 0; dPrec = 0; } // Clear the value without freeing
inline udJSON::~udJSON()          { Destroy(); }

// Set the value
inline void udJSON::SetVoid()      { Destroy(); }
inline void udJSON::Set(bool v)    { Destroy(); type = T_Bool;   u.bVal   = v; }
inline void udJSON::Set(int64_t v) { Destroy(); type = T_Int64;  u.i64Val = v; }
inline void udJSON::Set(double v)  { Destroy(); type = T_Double; u.dVal   = v; }

// Accessors
inline udJSON::Type udJSON::GetType() const { return type; }
inline bool udJSON::IsVoid()           const { return (type == T_Void); }
inline bool udJSON::IsNumeric()        const { return (type >= T_Int64 && type <= T_Double); }
inline bool udJSON::IsIntegral()       const { return (type >= T_Int64 && type <= T_Int64); }
inline bool udJSON::IsString()         const { return (type == T_String); }
inline bool udJSON::IsArray()          const { return (type == T_Array); }
inline bool udJSON::IsObject()         const { return (type == T_Object); }
inline bool udJSON::HasMemory()        const { return (type >= T_String && type < T_Count); }
inline size_t udJSON::ArrayLength()    const { return (type == T_Array) ? u.pArray->length : (type == T_Object) ? 1 : 0; }
inline size_t udJSON::MemberCount()    const { return (type == T_Object) ? AsObject()->length : 0; }
inline const char *udJSON::GetMemberName(size_t index) const { return (type == T_Object && index < AsObject()->length) ? AsObject()->GetElement(index)->pKey : nullptr; }
inline const udJSON *udJSON::GetMember(size_t index) const { return (type == T_Object && index < AsObject()->length) ? &AsObject()->GetElement(index)->value : nullptr; }

inline udJSONArray *udJSON::AsArray()     const { return (type == T_Array)   ? u.pArray  : nullptr; }
inline udJSONObject *udJSON::AsObject()   const { return (type == T_Object)  ? u.pObject : nullptr; }
inline udResult udJSON::ToString(const char **ppStr, bool escapeBackslashes) const { return ToString(ppStr, 0, "", "", "", escapeBackslashes); }
inline udResult udJSON::ExtractAndVoid(const char **ppStr) { if (!ppStr) return udR_InvalidParameter_; if (type == T_String) { *ppStr = u.pStr; Clear(); return udR_Success; } return udR_ObjectTypeMismatch; }
#endif // UDJSON_INL_H
