#ifndef _GRAPI
#define _GRAPI

#define Bit(x) 1 << x

#define DefineFlags(Type) \
	inline Type operator|(Type l, Type r) { return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(l) | static_cast<std::underlying_type_t<Type>>(r)); } \
	inline Type operator&(Type l, Type r) { return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(l) & static_cast<std::underlying_type_t<Type>>(r)); }

#define CheckFlag(Flags, Flag) static_cast<size_t>(Flags & Flag) != 0

#endif