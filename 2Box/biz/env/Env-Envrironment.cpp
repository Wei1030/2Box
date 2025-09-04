module Env;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

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

	void Env::addProcess(HANDLE handle)
	{
		const std::shared_ptr<ProcessInfo> newProcInfo = std::make_shared<ProcessInfo>(handle);
		if (addProcessInternal(newProcInfo))
		{
			m_waiter.addWait(newProcInfo->getHandle(), [this, newProcInfo]
			{
				removeProcessInternal(newProcInfo);
				removeToplevelWindowWhenProcessTerminate(newProcInfo);
			});
		}
	}

	void Env::addProcess(DWORD pid)
	{
		const std::shared_ptr<ProcessInfo> newProcInfo = std::make_shared<ProcessInfo>(pid);
		if (addProcessInternal(newProcInfo))
		{
			m_waiter.addWait(newProcInfo->getHandle(), [this, newProcInfo]
			{
				removeProcessInternal(newProcInfo);
				removeToplevelWindowWhenProcessTerminate(newProcInfo);
			});
		}
	}

	void Env::addProcess(HANDLE handle, DWORD pid)
	{
		const std::shared_ptr<ProcessInfo> newProcInfo = std::make_shared<ProcessInfo>(handle, pid);
		if (addProcessInternal(newProcInfo))
		{
			m_waiter.addWait(newProcInfo->getHandle(), [this, newProcInfo]
			{
				removeProcessInternal(newProcInfo);
				removeToplevelWindowWhenProcessTerminate(newProcInfo);
			});
		}
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
		m_toplevelWindows.insert(hWnd);
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
		m_toplevelWindows.erase(hWnd);
		proc->removeToplevelWindow(hWnd);
	}

	bool Env::containsToplevelWindow(void* hWnd) const
	{
		std::shared_lock lock(m_wndMutex);
		return m_toplevelWindows.contains(hWnd);
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
			m_toplevelWindows.erase(hWnd);
		}
	}
}
