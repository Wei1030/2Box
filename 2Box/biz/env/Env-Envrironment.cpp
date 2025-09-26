module Env;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import EssentialData;

namespace biz
{
	WaitObject::WaitObject()
	{
		m_pWait = CreateThreadpoolWait(&WaitObject::onHandleNotify, this, nullptr);
		if (!m_pWait)
		{
			throw std::runtime_error{std::format("CreateThreadpoolWait failed, error code:{}", GetLastError())};
		}
	}

	WaitObject::~WaitObject()
	{
		if (m_pWait)
		{
			SetThreadpoolWait(m_pWait, nullptr, nullptr);
			WaitForThreadpoolWaitCallbacks(m_pWait, TRUE);
			CloseThreadpoolWait(m_pWait);
		}
	}

	void WaitObject::setWait(HANDLE handle, WaitCallback cb)
	{
		SetThreadpoolWait(m_pWait, nullptr, nullptr);
		WaitForThreadpoolWaitCallbacks(m_pWait, TRUE);

		m_cb = std::move(cb);
		SetThreadpoolWait(m_pWait, handle, nullptr);
	}

	void WaitObject::onHandleNotify(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WAIT, TP_WAIT_RESULT)
	{
		if (WaitObject* pWaitObject = static_cast<WaitObject*>(Context);
			pWaitObject && pWaitObject->m_cb)
		{
			pWaitObject->m_cb();
			pWaitObject->m_cb = nullptr;
		}
	}

	void HandleWaiter::addWait(HANDLE handle, WaitObject::WaitCallback cb)
	{
		WaitObjectWrapper* objWrapper = getObject();
		objWrapper->obj.setWait(handle, [this, objWrapper, cb = std::move(cb)]
		{
			releaseObject(objWrapper);
			if (cb)
			{
				cb();
			}
		});
	}

	HandleWaiter::WaitObjectWrapper* HandleWaiter::getObject()
	{
		std::unique_ptr<WaitObjectWrapper> newObj;
		std::lock_guard lock(m_mutex);
		if (m_frees.empty())
		{
			newObj = std::make_unique<WaitObjectWrapper>();
		}
		else
		{
			newObj = std::move(m_frees.back());
			m_frees.pop_back();
		}

		WaitObjectWrapper* rawPtrReturn = newObj.get();
		m_inUse.push_back(std::move(newObj));
		const size_t index = m_inUse.size() - 1;
		rawPtrReturn->useIndex = index;
		return rawPtrReturn;
	}

	void HandleWaiter::releaseObject(const WaitObjectWrapper* obj)
	{
		std::lock_guard lock(m_mutex);

		const size_t idx = obj->useIndex;
		if (idx >= m_inUse.size())
		{
			std::unreachable();
		}
		if (idx < m_inUse.size() - 1)
		{
			std::swap(m_inUse.at(idx), m_inUse.back());
			m_inUse.at(idx)->useIndex = idx;
		}
		m_frees.push_back(std::move(m_inUse.back()));
		m_inUse.pop_back();
	}

	ProcessInfo::ProcessInfo(HANDLE handle) : m_hProcess(handle)
	{
		if (!m_hProcess)
		{
			std::unreachable();
		}
		m_processId = GetProcessId(m_hProcess);
		if (!m_processId)
		{
			throw std::runtime_error("Failed to get process id");
		}
		initializeFullPath();
	}

	ProcessInfo::ProcessInfo(DWORD pid) : m_hProcess(pid), m_processId(pid)
	{
		initializeFullPath();
	}

	ProcessInfo::ProcessInfo(HANDLE handle, DWORD pid) : m_hProcess(handle), m_processId(pid)
	{
		initializeFullPath();
	}

	void ProcessInfo::addToplevelWindow(void* hWnd)
	{
		m_toplevelWindows.insert(hWnd);
	}

	void ProcessInfo::removeToplevelWindow(void* hWnd)
	{
		m_toplevelWindows.erase(hWnd);
	}

	void ProcessInfo::initializeFullPath()
	{
		DWORD pathLength = std::numeric_limits<short>::max();
		m_fullPath.resize(pathLength);
		if (!QueryFullProcessImageNameW(m_hProcess, 0, m_fullPath.data(), &pathLength))
		{
			throw std::runtime_error("Failed to query full path");
		}
		m_fullPath.resize(pathLength);
		m_fullPath = std::wstring(m_fullPath);
	}

	bool ProcessDenseMap::addProcessInfo(const std::shared_ptr<ProcessInfo>& procInfo)
	{
		const DWORD pid = procInfo->getProcessId();
		auto [it, success] = m_sparse.insert(std::make_pair(pid, procInfo));
		if (!success)
		{
			return false;
		}

		m_procNames.insert(std::wstring{procInfo->getProcessFullPath()});

		m_densePids.push_back(pid);
		procInfo->setDenseIndex(m_densePids.size() - 1);
		return true;
	}

