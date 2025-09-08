module UI.EnvDetail;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;

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
		m_scrollBar.setTotalSize(m_processes.size() * LIST_ITEM_HEIGHT);
	}

	void ProcessList::clearEnv()
	{
		m_env.reset();
		m_processes.clear();
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
		m_scrollBar.setTotalSize(m_processes.size() * LIST_ITEM_HEIGHT);
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
			                        D2D1::RectF(0.f, yPos, rc.right, yPos + LIST_ITEM_TITLE_HEIGHT), solidBrush);
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
		m_btnLaunch.setBackgroundColor(D2D1::ColorF(0xe3f2fd));
		m_btnLaunch.setBackgroundColor(D2D1::ColorF(0x000000, 0.f), Button::EState::Normal);
		m_btnLaunch.setBorderColor(D2D1::ColorF(0x0078d4));
		m_btnLaunch.setTextColor(D2D1::ColorF(0x0078d4));
		m_btnLaunch.setOnClick([this] { onLaunchBtnClick(); });
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
		solidBrush->SetColor(D2D1::ColorF(0xffffff));
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
