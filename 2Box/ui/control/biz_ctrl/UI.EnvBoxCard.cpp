module UI.EnvBoxCard;

import MainApp;

namespace
{
	constexpr float PADDING = 16.f;
	constexpr float LINE1_GAP = 8.f;
	constexpr float LINE2_GAP = 12.f;
	constexpr float TITLE_HEIGHT = 24.f;
	constexpr float COUNT_HEIGHT = 21.f;
	constexpr float START_BTN_WIDTH = 78.f;
	constexpr float START_BTN_HEIGHT = 33.f;
}

namespace ui
{
	EnvBoxCard::~EnvBoxCard()
	{
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

		m_env->setProcCountChangeNotify([this](std::size_t count)
		{
			m_asyncScope.spawn(onProcessCountChange(count));
		});
	}

	void EnvBoxCard::initialize()
	{
		m_btnStart = std::make_unique<Button>(this);
		m_btnStart->setText(L"添加进程");
		m_btnStart->setBackgroundColor(D2D1::ColorF(0xe3f2fd));
		m_btnStart->setBackgroundColor(D2D1::ColorF(0x000000, 0.f), Button::EState::Normal);
		m_btnStart->setBorderColor(D2D1::ColorF(0x0078d4));
		m_btnStart->setTextColor(D2D1::ColorF(0x0078d4));
		m_btnStart->setOnClick([this] { onBtnStartPressed(); });
	}

	void EnvBoxCard::onResize(float width, float height)
	{
		constexpr float btnYPos = PADDING + TITLE_HEIGHT + LINE1_GAP + COUNT_HEIGHT + LINE2_GAP;
		m_btnStart->setBounds(D2D1::RectF(PADDING, btnYPos, PADDING + START_BTN_WIDTH, btnYPos + START_BTN_HEIGHT));
	}

	void EnvBoxCard::onMouseEnter(const MouseEvent& e)
	{
		m_isHovered = true;
		updateWholeWnd();
	}

	void EnvBoxCard::onMouseLeave(const MouseEvent& e)
	{
		// 鼠标有可能还在子控件中
		if (!hitTest(e.point))
		{
			m_isHovered = false;
			updateWholeWnd();
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
		if (m_isHovered || m_isPressed)
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

		solidBrush->SetColor(D2D1::ColorF(0x333333));
		renderTarget->DrawTextW(m_name.c_str(),
		                        static_cast<UINT32>(m_name.length()),
		                        app().textFormat().pTitleFormat,
		                        D2D1::RectF(PADDING, PADDING,
		                                    drawSize.width - PADDING, PADDING + TITLE_HEIGHT),
		                        solidBrush);

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

		m_btnStart->draw(renderCtx);
	}

	coro::LazyTask<void> EnvBoxCard::onProcessCountChange(std::size_t count)
	{
		// 转到主线程
		co_await sched::transfer_to(app().get_scheduler());
		m_strProcCount = std::format(L"{}", count);
		update();
	}

	void EnvBoxCard::onBtnStartPressed()
	{
	}
}
