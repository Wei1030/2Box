module UI.LeftSidebar;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import Scheduler;

namespace
{
	constexpr float PADDING = 24.f;
	constexpr float MARGIN_BOTTOM = 24.f;
	constexpr float CARD_HEIGHT = 132.f;
	constexpr float CARD_MARGIN_BOTTOM = 12.f;
}

namespace ui
{
	LeftSidebar::~LeftSidebar()
	{
		// 不再接收通知，且会等待已经通知的回调结束
		biz::env_mgr().setEnvChangeNotify(nullptr);
		// 之后就绝对不会spawn新的协程，才可以安全等待所有协程结束
		m_asyncScope.join();
	}

	void LeftSidebar::initialize()
	{
		m_startAppDiv = std::make_unique<StartAppDiv>(this);
		m_startAppDiv->setBounds(D2D1::RectF(PADDING, PADDING, PADDING + 232.f, PADDING + 36.f));

		initializeAllEnvBoxCard();
	}

	void LeftSidebar::initializeAllEnvBoxCard()
	{
		std::vector<std::shared_ptr<biz::Env>> allEnv = biz::env_mgr().getAllEnv();
		for (auto it = allEnv.begin(); it != allEnv.end(); ++it)
		{
			std::unique_ptr<EnvBoxCard> card = std::make_unique<EnvBoxCard>(this);
			card->setEnv(*it);
			m_envs.insert(std::make_pair((*it)->getIndex(), std::move(card)));
		}

		biz::env_mgr().setEnvChangeNotify([this](biz::EnvManager::EChangeType changeType, const std::shared_ptr<biz::Env>& env)
		{
			m_asyncScope.spawn(onEnvCountChange(changeType, env));
		});
	}

	coro::LazyTask<void> LeftSidebar::onEnvCountChange(biz::EnvManager::EChangeType changeType, std::shared_ptr<biz::Env> env)
	{
		// 转到主线程
		co_await sched::transfer_to(app().get_scheduler());

		if (changeType == biz::EnvManager::EChangeType::Create)
		{
			std::unique_ptr<EnvBoxCard> card = std::make_unique<EnvBoxCard>(this);
			card->setEnv(env);
			m_envs.insert(std::make_pair(env->getIndex(), std::move(card)));
		}
		else if (changeType == biz::EnvManager::EChangeType::Delete)
		{
			m_envs.erase(env->getIndex());
		}

		update();
	}

	void LeftSidebar::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		solidBrush->SetColor(D2D1::ColorF(0xFFFFFF));
		renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height), solidBrush);

		solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
		renderTarget->DrawLine(D2D1::Point2F(drawSize.width, 0.f), D2D1::Point2F(drawSize.width, drawSize.height), solidBrush);

		m_startAppDiv->draw(renderCtx);

		if (m_envs.size())
		{
			float startYPos = m_startAppDiv->getBounds().bottom + MARGIN_BOTTOM;
			solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
			renderTarget->DrawLine(D2D1::Point2F(PADDING, startYPos), D2D1::Point2F(drawSize.width, startYPos), solidBrush);

			startYPos += MARGIN_BOTTOM;

			for (auto it = m_envs.begin(); it != m_envs.end(); ++it)
			{
				EnvBoxCard* card = it->second.get();
				card->setBounds(D2D1::RectF(PADDING, startYPos, drawSize.width - PADDING, startYPos + CARD_HEIGHT));
				if (card->isHovered())
				{
					draw_box_shadow(renderCtx, card->getBounds(),
					                {
						                .offset = D2D1::Point2F(0.f, 4.f),
						                .size = 6.f,
						                .layers = 6,
						                .color = D2D1::ColorF{0x000000, 0.03f},
						                .radius = 12.f
					                });
				}
				else
				{
					draw_box_shadow(renderCtx, card->getBounds(), {.offset = D2D1::Point2F(0.f, 1.f), .radius = 12.f});
				}
				card->draw(renderCtx);

				startYPos += CARD_HEIGHT + CARD_MARGIN_BOTTOM;
			}
		}
	}
}
