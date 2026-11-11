module UI.EnvBoxCard;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import Scheduler;
import Biz.Core;
import UI.Core;

namespace
{
	constexpr float PADDING = 16.f;
	constexpr float LINE1_GAP = 8.f;
	// constexpr float LINE2_GAP = 12.f;
	constexpr float TITLE_HEIGHT = 24.f;
	constexpr float COUNT_HEIGHT = 21.f;
	// constexpr float START_BTN_WIDTH = 78.f;
	// constexpr float START_BTN_HEIGHT = 33.f;
	constexpr float DELETE_BTN_WIDTH = 20.f;
	constexpr float DELETE_BTN_HEIGHT = 20.f;
	constexpr float RENAME_BTN_WIDTH = 20.f;
	constexpr float RENAME_BTN_HEIGHT = 20.f;
	constexpr float TOOL_BTN_GAP = 6.f;
	constexpr float TITLE_BTN_GAP = 8.f;
}

namespace ui
{
	EnvBoxCard::~EnvBoxCard()
	{
		m_stopSource.request_stop();
		// 不再接收通知，且会等待已经通知的回调结束
		m_env->setProcCountChangeNotify(nullptr);
		// 之后就绝对不会spawn新的协程，才可以安全等待所有协程结束
		m_asyncScope.join();
	}

	void EnvBoxCard::setEnv(const std::shared_ptr<biz::Env>& env)
	{
		m_env = env;
		m_name = m_env->getName();
		m_strProcCount = std::format(L"{}", m_env->getAllProcessesCount());

		m_env->setProcCountChangeNotify([this](biz::Env::EProcEvent e, const std::shared_ptr<biz::ProcessInfo>& proc, std::size_t count)
		{
			m_asyncScope.spawn(onProcessCountChange(e, proc, count));
		});
	}

	void EnvBoxCard::setBusyTemp()
	{
		// 由于无法保证启动的进程一定能立即反映在env中，所以简单设置一个是否空闲的标志，env在启动任意一个进程后的1.6s内属于非空闲状态
		// 非空闲的env无法被 “点击总启动按钮时” 挑选到
		m_stopSource.request_stop();
		m_bIdle = false;
		m_stopSource = std::stop_source{};
		m_asyncScope.spawn(coro::co_with_cancellation(resetToIdleLater(), m_stopSource.get_token()));
	}

	void EnvBoxCard::programmaticDeselect()
	{
		m_isSelected = false;
	}

	void EnvBoxCard::refreshName()
	{
		if (!m_env)
		{
			return;
		}
		m_name = m_env->getName();
		updateWholeWnd();
	}

