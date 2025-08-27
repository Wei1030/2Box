export module Env:Envrironment;

import "sys_defs.h";
import std;

namespace biz
{
	class WaitObject
	{
	public:
		WaitObject();
		~WaitObject();

		WaitObject(const WaitObject&) = delete;
		WaitObject(WaitObject&&) = delete;
		WaitObject& operator=(const WaitObject&) = delete;
		WaitObject& operator=(WaitObject&&) = delete;

		using WaitCallback = std::function<void()>;
		void setWait(HANDLE handle, WaitCallback cb);

	private:
		static VOID CALLBACK onHandleNotify(_Inout_ PTP_CALLBACK_INSTANCE, _Inout_opt_ PVOID Context, _Inout_ PTP_WAIT, _In_ TP_WAIT_RESULT);

	private:
		TP_WAIT* m_pWait;
		WaitCallback m_cb;
	};

	class HandleWaiter
	{
	public:
		void addWait(HANDLE handle, WaitObject::WaitCallback cb);

	private:
		struct WaitObjectWrapper
		{
			WaitObject obj;
			size_t useIndex;
		};

		using Object = std::shared_ptr<WaitObjectWrapper>;
		Object getObject();
		void releaseObject(const Object& obj);

	private:
		std::mutex m_mutex;
		std::vector<Object> m_frees;
		std::vector<Object> m_inUse;
	};

	class ProcessHandle
	{
	public:
		explicit ProcessHandle(HANDLE handle) noexcept : m_handle(handle)
		{
		}

		explicit ProcessHandle(DWORD pid)
		{
			m_handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
			if (!m_handle)
			{
				throw std::runtime_error(std::format("Failed to open process, error code:{}", GetLastError()));
			}
		}

		operator HANDLE() const { return m_handle; }

		~ProcessHandle()
		{
			CloseHandle(m_handle);
		}

		ProcessHandle(const ProcessHandle&) = delete;
		ProcessHandle(ProcessHandle&&) = delete;
		ProcessHandle& operator=(const ProcessHandle&) = delete;
		ProcessHandle& operator=(ProcessHandle&&) = delete;

		explicit operator bool() const noexcept
		{
			return m_handle != nullptr;
		}

	private:
		HANDLE m_handle;
	};

	class ProcessInfo
	{
	public:
		explicit ProcessInfo(HANDLE handle);
		explicit ProcessInfo(DWORD pid);
		ProcessInfo(HANDLE handle, DWORD pid);

	public:
		HANDLE getHandle() const noexcept { return m_hProcess; }
		DWORD getProcessId() const noexcept { return m_processId; }
		std::wstring_view getProcessFullPath() const noexcept { return m_fullPath; }

		void setDenseIndex(size_t index) noexcept { m_indexInDense = index; }
		size_t getDenseIndex() const noexcept { return m_indexInDense; }

	private:
		void initializeFullPath();

	private:
		ProcessHandle m_hProcess;
		std::wstring m_fullPath;
		size_t m_indexInDense{0};
		DWORD m_processId;
	};

	class ProcessDenseMap
	{
	public:
		void addProcessInfo(const std::shared_ptr<ProcessInfo>& procInfo);
		void removeProcessInfoById(DWORD pid);

	public:
		const std::vector<DWORD>& getPids() const { return m_densePids; }
		bool contains(std::wstring_view procFullName) const { return m_uniqueProcNames.contains(procFullName); }

	private:
		std::unordered_set<std::wstring_view> m_uniqueProcNames;
		std::vector<DWORD> m_densePids;
		std::unordered_map<DWORD, std::shared_ptr<ProcessInfo>> m_sparse;
	};


	export class Env
	{
	public:
		Env(std::uint32_t index, std::uint64_t flag,
		    std::wstring_view flagName, std::wstring_view name, std::string_view dllFullPath)
			: m_index(index), m_flag(flag), m_flagName(flagName), m_name(name), m_dllFullPath(dllFullPath)
		{
		}

	public:
		std::uint32_t getIndex() const { return m_index; }
		std::uint64_t getFlag() const { return m_flag; }
		std::wstring_view getFlagName() const { return m_flagName; }
		std::wstring_view getName() const { return m_name; }
		std::string_view getDllFullPath() const { return m_dllFullPath; }

	public:
		void addProcess(HANDLE handle);
		void addProcess(DWORD pid);
		void addProcess(HANDLE handle, DWORD pid);

		std::vector<DWORD> getAllProcessIds() const;
		bool contains(std::wstring_view procFullName) const;

	private:
		void addProcessInternal(const std::shared_ptr<ProcessInfo>& procInfo);
		void removeProcessInternal(DWORD pid);

	private:
		std::uint32_t m_index{0};
		std::uint64_t m_flag{0};
		std::wstring m_flagName;
		std::wstring m_name;
		std::string m_dllFullPath;
		HandleWaiter m_waiter;
		mutable std::shared_mutex m_mutex;
		ProcessDenseMap m_processes;
	};
}
