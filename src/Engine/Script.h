#pragma once
#include "common/allocators/ChainedPool.h"
#include "papyrus/PapyrusCompilationContext.h"

namespace SQG::Engine::Script
{
	// # Script Store
	// =======================
	class Store final : public RE::BSScript::IStore
	{
	public:
		std::size_t GetSize() const override;
		std::size_t GetPosition() const override;
		RE::BSStorageDefs::ErrorCode Seek(std::size_t inOffset, RE::BSStorageDefs::SeekMode inSeekMode) const override;
		RE::BSStorageDefs::ErrorCode Read(std::size_t inNumBytes, std::byte* inBytes) const override;
		RE::BSStorageDefs::ErrorCode Write(std::size_t inNumBytes, const std::byte* inBytes) override;
		bool Open(const char* inFileName) override;
		void Close() override;
		const RE::BSFixedString& GetRelPath() override;
		bool HasOpenFile() override;
		bool FileIsGood() override;
		void Unk_0B() override;

		static void Init();
		static Store* GetCustomStore();

	private:
		static RE::BSTSmartPointer<Store> store;
		static RE::BSTSmartPointer<RE::BSScript::IStore> baseStore;
		bool readingCustomScript = false;
		std::string scriptName;
		RE::BSFixedString fakePath;
		mutable caprica::allocators::ChainedPool::HeapIterator chainedPoolIt;
		mutable std::size_t chainedPoolPos = 0;
		mutable std::size_t index = 0;
	};

	// # Caprica
	// =======================
	void Init();
	void AddScript(const std::string& inStringName, const std::string_view& inData);
}
