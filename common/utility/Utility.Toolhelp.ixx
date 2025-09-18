export module Utility.Toolhelp;

import "sys_defs.h";

namespace utils
{
	export class CToolhelp
	{
	private:
		HANDLE m_hSnapshot{INVALID_HANDLE_VALUE};

	public:
		explicit CToolhelp(DWORD dwFlags = 0, DWORD dwProcessID = 0)
		{
			createSnapshot(dwFlags, dwProcessID);
		}

		~CToolhelp()
		{
			if (m_hSnapshot != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_hSnapshot);
			}
		}

		BOOL createSnapshot(DWORD dwFlags, DWORD dwProcessID = 0)
		{
			if (m_hSnapshot != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_hSnapshot);
			}

			if (dwFlags == 0)
			{
				m_hSnapshot = INVALID_HANDLE_VALUE;
			}
			else
			{
				if (dwFlags & TH32CS_SNAPMODULE
					|| dwFlags & TH32CS_SNAPMODULE32)
				{
					int tryCount = 200;
					while (tryCount--)
					{
						m_hSnapshot = CreateToolhelp32Snapshot(dwFlags, dwProcessID);
						if (m_hSnapshot != INVALID_HANDLE_VALUE)
						{
							break;
						}
						if (GetLastError() != ERROR_BAD_LENGTH)
						{
							break;
						}
						Sleep(1);
					}
				}
				else
				{
					m_hSnapshot = CreateToolhelp32Snapshot(dwFlags, dwProcessID);
				}
			}
			return m_hSnapshot != INVALID_HANDLE_VALUE;
		}

		BOOL processFirst(PPROCESSENTRY32 ppe) const
		{
			BOOL fOk = Process32First(m_hSnapshot, ppe);
			if (fOk && ppe->th32ProcessID == 0)
			{
				fOk = processNext(ppe); // Remove the "[System Process]" (PID = 0)
			}
			return fOk;
		}

		BOOL processNext(PPROCESSENTRY32 ppe) const
		{
			BOOL fOk = Process32Next(m_hSnapshot, ppe);
			if (fOk && ppe->th32ProcessID == 0)
			{
				fOk = processNext(ppe); // Remove the "[System Process]" (PID = 0)
			}
			return fOk;
		}

		BOOL processFind(DWORD dwProcessId, PPROCESSENTRY32 ppe) const
		{
			BOOL fFound = FALSE;
			for (BOOL fOk = processFirst(ppe); fOk; fOk = processNext(ppe))
			{
				fFound = ppe->th32ProcessID == dwProcessId;
				if (fFound)
				{
					break;
				}
			}
			return fFound;
		}

		BOOL moduleFirst(PMODULEENTRY32 pme) const
		{
			return Module32First(m_hSnapshot, pme);
		}

		BOOL moduleNext(PMODULEENTRY32 pme) const
		{
			return Module32Next(m_hSnapshot, pme);
		}

		BOOL moduleFind(PVOID pvBaseAddr, PMODULEENTRY32 pme) const
		{
			for (BOOL fOk = moduleFirst(pme); fOk; fOk = moduleNext(pme))
			{
				if (pme->modBaseAddr == pvBaseAddr)
				{
					return TRUE;
				}
			}
			return FALSE;
		}

		BOOL moduleFind(PCTSTR pszModName, PMODULEENTRY32 pme) const
		{
			for (BOOL fOk = moduleFirst(pme); fOk; fOk = moduleNext(pme))
			{
				if (lstrcmpi(pme->szModule, pszModName) == 0 ||
					lstrcmpi(pme->szExePath, pszModName) == 0)
				{
					return TRUE;
				}
			}
			return FALSE;
		}

		BOOL threadFirst(PTHREADENTRY32 pte) const
		{
			return Thread32First(m_hSnapshot, pte);
		}

		BOOL threadNext(PTHREADENTRY32 pte) const
		{
			return Thread32Next(m_hSnapshot, pte);
		}

		BOOL heapListFirst(PHEAPLIST32 phl) const
		{
			return Heap32ListFirst(m_hSnapshot, phl);
		}

		BOOL heapListNext(PHEAPLIST32 phl) const
		{
			return Heap32ListNext(m_hSnapshot, phl);
		}

		int howManyHeaps() const
		{
			int nHowManyHeaps = 0;
			HEAPLIST32 hl = {sizeof(hl)};
			for (BOOL fOk = heapListFirst(&hl); fOk; fOk = heapListNext(&hl))
			{
				nHowManyHeaps++;
			}
			return nHowManyHeaps;
		}
	};
}
