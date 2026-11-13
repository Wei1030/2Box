module UI.EnvDetail;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import UI.Core;

namespace
{
	constexpr float PADDING = 16.f;
	constexpr float GAP = 8.f;
	constexpr float BUTTON_WIDTH = 78.f;
	constexpr float BUTTON_HEIGHT = 36.f;
	constexpr float CLEAR_BTN_WIDTH = 24.f;
	constexpr float CLEAR_BTN_HEIGHT = CLEAR_BTN_WIDTH;
	constexpr float SCROLL_WIDTH = 8.f;
	constexpr float WHEEL_SCROLL_SIZE = 24.f;
	constexpr float LIST_Y_POS_START = PADDING + BUTTON_HEIGHT + GAP;
	constexpr float LIST_ITEM_HEIGHT = 94.f;

	constexpr float LIST_ITEM_GAP = 4.f;
	constexpr float LIST_ITEM_TITLE_HEIGHT = 24.f;
	constexpr float LIST_ITEM_TIPS_HEIGHT = 20.f;

	constexpr float TERMINATE_BTN_WIDTH = 24.f;
	constexpr float TERMINATE_BTN_HEIGHT = 24.f;
	constexpr float TERMINATE_BTN_MARGIN_LEFT = 8.f;

	constexpr std::wstring_view NO_PROC_TEXT = L"暂无进程";
}

namespace ui
{
	void ProcessList::initialize()
	{
		m_scrollBar.setThumbPosChangeNotify([this] { updateAllItemPos(); });
	}

	void ProcessList::setEnv(const std::shared_ptr<biz::Env>& env)
	{
		m_env = env;
		m_processes.clear();
		std::vector<std::shared_ptr<biz::ProcessInfo>> allProc = env->getAllProcesses();
		for (const std::shared_ptr<biz::ProcessInfo>& proc : allProc)
		{
			m_processes.insert(std::make_pair(proc->getProcessId(), ListItem{proc}));
		}
		rebuildTerminateButtons();
		m_scrollBar.setTotalSize(m_processes.size() * LIST_ITEM_HEIGHT);
	}

	void ProcessList::clearEnv()
	{
		m_env.reset();
		m_processes.clear();
		m_terminateButtons.clear();
		m_scrollBar.setTotalSize(0.f);
	}

	void ProcessList::procCountChange(biz::Env::EProcEvent e, const std::shared_ptr<biz::ProcessInfo>& proc)
	{
		if (e == biz::Env::EProcEvent::Create)
		{
			if (m_processes.contains(proc->getProcessId()))
			{
				return;
			}
			m_processes.insert(std::make_pair(proc->getProcessId(), ListItem{proc}));
		}
		else if (e == biz::Env::EProcEvent::Terminate)
		{
			m_processes.erase(proc->getProcessId());
		}
		rebuildTerminateButtons();
		m_scrollBar.setTotalSize(m_processes.size() * LIST_ITEM_HEIGHT);
	}

	void ProcessList::rebuildTerminateButtons()
	{
		m_terminateButtons.clear();
		for (const auto& item : m_processes)
		{
			const std::uint32_t pid = item.first;
			std::unique_ptr<Button> button = std::make_unique<Button>(this);
			button->setBackgroundColor(D2D1::ColorF(0xfff5f5), Button::EState::Normal);
			button->setBackgroundColor(D2D1::ColorF(0xffebee), Button::EState::Hover);
			button->setBackgroundColor(D2D1::ColorF(0xffcdd2), Button::EState::Active);
			button->setBorderColor(D2D1::ColorF(0xef9a9a), Button::EState::Normal);
			button->setBorderColor(D2D1::ColorF(0xf44336), Button::EState::Hover);
			button->setBorderColor(D2D1::ColorF(0xf44336), Button::EState::Active);
			button->setRadius(4.f);
			// 正方形按钮内绘制一个大红叉（终结该进程）
			button->setDrawCallback([rawButton = button.get()](const RenderContext& renderCtx, Button::EState state)
			{
				const D2D1_RECT_F& bounds = rawButton->getBounds();
				const float width = bounds.right - bounds.left;
				const float height = bounds.bottom - bounds.top;
				const float padding = std::min(width, height) * 0.25f;
				const float thickness = 2.5f;
				const D2D1_COLOR_F color = (state == Button::EState::Hover || state == Button::EState::Active)
					                           ? D2D1::ColorF(0xc62828)
					                           : D2D1::ColorF(0xe53935);
				renderCtx.brush->SetColor(color);
				renderCtx.renderTarget->DrawLine(D2D1::Point2F(padding, padding),
				                                 D2D1::Point2F(width - padding, height - padding),
				                                 renderCtx.brush, thickness);
				renderCtx.renderTarget->DrawLine(D2D1::Point2F(width - padding, padding),
				                                 D2D1::Point2F(padding, height - padding),
				                                 renderCtx.brush, thickness);
			});
			button->setOnClick([this, pid] { onTerminateProcessClicked(pid); });
			m_terminateButtons.insert(std::make_pair(pid, std::move(button)));
		}
		updateTerminateButtonPos();
	}

