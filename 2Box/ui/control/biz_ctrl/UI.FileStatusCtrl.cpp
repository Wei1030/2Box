module UI.FileStatusCtrl;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import std;
import MainApp;
import Scheduler;

namespace
{
	void create_tips_text_layout(std::wstring_view text, UniqueComPtr<IDWriteTextLayout>& textLayout)
	{
		textLayout.reset();
		app().dWriteFactory()->CreateTextLayout(text.data(),
		                                        static_cast<UINT32>(text.length()),
		                                        app().textFormat().pTipsFormat,
		                                        std::numeric_limits<float>::max(), ui::FileStatusCtrl::tipsTextHeight,
		                                        &textLayout);
		if (textLayout)
		{
			textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER};
			textLayout->SetTrimming(&trimming, app().textFormat().pTipsEllipsisTrimmingSign);
		}
	}

	bool copy_text_to_clipboard(HWND hWnd, std::wstring_view text)
	{
		if (!OpenClipboard(hWnd))
		{
			return false;
		}
		EmptyClipboard();

		const HGLOBAL hCopy = GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(wchar_t));
		if (hCopy == nullptr)
		{
			CloseClipboard();
			return false;
		}
		wchar_t* pStrCopy = static_cast<wchar_t*>(GlobalLock(hCopy));
		if (!pStrCopy)
		{
			GlobalFree(hCopy);
			CloseClipboard();
			return false;
		}

		memcpy(pStrCopy, text.data(), text.length() * sizeof(wchar_t));
		pStrCopy[text.length()] = 0;

		GlobalUnlock(hCopy);
		const HANDLE hData = SetClipboardData(CF_UNICODETEXT, hCopy);
		CloseClipboard();
		return hData != nullptr;
	}
}

namespace ui
{
	void fsc_detail::CopyButton::initialize(ControlBase* parent)
	{
		m_button = std::make_unique<Button>(parent);
		m_button->setBackgroundColor(D2D1::ColorF(0xd3d3d3));
		m_button->setBackgroundColor(D2D1::ColorF(0xe6e6e6), Button::EState::Normal);
		m_button->setTextColor(D2D1::ColorF(0x323130));
		m_button->setTextFormat(app().textFormat().pToolBtnFormat);
		m_button->setText(L"复制");

		m_button->setOnClick([this, owner = parent->owner()]
		{
			if (copy_text_to_clipboard(owner->nativeHandle(), m_text))
			{
				m_button->setText(L"已复制");
				m_button->update();
				// stop last timer
				m_stopSource.request_stop();
				// create new timer
				m_stopSource = std::stop_source{};
				app().get_scheduler().addTimer(std::chrono::seconds(3), [this]
				{
					m_button->setText(L"复制");
					m_button->update();
				}, m_stopSource.get_token());
			}
		});
	}

	void FileStatusCtrl::setFilePath(std::wstring_view fileDir, std::wstring_view fileName)
	{
		m_fileDir = fileDir;
		m_fileName = fileName;
		m_filePath = std::format(L"{}\\{}", fileDir, fileName);

		create_tips_text_layout(fileDir, m_pDirLayout);
		create_tips_text_layout(fileName, m_pFileNameLayout);

		m_copyDirBtn.setTextToCopy(fileDir);
		m_copyFileNameBtn.setTextToCopy(fileName);
	}

	void FileStatusCtrl::setDownloadUrl(std::wstring_view serverName, std::wstring_view objName)
	{
		m_downloadServerName = serverName;
		m_downloadObjName = objName;
		m_downloadUrl = std::format(L"https://{}/{}", serverName, objName);

		create_tips_text_layout(m_downloadUrl, m_pUrlLayout);

		m_copyUrlBtn.setTextToCopy(m_downloadUrl);
	}

	void FileStatusCtrl::startAnaTask()
	{
		stopAnaTask();
		m_asyncScope.reset();

		// task 可能会因为失败而多次重试，而taskUntilSuccess只有task成功时才会被resolve且置空，仅当m_resolver非空时才需要重新创建taskUntilSuccess任务
		if (!m_resolver)
		{
			m_taskUntilSuccess = coro::SharedTask<void>::create([this](coro::GuaranteedResolver<void> resolver)
			{
				m_resolver = std::move(resolver);
			});
		}
		m_stopSource = std::stop_source{};
		m_task = coro::start_and_shared(coro::co_with_cancellation(startAnaTaskImpl(), m_stopSource.get_token()));
	}