	bool ProcessDenseMap::removeProcessInfoById(DWORD pid)
	{
		const auto it = m_sparse.find(pid);
		if (it == m_sparse.end())
		{
			return false;
		}

		const auto itName = m_procNames.find(std::wstring{it->second->getProcessFullPath()});
		if (itName != m_procNames.end())
		{
			m_procNames.erase(itName);
		}

		const size_t idx = it->second->getDenseIndex();
		if (idx < m_densePids.size() - 1)
		{
			std::swap(m_densePids.at(idx), m_densePids.back());
			if (const auto swapped = m_sparse.find(m_densePids.at(idx)); swapped != m_sparse.end())
			{
				swapped->second->setDenseIndex(idx);
			}
		}
		m_densePids.pop_back();
		m_sparse.erase(it);
		return true;
	}

	std::size_t ProcessDenseMap::getCount() const
	{
		return m_sparse.size();
	}

	std::vector<std::shared_ptr<ProcessInfo>> ProcessDenseMap::getAllProcesses() const
	{
		std::vector<std::shared_ptr<ProcessInfo>> result;
		for (auto it = m_sparse.begin(); it != m_sparse.end(); ++it)
		{
			result.emplace_back(it->second);
		}
		return result;
	}

	std::shared_ptr<ProcessInfo> ProcessDenseMap::getProcessInfo(DWORD pid) const
	{
		const auto it = m_sparse.find(pid);
		if (it == m_sparse.end())
		{
			return nullptr;
		}
		return it->second;
	}

	bool TopLevelWindowDenseMap::addTopLevelWindow(void* hWnd)
	{
		auto [it, success] = m_sparse.insert(std::make_pair(hWnd, TopLevelWindow{hWnd}));
		if (!success)
		{
			return false;
		}
		m_denseHandles.push_back(hWnd);
		it->second.setDenseIndex(m_denseHandles.size() - 1);
		return true;
	}

	bool TopLevelWindowDenseMap::removeTopLevelWindow(void* hWnd)
	{
		const auto it = m_sparse.find(hWnd);
		if (it == m_sparse.end())
		{
			return false;
		}
		const size_t idx = it->second.getDenseIndex();
		if (idx < m_denseHandles.size() - 1)
		{
			std::swap(m_denseHandles.at(idx), m_denseHandles.back());
			if (const auto swapped = m_sparse.find(m_denseHandles.at(idx)); swapped != m_sparse.end())
			{
				swapped->second.setDenseIndex(idx);
			}
		}
		m_denseHandles.pop_back();
		m_sparse.erase(it);
		return true;
	}

	template <ArchBit BitType = CURRENT_ARCH_BIT>
	std::filesystem::path get_dll_full_path(const std::filesystem::path& binDir, std::wstring_view flagName, std::wstring_view extra = L"")
	{
		namespace fs = std::filesystem;
		if constexpr (BitType == ArchBit::Bit64)
		{
			return fs::path{fs::weakly_canonical(binDir / fs::path{std::format(L"{}_{}64.bin", flagName, extra)})};
		}
		else
		{
			return fs::path{fs::weakly_canonical(binDir / fs::path{std::format(L"{}_{}32.bin", flagName, extra)})};
		}
	}

	std::string Env::ensureDllInDeviceAndReturnPath() const
	{
		namespace fs = std::filesystem;
		fs::path binDir{app().binDir()};
		if (!fs::exists(binDir))
		{
			fs::create_directories(binDir);
		}

		std::string dllFullPath;
		if (const fs::path path32{get_dll_full_path<ArchBit::Bit32>(binDir, m_flagName)}; !fs::exists(path32))
		{
			const fs::path tempPath{get_dll_full_path<ArchBit::Bit32>(binDir, m_flagName, L"temp")};
			const auto [address, size] = get_core_data().dll32;
			std::ofstream tempFile{tempPath, std::ios::binary | std::ios::trunc};
			tempFile.write(address, size);
			tempFile.close();
			fs::rename(tempPath, path32);
			if constexpr (CURRENT_ARCH_BIT == ArchBit::Bit32)
			{
				dllFullPath = path32.string();
			}
		}

		if (const fs::path path64{get_dll_full_path<ArchBit::Bit64>(binDir, m_flagName)}; !fs::exists(path64))
		{
			const fs::path tempPath{get_dll_full_path<ArchBit::Bit64>(binDir, m_flagName, L"temp")};
			const auto [address, size] = get_core_data().dll64;
			std::ofstream tempFile{tempPath, std::ios::binary | std::ios::trunc};
			tempFile.write(address, size);
			tempFile.close();
			fs::rename(tempPath, path64);
			if constexpr (CURRENT_ARCH_BIT == ArchBit::Bit64)
			{
				dllFullPath = path64.string();
			}
		}
		return dllFullPath;
	}