	void ProcessList::updateTerminateButtonPos()
	{
		if (m_terminateButtons.empty())
		{
			return;
		}
		// 不可见的列表项把按钮移到控件外，避免误点击
		const D2D1_RECT_F hiddenBounds = D2D1::RectF(-10000.f, -10000.f, -9999.f, -9999.f);
		for (auto& pair : m_terminateButtons)
		{
			pair.second->setBounds(hiddenBounds);
		}
		for (const ListItem* item : m_processesToDraw)
		{
			const auto it = m_terminateButtons.find(item->process->getProcessId());
			if (it == m_terminateButtons.end())
			{
				continue;
			}
			const D2D1_RECT_F& rc = item->rect;
			const float top = rc.top + PADDING + (LIST_ITEM_TITLE_HEIGHT - TERMINATE_BTN_HEIGHT) * 0.5f;
			const float left = rc.right - TERMINATE_BTN_WIDTH;
			it->second->setBounds(D2D1::RectF(left, top, left + TERMINATE_BTN_WIDTH, top + TERMINATE_BTN_HEIGHT));		}
	}

	void ProcessList::onTerminateProcessClicked(std::uint32_t pid)
	{
		if (!m_env)
		{
			return;
		}
		const std::shared_ptr<biz::ProcessInfo> process = m_env->getProcess(pid);
		if (!process)
		{
			return;
		}
		namespace fs = std::filesystem;
		const std::wstring procName = fs::path{process->getProcessFullPath()}.stem().native();
		const std::wstring message = std::format(L"确定要终结进程「{}」吗？\n\n强制终结可能导致进程数据丢失。", procName);
		if (MessageBoxW(m_ownerWnd->nativeHandle(), message.c_str(), MainApp::appName.data(),
		                MB_OKCANCEL | MB_ICONWARNING) != IDOK)
		{
			return;
		}
		if (!m_env->terminateProcess(pid))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(), L"终结进程失败，该进程可能已经退出。",
			            MainApp::appName.data(), MB_OK | MB_ICONERROR);
		}
	}

	void ProcessList::onResize(float width, float height)
	{
		m_scrollBar.setVisibleSize(height);
		m_scrollBar.setBounds(D2D1::RectF(width - SCROLL_WIDTH - 4.f, 0, width - 4.f, height));
	}

	void ProcessList::onMouseEnter(const MouseEvent& e)
	{
		e.accept = true;

		m_isHovered = true;
		update();
	}

	void ProcessList::onMouseLeave(const MouseEvent& e)
	{
		e.accept = true;

		m_isHovered = false;
		update();
	}

	void ProcessList::onMouseWheel(const MouseWheelEvent& e)
	{
		e.accept = true;

		const float wheelCount = e.zDelta / 120.f;
		m_scrollBar.scroll(-wheelCount * WHEEL_SCROLL_SIZE);
	}

	void ProcessList::drawImpl(const RenderContext& renderCtx)
	{
		if (m_processesToDraw.empty())
		{
			return;
		}

		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;

		namespace fs = std::filesystem;
		app().textFormat().setTitleEllipsisTrimming();
		app().textFormat().setTipsEllipsisTrimming();
		for (auto it = m_processesToDraw.begin(); it != m_processesToDraw.end(); ++it)
		{
			ListItem* item = *it;
			biz::ProcessInfo* processInfo = item->process.get();
			const D2D1_RECT_F& rc = item->rect;
			const fs::path fullPath{processInfo->getProcessFullPath()};
			const fs::path procName = fullPath.stem();
			float yPos = rc.top + PADDING;
			solidBrush->SetColor(D2D1::ColorF(0x1a1a1a));
			renderTarget->DrawTextW(procName.native().c_str(),
			                        static_cast<UINT32>(procName.native().size()),
			                        app().textFormat().pTitleFormat,
			                        D2D1::RectF(0.f, yPos, rc.right - TERMINATE_BTN_WIDTH - TERMINATE_BTN_MARGIN_LEFT, yPos + LIST_ITEM_TITLE_HEIGHT), solidBrush);
			yPos += LIST_ITEM_TITLE_HEIGHT + LIST_ITEM_GAP;
			const std::wstring pid = std::format(L"PID: {}", processInfo->getProcessId());
			solidBrush->SetColor(D2D1::ColorF(0x666666));
			renderTarget->DrawTextW(pid.c_str(),
			                        static_cast<UINT32>(pid.size()),
			                        app().textFormat().pTipsFormat,
			                        D2D1::RectF(0.f, yPos, rc.right, yPos + LIST_ITEM_TIPS_HEIGHT), solidBrush);
			yPos += LIST_ITEM_TIPS_HEIGHT + LIST_ITEM_GAP;
			renderTarget->DrawTextW(fullPath.native().c_str(),
			                        static_cast<UINT32>(fullPath.native().size()),
			                        app().textFormat().pTipsFormat,
			                        D2D1::RectF(0.f, yPos, rc.right, yPos + LIST_ITEM_TIPS_HEIGHT), solidBrush);

			// 每个进程条目右上角的“终结”按钮
			if (const auto btnIt = m_terminateButtons.find(processInfo->getProcessId()); btnIt != m_terminateButtons.end())
			{
				btnIt->second->draw(renderCtx);
			}

			solidBrush->SetColor(D2D1::ColorF(0xf0f0f0));
			renderTarget->DrawLine(D2D1::Point2F(rc.left, rc.bottom), D2D1::Point2F(rc.right, rc.bottom), solidBrush);
		}
		app().textFormat().clearTitleTrimming();
		app().textFormat().clearTipsEllipsisTrimming();

		if (m_isHovered || m_scrollBar.isThumbPressed())
		{
			m_scrollBar.draw(renderCtx);
		}
	}

	void ProcessList::updateAllItemPos()
	{
		if (m_processes.empty())
		{
			m_processesToDraw.clear();
			updateTerminateButtonPos();
			update();
			return;
		}

		const float scrollOffset = m_scrollBar.getScrollOffset();
		const std::uint32_t startIndex = static_cast<std::uint32_t>(scrollOffset / LIST_ITEM_HEIGHT);
		const std::uint32_t endIndex = std::min(
			static_cast<std::uint32_t>((scrollOffset + m_scrollBar.getVisibleSize()) / LIST_ITEM_HEIGHT),
			static_cast<std::uint32_t>(m_processes.size() - 1));
		m_processesToDraw.clear();

		if (startIndex > endIndex)
		{
			updateTerminateButtonPos();
			update();
			return;
		}

		m_processesToDraw.reserve(endIndex - startIndex);

		const std::uint32_t startDrawOffsetY = static_cast<std::uint32_t>(scrollOffset) % static_cast<std::uint32_t>(LIST_ITEM_HEIGHT);
		float startYPos = 0.f - startDrawOffsetY;
		const auto drawSize = size();
		std::size_t index = 0;
		for (auto it = m_processes.begin(); it != m_processes.end(); ++it, ++index)
		{
			if (index < startIndex || index > endIndex)
			{
				continue;
			}
			ListItem& item = it->second;
			item.rect = D2D1::RectF(0.f, startYPos, drawSize.width - PADDING, startYPos + LIST_ITEM_HEIGHT);
			m_processesToDraw.push_back(&item);
			startYPos += LIST_ITEM_HEIGHT;
		}
		updateTerminateButtonPos();
		update();
	}

	void EnvDetail::initialize()
	{
		m_btnClear.setText(L"x");
		m_btnClear.setBackgroundColor(D2D1::ColorF(0xe0e0e0));
		m_btnClear.setBackgroundColor(D2D1::ColorF(0xf0f0f0), Button::EState::Normal);
		m_btnClear.setTextColor(D2D1::ColorF(0x333333));
		m_btnClear.setTextFormat(app().textFormat().pToolBtnFormat);
		m_btnClear.setOnClick([this]
		{
			m_strProcPath.clear();
			update();
		});

		m_btnLaunch.setText(L"添加进程");
		m_btnLaunch.setBackgroundColor(D2D1::ColorF(0x000000, 0.f), Button::EState::Normal);
		m_btnLaunch.setBackgroundColor(D2D1::ColorF(0xe3f2fd), Button::EState::Hover);
		m_btnLaunch.setBackgroundColor(D2D1::ColorF(0xbbdefb), Button::EState::Active);
		m_btnLaunch.setBorderColor(D2D1::ColorF(0x0078d4));
		m_btnLaunch.setTextColor(D2D1::ColorF(0x0078d4));
		m_btnLaunch.setOnClick([this] { onLaunchBtnClick(); });

		m_noProcTextHeight = LIST_ITEM_TITLE_HEIGHT;
		if (SUCCEEDED(app().dWriteFactory()->CreateTextLayout(NO_PROC_TEXT.data(),
			static_cast<UINT32>(NO_PROC_TEXT.size()),
			app().textFormat().pMainFormat,
			std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
			&m_noProcTextLayout)))
		{
			m_noProcTextLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(m_noProcTextLayout->GetMetrics(&textMetrics)))
			{
				m_noProcTextHeight = textMetrics.height;
			}
		}
	}

	void EnvDetail::onResize(float width, float height)
	{
		const float launchBtnXPos = width - PADDING - BUTTON_WIDTH;
		m_btnLaunch.setBounds(D2D1::RectF(launchBtnXPos, PADDING, launchBtnXPos + BUTTON_WIDTH, PADDING + BUTTON_HEIGHT));

		const float clearBtnXPos = launchBtnXPos - GAP * 2.f - CLEAR_BTN_WIDTH;
		constexpr float clearBtnYPos = PADDING + (BUTTON_HEIGHT - CLEAR_BTN_HEIGHT) * 0.5f;
		m_btnClear.setBounds(D2D1::RectF(clearBtnXPos, clearBtnYPos, clearBtnXPos + CLEAR_BTN_WIDTH, clearBtnYPos + CLEAR_BTN_HEIGHT));

		m_processList.setBounds(D2D1::RectF(PADDING, LIST_Y_POS_START, width, height - PADDING));
	}

	void EnvDetail::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto [width, height] = size();

		const D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(0.f, 0.f, width, height), 8.f, 8.f);
		// 设置了背景图片时面板底色半透明，让背景图片透出
		solidBrush->SetColor(ui::has_app_background_image() ? D2D1::ColorF(0xffffff, 0.12f) : D2D1::ColorF(0xffffff));
		renderTarget->FillRoundedRectangle(roundedRect, solidBrush);
		solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
		renderTarget->DrawRoundedRectangle(roundedRect, solidBrush);

		if (m_strProcPath.size())
		{
			const D2D1_ROUNDED_RECT pathRect = D2D1::RoundedRect(D2D1::RectF(PADDING, PADDING,
			                                                                 width - PADDING - BUTTON_WIDTH - GAP,
			                                                                 PADDING + BUTTON_HEIGHT), 8.0f, 8.0f);
			solidBrush->SetColor(D2D1::ColorF(0xf8f9fa));
			renderTarget->FillRoundedRectangle(pathRect, solidBrush);
			solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
			renderTarget->DrawRoundedRectangle(pathRect, solidBrush);

			solidBrush->SetColor(D2D1::ColorF(0x1a1a1a));

			constexpr float textXPos = PADDING + GAP;
			const float textYPos = PADDING + (BUTTON_HEIGHT - m_pathTextHeight) * 0.5f;
			const float textWidth = width - PADDING - BUTTON_WIDTH - GAP * 3.f - CLEAR_BTN_WIDTH - textXPos;
			if (m_procPathTextLayout)
			{
				m_procPathTextLayout->SetMaxWidth(textWidth);
				renderTarget->DrawTextLayout(D2D1::Point2F(textXPos, textYPos), m_procPathTextLayout, solidBrush);
			}
			else
			{
				app().textFormat().setMainEllipsisTrimming();
				renderTarget->DrawTextW(m_strProcPath.c_str(),
				                        static_cast<UINT32>(m_strProcPath.length()),
				                        app().textFormat().pMainFormat,
				                        D2D1::RectF(textXPos, textYPos, textXPos + textWidth, PADDING + BUTTON_HEIGHT),
				                        solidBrush);
				app().textFormat().clearMainEllipsisTrimming();
			}
			m_btnClear.draw(renderCtx);
		}

		m_btnLaunch.draw(renderCtx);

		solidBrush->SetColor(D2D1::ColorF(0xf0f0f0));
		renderTarget->DrawLine(D2D1::Point2F(PADDING, LIST_Y_POS_START), D2D1::Point2F(width - PADDING, LIST_Y_POS_START), solidBrush);

		if (m_processList.hasAnyProcesses())
		{
			m_processList.draw(renderCtx);
		}
		else
		{
			solidBrush->SetColor(D2D1::ColorF(0x333333));
			const float listAreaHeight = height - PADDING - LIST_Y_POS_START - PADDING;
			const float textYPos = (listAreaHeight - m_noProcTextHeight) * 0.5f;
			if (m_noProcTextLayout)
			{
				m_noProcTextLayout->SetMaxWidth(width - PADDING - PADDING);
				m_noProcTextLayout->SetMaxHeight(listAreaHeight);
				renderTarget->DrawTextLayout(D2D1::Point2F(PADDING, textYPos), m_noProcTextLayout, solidBrush);
			}
			else
			{
				DWRITE_TEXT_ALIGNMENT oldAlignment = app().textFormat().pMainFormat->GetTextAlignment();
				app().textFormat().pMainFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				renderTarget->DrawTextW(NO_PROC_TEXT.data(), static_cast<UINT32>(NO_PROC_TEXT.size()), app().textFormat().pMainFormat,
				                        D2D1::RectF(PADDING, textYPos, width - PADDING, height - PADDING), solidBrush);
				app().textFormat().pMainFormat->SetTextAlignment(oldAlignment);
			}
		}
	}

	void EnvDetail::setProcPath(std::wstring_view path)
	{
		m_strProcPath = path;
		if (m_strProcPath.size())
		{
			m_pathTextHeight = CLEAR_BTN_HEIGHT;
			m_procPathTextLayout.reset();
			app().textFormat().setMainEllipsisTrimming();
			if (SUCCEEDED(app().dWriteFactory()->CreateTextLayout(m_strProcPath.c_str(),
				static_cast<UINT32>(m_strProcPath.size()),
				app().textFormat().pMainFormat,
				std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
				&m_procPathTextLayout)))
			{
				DWRITE_TEXT_METRICS textMetrics;
				if (SUCCEEDED(m_procPathTextLayout->GetMetrics(&textMetrics)))
				{
					m_pathTextHeight = textMetrics.height;
				}
			}
			app().textFormat().clearMainEllipsisTrimming();
		}
		update();
	}

	void EnvDetail::onLaunchBtnClick()
	{
		if (!m_processList.hasEnv())
		{
			return;
		}
		if (m_strProcPath.size())
		{
			biz::launcher().run(m_processList.getEnv(), m_strProcPath);
			return;
		}

		const std::optional<std::wstring> fullPath = select_file(m_ownerWnd);
		if (!fullPath.has_value())
		{
			return;
		}
		setProcPath(fullPath.value());

		biz::launcher().run(m_processList.getEnv(), m_strProcPath);
	}
}
