#ifndef UDVALUE_INL_H
#define UDVALUE_INL_H

inline udValue::udValue()           { Clear(); }
inline udValue::udValue(int64_t v)  { type = T_Int64;  u.i64Val = v; }
inline udValue::udValue(double v)   { type = T_Double; u.dVal   = v; }
inline void udValue::Clear()        { type = T_Void;   u.i64Val = 0; } // Clear the value without freeing
inline udValue::~udValue()          { Destroy(); }

// Set the value
inline void udValue::SetVoid()      { Destroy(); }
inline void udValue::Set(bool v)    { Destroy(); type = T_Bool;   u.bVal   = v; }
inline void udValue::Set(int64_t v) { Destroy(); type = T_Int64;  u.i64Val = v; }
inline void udValue::Set(double v)  { Destroy(); type = T_Double; u.dVal   = v; }

// Accessors
inline udValue::Type udValue::GetType() const { return type; }
inline bool udValue::IsVoid()           const { return (type == T_Void); }
inline bool udValue::IsNumeric()        const { return (type >= T_Int64 && type <= T_Double); }
inline bool udValue::IsIntegral()       const { return (type >= T_Int64 && type <= T_Int64); }
inline bool udValue::IsString()         const { return (type == T_String); }
inline bool udValue::IsArray()          const { return (type == T_Array); }
inline bool udValue::IsObject()         const { return (type == T_Object); }
inline bool udValue::HasMemory()        const { return (type >= T_String && type < T_Count); }
inline size_t udValue::ArrayLength()    const { return (type == T_Array) ? u.pArray->length : (type == T_Object) ? 1 : 0; }
inline size_t udValue::MemberCount()    const { return (type == T_Object) ? AsObject()->length : 0; }
inline const char *udValue::GetMemberName(size_t index) const { return (type == T_Object && index < AsObject()->length) ? AsObject()->GetElement(index)->pKey : nullptr; }
inline const udValue *udValue::GetMember(size_t index) const { return (type == T_Object && index < AsObject()->length) ? &AsObject()->GetElement(index)->value : nullptr; }

inline udValueArray *udValue::AsArray()     const { return (type == T_Array)   ? u.pArray  : nullptr; }
inline udValueObject *udValue::AsObject()   const { return (type == T_Object)  ? u.pObject : nullptr; }
inline udResult udValue::ToString(const char **ppStr, bool escapeBackslashes) const { return ToString(ppStr, 0, "", "", "", escapeBackslashes); }
inline udResult udValue::ExtractAndVoid(const char **ppStr) { if (!ppStr) return udR_InvalidParameter_; if (type == T_String) { *ppStr = u.pStr; Clear(); return udR_Success; } return udR_ObjectTypeMismatch; }
#endif // UDVALUE_INL_H