	void FileStatusCtrl::stopAnaTask()
	{
		m_stopSource.request_stop();
		m_task.waitUntilDone();
		m_asyncScope.join();

		m_pLoadingIndicator->stopAnim();
		m_totalLength = 0;
		m_currentSize = 0;
	}

	coro::LazyTask<void> FileStatusCtrl::joinAsync()
	{
		try
		{
			co_await m_task;
			co_return;
		}
		catch (...)
		{
		}
		co_await sched::transfer_to(app().get_scheduler());
		stopAnaTask();
		co_return;
	}

	coro::LazyTask<void> FileStatusCtrl::untilSuccess()
	{
		co_await m_taskUntilSuccess;
		co_return;
	}

	void FileStatusCtrl::initialize()
	{
		m_pLoadingIndicator = std::make_unique<LoadingIndicator>(this);

		m_retryBtn = std::make_unique<Button>(this);
		m_retryBtn->setBackgroundColor(D2D1::ColorF(0x4895ef));
		m_retryBtn->setBackgroundColor(D2D1::ColorF(0x0078d7), Button::EState::Normal);
		m_retryBtn->setTextColor(D2D1::ColorF(0xffffff));
		m_retryBtn->setTextFormat(app().textFormat().pToolBtnFormat);
		m_retryBtn->setText(L"重试下载");
		m_retryBtn->setOnClick([this]
		{
			startAnaTask();
		});

		m_copyUrlBtn.initialize(this);
		m_copyDirBtn.initialize(this);
		m_copyFileNameBtn.initialize(this);

		m_painter.transferTo<EPainterType::Initial>();
	}

	void FileStatusCtrl::drawImpl(const RenderContext& renderCtx)
	{
		app().textFormat().setAllTextEllipsisTrimming();

		PainterContext ctx{*this, renderCtx};
		m_painter.setCtx(&ctx);
		m_painter.update();

		app().textFormat().clearAllTextEllipsisTrimming();
	}

	void FileStatusCtrl::drawBaseContent(const RenderContext& renderCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();
		const D2D1_ROUNDED_RECT round = D2D1::RoundedRect(
			D2D1::RectF(0, 0, drawSize.width, drawSize.height),
			4.f, 4.f);
		solidBrush->SetColor(D2D1::ColorF(0xE6E6E6));
		renderTarget->DrawRoundedRectangle(round, solidBrush);
		solidBrush->SetColor(D2D1::ColorF(0xFDFDFD));
		renderTarget->FillRoundedRectangle(round, solidBrush);

		const std::wstring fileNameText = std::format(L"文件名: {}", m_fileName);
		solidBrush->SetColor(D2D1::ColorF(0x333333));
		renderTarget->DrawTextW(fileNameText.c_str(),
		                        static_cast<UINT32>(fileNameText.length()),
		                        app().textFormat().pTitleFormat,
		                        D2D1::RectF(padding, padding, drawSize.width - padding, padding + titleFileNameHeight),
		                        solidBrush);
	}

