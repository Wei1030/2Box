module UI.ProcessList;

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
}

namespace ui
{
	void ProcessList::initialize()
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

	void ProcessList::setEnv(const std::shared_ptr<biz::Env>& env)
	{
		m_env = env;
		std::vector<std::shared_ptr<biz::ProcessInfo>> allProc = env->getAllProcesses();
		for (const std::shared_ptr<biz::ProcessInfo>& proc : allProc)
		{
			m_processes.insert(std::make_pair(proc->getProcessId(), proc));
		}
		update();
	}

	void ProcessList::clearEnv()
	{
		m_env.reset();
		m_processes.clear();
		update();
	}

	void ProcessList::procCountChange(biz::Env::EProcEvent e, const std::shared_ptr<biz::ProcessInfo>& proc)
	{
		if (e == biz::Env::EProcEvent::Create)
		{
			if (m_processes.contains(proc->getProcessId()))
			{
				return;
			}
			m_processes.insert(std::make_pair(proc->getProcessId(), proc));
		}
		else if (e == biz::Env::EProcEvent::Terminate)
		{
			m_processes.erase(proc->getProcessId());
		}
		update();
	}

	void ProcessList::onResize(float width, float height)
	{
		const float launchBtnXPos = width - PADDING - BUTTON_WIDTH;
		m_btnLaunch.setBounds(D2D1::RectF(launchBtnXPos, PADDING, launchBtnXPos + BUTTON_WIDTH, PADDING + BUTTON_HEIGHT));

		const float clearBtnXPos = launchBtnXPos - GAP * 2.f - CLEAR_BTN_WIDTH;
		constexpr float clearBtnYPos = PADDING + (BUTTON_HEIGHT - CLEAR_BTN_HEIGHT) * 0.5f;
		m_btnClear.setBounds(D2D1::RectF(clearBtnXPos, clearBtnYPos, clearBtnXPos + CLEAR_BTN_WIDTH, clearBtnYPos + CLEAR_BTN_HEIGHT));
	}

	void ProcessList::drawImpl(const RenderContext& renderCtx)
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
	}

	void ProcessList::setProcPath(std::wstring_view path)
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

	void ProcessList::onLaunchBtnClick()
	{
		if (!m_env)
		{
			return;
		}
		if (m_strProcPath.size())
		{
			biz::launcher().run(m_env, m_strProcPath);
			return;
		}

		const std::optional<std::wstring> fullPath = select_file(m_ownerWnd);
		if (!fullPath.has_value())
		{
			return;
		}
		setProcPath(fullPath.value());

		biz::launcher().run(m_env, m_strProcPath);
	}
}