	void Env::deleteDllFromDevice() const
	{
		namespace fs = std::filesystem;
		fs::path binDir{app().binDir()};
		if (!fs::exists(binDir))
		{
			return;
		}
		if (const fs::path path32{biz::get_dll_full_path<ArchBit::Bit32>(binDir, m_flagName)}; fs::exists(path32))
		{
			const fs::path tempPath{biz::get_dll_full_path<ArchBit::Bit32>(binDir, m_flagName, L"temp_to_delete")};
			fs::rename(path32, tempPath);
			fs::remove(tempPath);
		}

		if (const fs::path path64{biz::get_dll_full_path<ArchBit::Bit64>(binDir, m_flagName)}; fs::exists(path64))
		{
			const fs::path tempPath{biz::get_dll_full_path<ArchBit::Bit64>(binDir, m_flagName, L"temp_to_delete")};
			fs::rename(path64, tempPath);
			fs::remove(tempPath);
		}
	}

	std::shared_ptr<ProcessInfo> Env::addProcess(DWORD pid)
	{
		const std::shared_ptr<ProcessInfo> newProcInfo = std::make_shared<ProcessInfo>(pid);
		if (addProcessInternal(newProcInfo))
		{
			m_waiter.addWait(newProcInfo->getHandle(), [this, newProcInfo]
			{
				removeProcessInternal(newProcInfo);
				removeToplevelWindowWhenProcessTerminate(newProcInfo);
			});
			return newProcInfo;
		}
		return nullptr;
	}

	std::size_t Env::getAllProcessesCount() const
	{
		std::shared_lock lock(m_mutex);
		return m_processes.getCount();
	}

	std::vector<std::shared_ptr<ProcessInfo>> Env::getAllProcesses() const
	{
		std::shared_lock lock(m_mutex);
		return m_processes.getAllProcesses();
	}

	std::shared_ptr<ProcessInfo> Env::getProcess(DWORD pid) const
	{
		std::shared_lock lock(m_mutex);
		return m_processes.getProcessInfo(pid);
	}

	std::vector<DWORD> Env::getAllProcessIds() const
	{
		std::shared_lock lock(m_mutex);
		return m_processes.getPids();
	}

	bool Env::contains(const std::wstring& procFullName) const
	{
		std::shared_lock lock(m_mutex);
		return m_processes.contains(procFullName);
	}

	void Env::setProcCountChangeNotify(ProcCountChangeNotify notify)
	{
		std::unique_lock lock(m_mutex);
		m_notify = std::move(notify);
	}

	void Env::addToplevelWindow(DWORD pid, void* hWnd)
	{
		std::shared_ptr<ProcessInfo> proc = getProcess(pid);
		if (!proc)
		{
			return;
		}
		std::unique_lock lock(m_wndMutex);
		m_toplevelWindows.addTopLevelWindow(hWnd);
		proc->addToplevelWindow(hWnd);
	}

	void Env::removeToplevelWindow(DWORD pid, void* hWnd)
	{
		std::shared_ptr<ProcessInfo> proc = getProcess(pid);
		if (!proc)
		{
			return;
		}
		std::unique_lock lock(m_wndMutex);
		m_toplevelWindows.removeTopLevelWindow(hWnd);
		proc->removeToplevelWindow(hWnd);
	}

	bool Env::containsToplevelWindow(void* hWnd) const
	{
		std::shared_lock lock(m_wndMutex);
		return m_toplevelWindows.contains(hWnd);
	}

	std::vector<void*> Env::getAllToplevelWindows() const
	{
		std::shared_lock lock(m_wndMutex);
		return m_toplevelWindows.getAllHandles();
	}

	bool Env::addProcessInternal(const std::shared_ptr<ProcessInfo>& procInfo)
	{
		std::unique_lock lock(m_mutex);
		if (m_processes.addProcessInfo(procInfo))
		{
			if (m_notify)
			{
				m_notify(EProcEvent::Create, procInfo, m_processes.getCount());
			}
			return true;
		}
		return false;
	}

	bool Env::removeProcessInternal(const std::shared_ptr<ProcessInfo>& procInfo)
	{
		std::unique_lock lock(m_mutex);
		if (m_processes.removeProcessInfoById(procInfo->getProcessId()))
		{
			if (m_notify)
			{
				m_notify(EProcEvent::Terminate, procInfo, m_processes.getCount());
			}
			return true;
		}
		return false;
	}

	void Env::removeToplevelWindowWhenProcessTerminate(const std::shared_ptr<ProcessInfo>& procInfo)
	{
		std::unique_lock lock(m_wndMutex);
		const std::unordered_set<void*>& allToplevelWindows = procInfo->getToplevelWindows();
		for (void* hWnd : allToplevelWindows)
		{
			m_toplevelWindows.removeTopLevelWindow(hWnd);
		}
	}
}
