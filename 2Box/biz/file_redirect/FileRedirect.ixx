export module FileRedirect;

import "sys_defs.h";
import std;

namespace biz
{
	export class FileRedirect
	{
	public:
		void requestCreateRedirectFile(const std::wstring& originalFile, const std::wstring& redirectFile);

	private:
		void endTask(const std::wstring& redirectFile);

	private:
		std::mutex m_mutex;

		class DoneEvent
		{
		public:
			DoneEvent()
			{
				m_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
				if (!m_event)
				{
					throw std::runtime_error(std::format("CreateEventW failed, error code:{}", GetLastError()));
				}
			}

			~DoneEvent()
			{
				CloseHandle(m_event);
			}

			DoneEvent(const DoneEvent&) = delete;
			DoneEvent& operator=(const DoneEvent&) = delete;
			DoneEvent(DoneEvent&& other) = delete;
			DoneEvent& operator=(DoneEvent&& other) = delete;

			void signal() const
			{
				SetEvent(m_event);
			}

			void wait() const
			{
				WaitForSingleObject(m_event, INFINITE);
			}

		private:
			HANDLE m_event;
		};

		std::unordered_map<std::wstring, std::shared_ptr<DoneEvent>> m_tasks;
	};
}
