module FileRedirect;

namespace
{
	void create_redirect_file(const std::wstring& originalFile, const std::wstring& redirectFile)
	{
		try
		{
			namespace fs = std::filesystem;
			if (fs::exists(originalFile) && !fs::exists(redirectFile))
			{
				const fs::path tempPath{std::format(L"{}temp", redirectFile)};
				fs::copy(originalFile, tempPath);
				fs::rename(tempPath, fs::path{redirectFile});
			}
		}
		catch (...)
		{
		}
	}
}

namespace biz
{
	void FileRedirect::requestCreateRedirectFile(const std::wstring& originalFile, const std::wstring& redirectFile)
	{
		std::shared_ptr<DoneEvent> doneEvent;
		bool bIsFirstRequest;
		{
			std::lock_guard lock(m_mutex);
			if (const auto it = m_tasks.find(redirectFile); it == m_tasks.end())
			{
				doneEvent = std::make_shared<DoneEvent>();
				m_tasks.insert(std::make_pair(redirectFile, doneEvent));
				bIsFirstRequest = true;
			}
			else
			{
				doneEvent = it->second;
				bIsFirstRequest = false;
			}
		}
		if (bIsFirstRequest)
		{
			create_redirect_file(originalFile, redirectFile);
			doneEvent->signal();
			endTask(redirectFile);
		}
		else
		{
			doneEvent->wait();
		}
	}

	void FileRedirect::endTask(const std::wstring& redirectFile)
	{
		std::lock_guard lock(m_mutex);
		m_tasks.erase(redirectFile);
	}
}
