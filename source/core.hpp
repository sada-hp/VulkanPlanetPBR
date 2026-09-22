#ifndef GRAPI

#ifdef DLL_API
	#define GRAPI __declspec(dllexport)
#else
	#define GRAPI __declspec(dllimport)
#endif

#define Bit(x) 1 << x

#define DefineFlags(Type) \
	inline Type operator|(Type l, Type r) { return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(l) | static_cast<std::underlying_type_t<Type>>(r)); } \
	inline Type operator&(Type l, Type r) { return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(l) & static_cast<std::underlying_type_t<Type>>(r)); }

#define CheckFlag(Flags, Flag) static_cast<std::underlying_type_t<decltype(Flags)>>(Flags & Flag) != 0

#endif