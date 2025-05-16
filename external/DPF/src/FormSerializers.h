#pragma once

//Thanks to David Mazières (https://www.scs.stanford.edu/~dm/blog/va-opt.html)
#define PARENS ()

#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH(macro, ...)                                    \
  __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__))) \

#define FOR_EACH_HELPER(macro, a1, ...)                         \
  macro(a1)                                                     \
  __VA_OPT__(FOR_EACH_AGAIN PARENS (macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

namespace DPF
{
	template <typename T> class FormSerializer {
	public:
	    virtual void Serialize(Serializer<T>*, RE::TESForm*) = 0;
	    virtual void Deserialize(Serializer<T>*, RE::TESForm*) = 0;
	    virtual bool Condition(RE::TESForm*) = 0;
	    virtual ~FormSerializer() = default;
	};

	#define REGISTER_SERIALIZER(serializerName) \
	    template <typename T>  class serializerName ## Serializer : public FormSerializer<T> { \
	    public: \
	    void Serialize(Serializer<T>* serializer, RE::TESForm* form) override; \
	    void Deserialize(Serializer<T>* serializer, RE::TESForm* form) override; \
	    bool Condition(RE::TESForm* form) override; \
	    };

	#define ADD_SERIALIZER_TO_LIST(serializer) \
	    filters.push_back(std::make_unique<serializer ## Serializer<T>>());

	#define REGISTER_SERIALIZERS(...) \
	EXPAND(FOR_EACH(REGISTER_SERIALIZER, __VA_ARGS__)) \
	template<class T> auto& GetFilters() \
	{ \
	static std::vector<std::unique_ptr<FormSerializer<T>>> filters; \
	EXPAND(FOR_EACH(ADD_SERIALIZER_TO_LIST, __VA_ARGS__)) \
	    return filters; \
	}

	template <typename T> void StoreEachFormData(Serializer<T>* serializer, FormRecord& elem);
	template <typename T> void RestoreEachFormData(Serializer<T>* serializer, FormRecord& elem);
}

#include "FormSerializers.hpp"