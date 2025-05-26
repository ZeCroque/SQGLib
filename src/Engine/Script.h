#pragma once
#include "common/allocators/ChainedPool.h"

namespace SQG
{
	namespace ScriptEngine
	{
		// # Script Store
		// =======================
		class Store : public RE::BSScript::IStore
		{
		public:
			std::size_t GetSize() const override;
			std::size_t GetPosition() const override;
			RE::BSStorageDefs::ErrorCode Seek(std::size_t a_offset, RE::BSStorageDefs::SeekMode a_seekMode) const override;
			RE::BSStorageDefs::ErrorCode Read(std::size_t a_numBytes, std::byte* a_bytes) const override;
			RE::BSStorageDefs::ErrorCode Write(std::size_t a_numBytes, const std::byte* a_bytes) override;
			bool Open(const char* a_fileName) override;
			void Close() override;
			const RE::BSFixedString& GetRelPath() override;
			bool HasOpenFile() override;
			bool FileIsGood() override;
			void Unk_0B() override;

			static void Init();
			static Store* GetCustomStore();

		private:
			static RE::BSTSmartPointer<Store> store;
			static std::ifstream fileStream;
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
}