	void EnvBoxCard::onRenameBtnClick()
	{
		if (!m_env)
		{
			return;
		}

		const std::wstring currentName{m_env->getName()};
		const std::wstring dialogPrompt = std::format(L"请输入新的显示名称（底层编号：{}）：", m_env->getIndex());
		const std::optional<std::wstring> newName = ui::prompt_text_input(m_ownerWnd->nativeHandle(),
		                                                                  L"重命名环境",
		                                                                  dialogPrompt,
		                                                                  currentName,
		                                                                  biz::ENV_NAME_MAX_LENGTH);
		if (!newName.has_value() || newName.value() == currentName)
		{
			return;
		}

		try
		{
			biz::env_mgr().renameEnv(m_env, newName.value());
		}
		catch (const std::exception& e)
		{
			const std::wstring errorMsg = utf8_to_wide_string(e.what());
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"重命名环境失败：{}", errorMsg).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR);
			return;
		}

		refreshName();
	}

	void EnvBoxCard::initialize()
	{
		// m_btnStart = std::make_unique<Button>(this);
		// m_btnStart->setText(L"添加进程");
		// m_btnStart->setBackgroundColor(D2D1::ColorF(0xe3f2fd));
		// m_btnStart->setBackgroundColor(D2D1::ColorF(0x000000, 0.f), Button::EState::Normal);
		// m_btnStart->setBorderColor(D2D1::ColorF(0x0078d4));
		// m_btnStart->setTextColor(D2D1::ColorF(0x0078d4));
		// m_btnStart->setOnClick([this] { onBtnStartPressed(); });

		m_btnRename = std::make_unique<Button>(this);
		m_btnRename->setText(L"改");
		m_btnRename->setBackgroundColor(D2D1::ColorF(0x0078d4, 0.f));
		m_btnRename->setBackgroundColor(D2D1::ColorF(0xe3f2fd), Button::EState::Hover);
		m_btnRename->setBackgroundColor(D2D1::ColorF(0xbbdefb), Button::EState::Active);
		m_btnRename->setBorderColor(D2D1::ColorF(0x90caf9));
		m_btnRename->setBorderColor(D2D1::ColorF(0x0078d4), Button::EState::Hover);
		m_btnRename->setBorderColor(D2D1::ColorF(0x0078d4), Button::EState::Active);
		m_btnRename->setTextColor(D2D1::ColorF(0x0078d4));
		m_btnRename->setTextFormat(app().textFormat().pToolBtnFormat);
		m_btnRename->setOnClick([this] { onRenameBtnClick(); });

		m_btnDelete = std::make_unique<Button>(this);
		m_btnDelete->setText(L"x");
		m_btnDelete->setBackgroundColor(D2D1::ColorF(0xffebee));
		m_btnDelete->setBackgroundColor(D2D1::ColorF(0x000000, 0.f), Button::EState::Normal);
		m_btnDelete->setBorderColor(D2D1::ColorF(0xf44336));
		m_btnDelete->setBorderColor(D2D1::ColorF(0xe0e0e0), Button::EState::Normal);
		m_btnDelete->setTextColor(D2D1::ColorF(0x333333));
		m_btnDelete->setTextFormat(app().textFormat().pToolBtnFormat);
		m_btnDelete->setOnClick([this]
		{
			if (m_procCount)
			{
				MessageBoxW(m_ownerWnd->nativeHandle(), L"环境中仍有正在运行的程序时无法删除", MainApp::appName.data(), MB_OK);
			}
			else if (MessageBoxW(m_ownerWnd->nativeHandle(), L"确定要删除该环境吗?", MainApp::appName.data(), MB_OKCANCEL) == IDOK)
			{
				biz::env_mgr().deleteEnv(m_env);
			}
		});

		if (!m_bIdle)
		{
			m_stopSource = std::stop_source{};
			m_asyncScope.spawn(coro::co_with_cancellation(resetToIdleLater(), m_stopSource.get_token()));
		}
	}

	void EnvBoxCard::onResize(float width, float height)
	{
		const float deleteBtnXPos = width - PADDING - DELETE_BTN_WIDTH;
		m_btnDelete->setBounds(D2D1::RectF(deleteBtnXPos, PADDING, deleteBtnXPos + DELETE_BTN_WIDTH, PADDING + DELETE_BTN_HEIGHT));

		const float renameBtnXPos = deleteBtnXPos - TOOL_BTN_GAP - RENAME_BTN_WIDTH;
		m_btnRename->setBounds(D2D1::RectF(renameBtnXPos, PADDING, renameBtnXPos + RENAME_BTN_WIDTH, PADDING + RENAME_BTN_HEIGHT));

		// constexpr float startBtnYPos = PADDING + TITLE_HEIGHT + LINE1_GAP + COUNT_HEIGHT + LINE2_GAP;
		// m_btnStart->setBounds(D2D1::RectF(PADDING, startBtnYPos, PADDING + START_BTN_WIDTH, startBtnYPos + START_BTN_HEIGHT));
	}

	void EnvBoxCard::onMouseEnter(const MouseEvent& e)
	{
		m_isHovered = true;
		updateWholeWnd();
	}

	void EnvBoxCard::onMouseLeave(const MouseEvent& e)
	{
		// 1.进入子控件会触发leave
		// 2.离开子控件也会触发leave(除非子控件拦截)
		if (!hitTest(e.point))
		{
			m_isHovered = false;
			updateWholeWnd();
		}
	}

	void EnvBoxCard::onClick(const MouseEvent& e)
	{
		m_isSelected = !m_isSelected;

		if (m_pfnOnSelect)
		{
			m_pfnOnSelect(m_isSelected);
		}
	}

	void EnvBoxCard::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		const D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
			D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height),
			12.0f, 12.0f);
		if (m_isHovered || m_isSelected || m_isBright)
		{
			solidBrush->SetColor(D2D1::ColorF(0xf0f8ff));
			renderTarget->FillRoundedRectangle(roundedRect, solidBrush);

			solidBrush->SetColor(D2D1::ColorF(0x0078d4));
			renderTarget->DrawRoundedRectangle(roundedRect, solidBrush);
		}
		else
		{
			solidBrush->SetColor(D2D1::ColorF(0xffffff));
			renderTarget->FillRoundedRectangle(roundedRect, solidBrush);

			solidBrush->SetColor(D2D1::ColorF(0xe9e9e9));
			renderTarget->DrawRoundedRectangle(roundedRect, solidBrush);
		}

		const float titleRight = drawSize.width - PADDING - DELETE_BTN_WIDTH - TOOL_BTN_GAP - RENAME_BTN_WIDTH - TITLE_BTN_GAP;
		solidBrush->SetColor(D2D1::ColorF(0x333333));
		app().textFormat().setTitleEllipsisTrimming();
		renderTarget->DrawTextW(m_name.c_str(),
		                        static_cast<UINT32>(m_name.length()),
		                        app().textFormat().pTitleFormat,
		                        D2D1::RectF(PADDING, PADDING, titleRight, PADDING + TITLE_HEIGHT),
		                        solidBrush);
		app().textFormat().clearTitleTrimming();
		m_btnRename->draw(renderCtx);
		m_btnDelete->draw(renderCtx);

		solidBrush->SetColor(D2D1::ColorF(0x666666));
		constexpr std::wstring_view countLabel = L"进程数量：";
		constexpr float countLabelTop = PADDING + TITLE_HEIGHT + LINE1_GAP;
		renderTarget->DrawTextW(countLabel.data(),
		                        static_cast<UINT32>(countLabel.length()),
		                        app().textFormat().pTipsFormat,
		                        D2D1::RectF(PADDING, countLabelTop,
		                                    drawSize.width - PADDING, countLabelTop + COUNT_HEIGHT),
		                        solidBrush);
		solidBrush->SetColor(D2D1::ColorF(0x0078d4));
		renderTarget->DrawTextW(m_strProcCount.c_str(),
		                        static_cast<UINT32>(m_strProcCount.length()),
		                        app().textFormat().pTipsFormat,
		                        D2D1::RectF(PADDING + 64.f, countLabelTop,
		                                    drawSize.width - PADDING, countLabelTop + COUNT_HEIGHT),
		                        solidBrush);

		// 重命名只修改界面显示名称，底层的数字编号始终展示，便于识别环境
		if (m_env)
		{
			const std::wstring indexLabel = std::format(L"编号：{}", m_env->getIndex());
			IDWriteTextFormat* const tipsFormat = app().textFormat().pTipsFormat;
			const DWRITE_TEXT_ALIGNMENT oldAlignment = tipsFormat->GetTextAlignment();
			tipsFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
			solidBrush->SetColor(D2D1::ColorF(0x999999));
			renderTarget->DrawTextW(indexLabel.c_str(),
			                        static_cast<UINT32>(indexLabel.length()),
			                        tipsFormat,
			                        D2D1::RectF(PADDING, countLabelTop,
			                                    drawSize.width - PADDING, countLabelTop + COUNT_HEIGHT),
			                        solidBrush);
			tipsFormat->SetTextAlignment(oldAlignment);
		}

		// m_btnStart->draw(renderCtx);
	}

	coro::LazyTask<void> EnvBoxCard::resetToIdleLater()
	{
		std::uint32_t twinkleCount = 4;
		while (twinkleCount > 0)
		{
			m_isBright = true;
			updateWholeWnd();
			co_await sched::transfer_after(std::chrono::milliseconds{200}, app().get_scheduler());
			m_isBright = false;
			updateWholeWnd();
			co_await sched::transfer_after(std::chrono::milliseconds{200}, app().get_scheduler());
			twinkleCount--;
		}

		m_bIdle = true;
	}

	coro::LazyTask<void> EnvBoxCard::onProcessCountChange(biz::Env::EProcEvent e, std::shared_ptr<biz::ProcessInfo> proc, std::size_t count)
	{
		// 转到主线程
		co_await sched::transfer_to(app().get_scheduler());
		m_procCount = count;
		m_strProcCount = std::format(L"{}", count);

		if (m_isSelected)
		{
			if (m_pfnOnProcCountChange)
			{
				m_pfnOnProcCountChange(e, proc);
			}
		}

		update();
	}

	// void EnvBoxCard::onBtnStartPressed()
	// {
	// 	if (const std::optional<std::wstring> fullPath = select_file(m_ownerWnd))
	// 	{
	// 		launchProcess(fullPath.value());
	// 	}
	// }
}
