module UI.EnvBoxCardArea;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import Scheduler;

namespace
{
	constexpr float CARD_HEIGHT = 132.f;
	constexpr float CARD_MARGIN_BOTTOM = 12.f;
}

namespace ui
{
	EnvBoxCardArea::~EnvBoxCardArea()
	{
		// 不再接收通知，且会等待已经通知的回调结束
		biz::env_mgr().setEnvChangeNotify(nullptr);
		// 之后就绝对不会spawn新的协程，才可以安全等待所有协程结束
		m_asyncScope.join();
	}

	void EnvBoxCardArea::launchProcess(const std::wstring& procFullPath)
	{
		for (auto it = m_envs.begin(); it != m_envs.end(); ++it)
		{
			EnvBoxCard* box = it->second.get();
			if (!box->isIdle() || box->contains(procFullPath))
			{
				continue;
			}
			box->launchProcess(procFullPath);
			return;
		}
		// 没有合适的env，则创建新的
		std::shared_ptr<biz::Env> env = biz::env_mgr().createEnv();
		// 这里不考虑m_envs了，env的创建回调中会加入m_envs，这里直接使用launcher接口启动进程
		biz::launcher().run(env, procFullPath);
	}

	void EnvBoxCardArea::initialize()
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

	coro::LazyTask<void> EnvBoxCardArea::onEnvCountChange(biz::EnvManager::EChangeType changeType, std::shared_ptr<biz::Env> env)
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

		updateWholeWnd();
	}

	void EnvBoxCardArea::drawImpl(const RenderContext& renderCtx)
	{
		if (m_envs.empty())
		{
			return;
		}
		
		const auto drawSize = size();
		
		float startYPos = shadowSize - shadowOffsetY;
		for (auto it = m_envs.begin(); it != m_envs.end(); ++it)
		{
			EnvBoxCard* card = it->second.get();
			card->setBounds(D2D1::RectF(shadowSize, startYPos, drawSize.width - shadowSize, startYPos + CARD_HEIGHT));
			if (card->isHovered())
			{
				draw_box_shadow(renderCtx, card->getBounds(),
				                {
					                .offset = D2D1::Point2F(0.f, shadowOffsetY),
					                .size = shadowSize,
					                .layers = static_cast<int>(shadowSize),
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