	void FileStatusCtrl::drawStatusContent(const RenderContext& renderCtx, const D2D1_COLOR_F& color, std::wstring_view statusText) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		constexpr float yPos = padding + titleFileNameHeight + margin;
		solidBrush->SetColor(color);
		renderTarget->DrawTextW(statusText.data(),
		                        static_cast<UINT32>(statusText.length()),
		                        app().textFormat().pMainFormat,
		                        D2D1::RectF(padding, yPos, size().width - padding, yPos + statusTextHeight),
		                        solidBrush);
	}

	void FileStatusCtrl::drawTipsContent(const RenderContext& renderCtx, const D2D1_COLOR_F& color, std::wstring_view tipsText) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + margin * 2;
		solidBrush->SetColor(color);
		renderTarget->DrawTextW(tipsText.data(),
		                        static_cast<UINT32>(tipsText.length()),
		                        app().textFormat().pTipsFormat,
		                        D2D1::RectF(padding, yPos, size().width - padding, yPos + tipsTextHeight),
		                        solidBrush);
	}

	void FileStatusCtrl::drawLoadingContent(const RenderContext& renderCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + tipsTextHeight + margin * 3;
		const D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
			D2D1::RectF(padding, yPos, size().width - padding, yPos + loadingIndicatorHeight),
			4.f, 4.f);
		solidBrush->SetColor(D2D1::ColorF(0xE0E5EC));
		renderTarget->FillRoundedRectangle(&trackRect, solidBrush);
	}

	void FileStatusCtrl::drawLoadingIndicator(const RenderContext& renderCtx, float progress) const
	{
		constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + tipsTextHeight + margin * 3;
		const float contentWidth = size().width - padding * 2;
		const float filledWidth = progress * contentWidth;
		m_pLoadingIndicator->setBounds(D2D1::RectF(padding, yPos, padding + filledWidth, yPos + loadingIndicatorHeight));
		m_pLoadingIndicator->draw(renderCtx);
	}

	void FileStatusCtrl::drawErrorRoundedArea(const RenderContext& renderCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + margin * 2;
		D2D1_RECT_F errorBgRect = D2D1::RectF(padding, yPos, size().width - padding, yPos + errorAreaHeight);
		const D2D1_ROUNDED_RECT errorBgRoundedRect = D2D1::RoundedRect(errorBgRect, 4.f, 4.f);
		solidBrush->SetColor(D2D1::ColorF(0xfdf6f6));
		renderTarget->FillRoundedRectangle(&errorBgRoundedRect, solidBrush);

		errorBgRect.right = errorBgRect.left + 3.f;
		renderTarget->PushAxisAlignedClip(errorBgRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		solidBrush->SetColor(D2D1::ColorF(0xd83b01));
		renderTarget->FillRoundedRectangle(&errorBgRoundedRect, solidBrush);
		renderTarget->PopAxisAlignedClip();
	}

	void FileStatusCtrl::drawErrorContent(const RenderContext& renderCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();
		const float contentRight = drawSize.width - padding;

		constexpr float xPos = padding + subAreaPadding;
		solidBrush->SetColor(D2D1::ColorF(0xd83b01));
		{
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + margin * 2 + subAreaPadding;
			const std::wstring errorMsg = std::format(L"错误描述：{}", m_errorMsg);
			renderTarget->DrawTextW(errorMsg.c_str(),
			                        static_cast<UINT32>(errorMsg.length()),
			                        app().textFormat().pErrorMsgFormat,
			                        D2D1::RectF(xPos, yPos, contentRight, yPos + statusTextHeight),
			                        solidBrush);
		}
		{
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight * 2 + margin * 3 + subAreaPadding;
			constexpr std::wstring_view message = L"您可以尝试以下解决方案：";
			renderTarget->DrawTextW(message.data(),
			                        static_cast<UINT32>(message.length()),
			                        app().textFormat().pErrorMsgFormat,
			                        D2D1::RectF(xPos, yPos, contentRight, yPos + statusTextHeight),
			                        solidBrush);
		}
		{
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight * 3 + margin * 4 + subAreaPadding;
			m_retryBtn->setBounds(D2D1::RectF(xPos, yPos, xPos + 64.f, yPos + statusTextHeight));
			m_retryBtn->draw(renderCtx);
		}
		solidBrush->SetColor(D2D1::ColorF(0xd83b01));
		{
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight * 4 + margin * 5 + subAreaPadding;
			constexpr std::wstring_view message = L"或者，手动下载文件并放置到指定目录";
			renderTarget->DrawTextW(message.data(),
			                        static_cast<UINT32>(message.length()),
			                        app().textFormat().pErrorMsgFormat,
			                        D2D1::RectF(xPos, yPos, contentRight, yPos + statusTextHeight),
			                        solidBrush);
		}
	}

	void FileStatusCtrl::drawInfoRoundedArea(const RenderContext& renderCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + errorAreaHeight + margin * 3;
		const D2D1_ROUNDED_RECT infoRoundedRect = D2D1::RoundedRect(
			D2D1::RectF(padding, yPos, size().width - padding, yPos + infoAreaHeight),
			4.f, 4.f);
		solidBrush->SetColor(D2D1::ColorF(0xf3f2f1));
		renderTarget->FillRoundedRectangle(&infoRoundedRect, solidBrush);
	}

	void FileStatusCtrl::drawInfoContent(const RenderContext& renderCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();
		const float contentRight = drawSize.width - padding - subAreaPadding;
		constexpr float xPos = padding + subAreaPadding;
		constexpr float labelRight = xPos + infoLabelWidth;
		constexpr float copyBtnWidth = 48.f;
		const float infoValueWidth = contentRight - labelRight - copyBtnWidth - margin;

		{
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + errorAreaHeight + margin * 3 + subAreaPadding;
			constexpr std::wstring_view label = L"下载URL：";
			solidBrush->SetColor(D2D1::ColorF(0x605e5c));
			renderTarget->DrawTextW(label.data(),
			                        static_cast<UINT32>(label.length()),
			                        app().textFormat().pTipsFormat,
			                        D2D1::RectF(xPos, yPos, labelRight, yPos + tipsTextHeight),
			                        solidBrush);
			solidBrush->SetColor(D2D1::ColorF(0x333333));
			float textWidth = infoValueWidth;
			if (m_pUrlLayout)
			{
				m_pUrlLayout->SetMaxWidth(infoValueWidth);
				DWRITE_TEXT_METRICS textMetrics;
				if (SUCCEEDED(m_pUrlLayout->GetMetrics(&textMetrics)))
				{
					textWidth = textMetrics.width;
				}
				renderTarget->DrawTextLayout(D2D1::Point2F(labelRight, yPos), m_pUrlLayout, solidBrush);
			}
			else
			{
				renderTarget->DrawTextW(m_downloadUrl.c_str(),
				                        static_cast<UINT32>(m_downloadUrl.length()),
				                        app().textFormat().pTipsFormat,
				                        D2D1::RectF(labelRight, yPos, labelRight + textWidth, yPos + tipsTextHeight),
				                        solidBrush);
			}
			const float btnXPos = labelRight + textWidth + margin;
			m_copyUrlBtn->setBounds(D2D1::RectF(btnXPos, yPos, btnXPos + copyBtnWidth, yPos + tipsTextHeight));
			m_copyUrlBtn->draw(renderCtx);
		}
		{
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + errorAreaHeight + margin * 4 + subAreaPadding + tipsTextHeight;
			constexpr std::wstring_view label = L"目标目录：";
			solidBrush->SetColor(D2D1::ColorF(0x605e5c));
			renderTarget->DrawTextW(label.data(),
			                        static_cast<UINT32>(label.length()),
			                        app().textFormat().pTipsFormat,
			                        D2D1::RectF(xPos, yPos, labelRight, yPos + tipsTextHeight),
			                        solidBrush);
			solidBrush->SetColor(D2D1::ColorF(0x333333));
			float textWidth = infoValueWidth;
			if (m_pDirLayout)
			{
				m_pDirLayout->SetMaxWidth(infoValueWidth);
				DWRITE_TEXT_METRICS textMetrics;
				if (SUCCEEDED(m_pDirLayout->GetMetrics(&textMetrics)))
				{
					textWidth = textMetrics.width;
				}
				renderTarget->DrawTextLayout(D2D1::Point2F(labelRight, yPos), m_pDirLayout, solidBrush);
			}
			else
			{
				renderTarget->DrawTextW(m_fileDir.c_str(),
				                        static_cast<UINT32>(m_fileDir.length()),
				                        app().textFormat().pTipsFormat,
				                        D2D1::RectF(labelRight, yPos, labelRight + textWidth, yPos + tipsTextHeight),
				                        solidBrush);
			}
			const float btnXPos = labelRight + textWidth + margin;
			m_copyDirBtn->setBounds(D2D1::RectF(btnXPos, yPos, btnXPos + copyBtnWidth, yPos + tipsTextHeight));
			m_copyDirBtn->draw(renderCtx);
		}
		{
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + errorAreaHeight + margin * 5 + subAreaPadding + tipsTextHeight * 2;
			constexpr std::wstring_view label = L"文件名：";
			solidBrush->SetColor(D2D1::ColorF(0x605e5c));
			renderTarget->DrawTextW(label.data(),
			                        static_cast<UINT32>(label.length()),
			                        app().textFormat().pTipsFormat,
			                        D2D1::RectF(xPos, yPos, labelRight, yPos + tipsTextHeight),
			                        solidBrush);
			solidBrush->SetColor(D2D1::ColorF(0x333333));
			float textWidth = infoValueWidth;
			if (m_pFileNameLayout)
			{
				m_pFileNameLayout->SetMaxWidth(infoValueWidth);
				DWRITE_TEXT_METRICS metrics;
				if (SUCCEEDED(m_pFileNameLayout->GetMetrics(&metrics)))
				{
					textWidth = metrics.width;
				}
				renderTarget->DrawTextLayout(D2D1::Point2F(labelRight, yPos), m_pFileNameLayout, solidBrush);
			}
			else
			{
				renderTarget->DrawTextW(m_fileName.c_str(),
				                        static_cast<UINT32>(m_fileName.length()),
				                        app().textFormat().pTipsFormat,
				                        D2D1::RectF(labelRight, yPos, labelRight + textWidth, yPos + tipsTextHeight),
				                        solidBrush);
			}
			const float btnXPos = labelRight + textWidth + margin;
			m_copyFileNameBtn->setBounds(D2D1::RectF(btnXPos, yPos, btnXPos + copyBtnWidth, yPos + tipsTextHeight));
			m_copyFileNameBtn->draw(renderCtx);
		}
	}

	////////////////////////////////////////////////////////////////////////////
	void FileStatusCtrl::drawInitial(const RenderContext& renderCtx) const
	{
		drawBaseContent(renderCtx);
		drawStatusContent(renderCtx, D2D1::ColorF(0xe3a21a), L"验证文件中...");
	}

	void FileStatusCtrl::drawVerified(const RenderContext& renderCtx) const
	{
		drawBaseContent(renderCtx);
		drawStatusContent(renderCtx, D2D1::ColorF(0x107c10), L"文件已存在，验证通过");
	}

	void FileStatusCtrl::drawDownloading(const RenderContext& renderCtx) const
	{
		drawBaseContent(renderCtx);
		drawStatusContent(renderCtx, D2D1::ColorF(0x0078d7), std::format(L"正在下载: {:.0f}%", m_progress * 100.f));
		drawTipsContent(renderCtx, D2D1::ColorF(0x605e5c), std::format(L"下载源: {}", m_downloadUrl));
		drawLoadingContent(renderCtx);
		drawLoadingIndicator(renderCtx, m_progress);
	}

	void FileStatusCtrl::drawWriting(const RenderContext& renderCtx) const
	{
		drawBaseContent(renderCtx);
		drawStatusContent(renderCtx, D2D1::ColorF(0x0078d7), L"下载完成，正在写入");
		drawTipsContent(renderCtx, D2D1::ColorF(0x605e5c), std::format(L"写入位置: {}", m_filePath));
		drawLoadingIndicator(renderCtx, 1.f);
	}

	void FileStatusCtrl::drawError(const RenderContext& renderCtx) const
	{
		drawBaseContent(renderCtx);
		drawStatusContent(renderCtx, D2D1::ColorF(0xd83b01), L"发生错误");
		drawErrorRoundedArea(renderCtx);
		drawErrorContent(renderCtx);
		drawInfoRoundedArea(renderCtx);
		drawInfoContent(renderCtx);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	coro::LazyTask<void> FileStatusCtrl::updateTotalSizeInMainThread(std::uint64_t total)
	{
		co_await sched::transfer_to(app().get_scheduler());
		m_totalLength += total;
		if (m_totalLength)
		{
			m_progress = static_cast<float>(m_currentSize * 1.0 / m_totalLength);
		}
		else
		{
			m_progress = 0.f;
		}
		update();
		co_return;
	}

	coro::LazyTask<void> FileStatusCtrl::updateCurrentSizeInMainThread(std::uint64_t size)
	{
		co_await sched::transfer_to(app().get_scheduler());
		m_currentSize += size;
		if (m_totalLength)
		{
			m_progress = static_cast<float>(m_currentSize * 1.0 / m_totalLength);
		}
		else
		{
			m_progress = 0.f;
		}
		update();
		co_return;
	}

	namespace
	{
		std::wstring utf8_to_wide_string(std::string_view utf8)
		{
			const int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
			if (len == 0)
			{
				throw std::runtime_error{std::format("MultiByteToWideChar fail, error code: {}", GetLastError())};
			}
			std::wstring result(len, 0);
			if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), result.data(), len))
			{
				throw std::runtime_error{std::format("MultiByteToWideChar fail, error code: {}", GetLastError())};
			}
			return result;
		}

		void write_file_thread(std::wstring_view pdbFile, std::span<std::byte> bytes, const coro::GuaranteedResolver<void>& resolver)
		{
			try
			{
				namespace fs = std::filesystem;
				fs::create_directories(fs::path{pdbFile}.parent_path());
			}
			catch (const std::exception& e)
			{
				resolver->rejectWithRuntimeError(std::format("create_directories failed, error msg: {}", e.what()));
				return;
			}

			HANDLE handle = CreateFileW(std::format(L"\\\\?\\{}", pdbFile).c_str(),
			                            GENERIC_READ | GENERIC_WRITE,
			                            0,
			                            nullptr,
			                            CREATE_ALWAYS,
			                            FILE_ATTRIBUTE_NORMAL,
			                            nullptr);
			if (handle == INVALID_HANDLE_VALUE)
			{
				resolver->rejectWithRuntimeError(std::format("CreateFileW failed, error code: {}", GetLastError()));
				return;
			}
			// On Windows 7: lpNumberOfBytesWritten parameter can not be NULL.
			DWORD numberOfBytesWritten = 0;
			if (!WriteFile(handle, bytes.data(), static_cast<DWORD>(bytes.size()), &numberOfBytesWritten, nullptr))
			{
				CloseHandle(handle);
				resolver->rejectWithRuntimeError(std::format("WriteFile failed, error code: {}", GetLastError()));
				return;
			}
			CloseHandle(handle);
			resolver->resolve();
		}

		coro::LazyTask<void> write_file(std::wstring_view pdbFile, std::span<std::byte> bytes)
		{
			co_await coro::LazyTask<void>::create([pdbFile, bytes](coro::GuaranteedResolver<void> resolver)
			{
				// 反正设计成开始写文件后不允许取消
				// 直接简单的创建一个线程阻塞式写文件
				std::thread{
					[pdbFile, bytes, res = std::move(resolver)]
					{
						write_file_thread(pdbFile, bytes, res);
					}
				}.detach();
			});
			co_return;
		}
	}

	coro::LazyTask<void> FileStatusCtrl::startAnaTaskImpl()
	{
		namespace fs = std::filesystem;

		std::wstring errMsg;

		try
		{
			m_painter.transferTo<EPainterType::Initial>();
			const fs::path filePath{m_filePath};
			if (!fs::exists(filePath))
			{
				m_connection = ms::get_default_win_http_session()->createConnection(m_downloadServerName);

				auto totalSizeCallback = [this](std::uint64_t total)
				{
					m_asyncScope.spawn(updateTotalSizeInMainThread(total));
				};
				auto currentSizeCallback = [this](std::uint64_t size)
				{
					m_asyncScope.spawn(updateCurrentSizeInMainThread(size));
				};
				// start download
				m_painter.transferTo<EPainterType::Downloading>();
				m_pLoadingIndicator->startAnim();
				updateWholeWnd();
				const std::shared_ptr<ms::WinHttpRequest> req = m_connection->openRequest(L"GET", m_downloadObjName);
				std::vector<std::byte> bytes = co_await req->request(totalSizeCallback, currentSizeCallback);
				co_await sched::transfer_to(app().get_scheduler());
				m_painter.transferTo<EPainterType::Writing>();
				updateWholeWnd();
				// 将bytes写入文件...
				co_await write_file(filePath.native(), bytes);
				if (!fs::exists(filePath))
				{
					throw std::runtime_error{"write file failed"};
				}
			}
		}
		catch (const std::exception& e)
		{
			errMsg = utf8_to_wide_string(e.what());
		}
		catch (...)
		{
			errMsg = L"unknown error in startAnaTask";
		}

		co_await sched::transfer_to(app().get_scheduler());
		m_pLoadingIndicator->stopAnim();

		if (errMsg.empty())
		{
			m_painter.transferTo<EPainterType::Verified>();
			if (m_resolver)
			{
				// 延后执行，因为resolve中外面可能要销毁此类，销毁此类又要等待此任务结束，所以如果直接resolve可能引起互相等待。
				app().get_scheduler().addTask([resolver = std::move(m_resolver)]
				{
					resolver->resolve();
				});
			}
		}
		else
		{
			m_errorMsg = errMsg;
			m_painter.transferTo<EPainterType::Error>();
		}
		updateWholeWnd();
	}
}
