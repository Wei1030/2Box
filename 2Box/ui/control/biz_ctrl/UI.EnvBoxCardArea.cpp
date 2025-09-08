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
	constexpr float CARD_HEIGHT = 97.f; //132.f;
	constexpr float CARD_MARGIN_BOTTOM = 12.f;
	constexpr float WHEEL_SCROLL_SIZE = 24.f;
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

	void EnvBoxCardArea::onResize(float width, float height)
	{
		m_scrollBar->setVisibleSize(height);
		m_scrollBar->setBounds(D2D1::RectF(width - shadowSize - scrollWidth, 0.f,
		                                   width - shadowSize, height));
	}

	void EnvBoxCardArea::onMouseEnter(const MouseEvent& e)
	{
		m_isHovered = true;
		update();
	}

	void EnvBoxCardArea::onMouseLeave(const MouseEvent& e)
	{
		// 1.进入子控件会触发leave
		// 2.离开子控件也会触发leave(除非子控件拦截)
		if (!hitTest(e.point))
		{
			m_isHovered = false;
			update();
		}
		e.accept = true;
	}

	void EnvBoxCardArea::onMouseWheel(const MouseWheelEvent& e)
	{
		e.accept = true;

		const float wheelCount = e.zDelta / 120.f;
		m_scrollBar->scroll(-wheelCount * WHEEL_SCROLL_SIZE);
	}

	void EnvBoxCardArea::initialize()
	{
		biz::env_mgr().setEnvChangeNotify([this](biz::EnvManager::EChangeType changeType, const std::shared_ptr<biz::Env>& env)
		{
			m_asyncScope.spawn(onEnvCountChange(changeType, env));
		});
		std::vector<std::shared_ptr<biz::Env>> allEnv = biz::env_mgr().getAllEnv();
		for (auto it = allEnv.begin(); it != allEnv.end(); ++it)
		{
			addEnv(*it, true);
		}

		m_scrollBar = std::make_unique<ScrollBar>(this);
		m_scrollBar->setThumbPosChangeNotify([this] { updateAllEnvPos(); });
		m_scrollBar->setTotalSize(m_envs.size() * (CARD_HEIGHT + CARD_MARGIN_BOTTOM));
	}

	coro::LazyTask<void> EnvBoxCardArea::onEnvCountChange(biz::EnvManager::EChangeType changeType, std::shared_ptr<biz::Env> env)
	{
		// 转到主线程
		co_await sched::transfer_to(app().get_scheduler());

		if (changeType == biz::EnvManager::EChangeType::Create)
		{
			addEnv(env);
		}
		else if (changeType == biz::EnvManager::EChangeType::Delete)
		{
			removeEnv(env->getIndex());
		}

		m_scrollBar->setTotalSize(m_envs.size() * (CARD_HEIGHT + CARD_MARGIN_BOTTOM));
	}

	void EnvBoxCardArea::addEnv(const std::shared_ptr<biz::Env>& env, bool initialIdle /*= false*/)
	{
		if (m_envs.contains(env->getIndex()))
		{
			return;
		}
		std::unique_ptr<EnvBoxCard> card = std::make_unique<EnvBoxCard>(initialIdle, this);
		card->setEnv(env);
		card->setOnSelect([this, rawPtr = card.get()](bool b) { onEnvSelected(rawPtr, b); });
		m_envs.insert(std::make_pair(env->getIndex(), std::move(card)));
	}

	void EnvBoxCardArea::removeEnv(std::uint32_t envIndex)
	{
		if (const auto it = m_envs.find(envIndex); it != m_envs.end())
		{
			onEnvSelected(it->second.get(), false);
			m_envs.erase(it);
		}
	}

	void EnvBoxCardArea::onEnvSelected(EnvBoxCard* card, bool bSelected)
	{
		if (bSelected)
		{
			if (m_currentSelectedEnv)
			{
				m_currentSelectedEnv->programmaticDeselect();
				m_currentSelectedEnv->setOnProcCountChange(nullptr);
			}
			m_currentSelectedEnv = card;
			m_currentSelectedEnv->setOnProcCountChange(m_pfnOnProcCountChange);
			if (m_pfnOnSelect)
			{
				m_pfnOnSelect(card->getEnv(), true);
			}
		}
		else
		{
			if (m_currentSelectedEnv == card)
			{
				m_currentSelectedEnv = nullptr;

				card->setOnProcCountChange(nullptr);
				if (m_pfnOnSelect)
				{
					m_pfnOnSelect(card->getEnv(), false);
				}
			}
		}

		update();
	}

	void EnvBoxCardArea::updateAllEnvPos()
	{
		if (m_envs.empty())
		{
			m_envsToDraw.clear();
			updateWholeWnd();
			return;
		}

		constexpr float itemHeight = CARD_HEIGHT + CARD_MARGIN_BOTTOM;
		const float scrollOffset = m_scrollBar->getScrollOffset();
		const std::uint32_t startIndex = static_cast<std::uint32_t>(scrollOffset / itemHeight);
		const std::uint32_t endIndex = std::min(
			static_cast<std::uint32_t>((scrollOffset + m_scrollBar->getVisibleSize()) / itemHeight),
			static_cast<std::uint32_t>(m_envs.size() - 1));
		m_envsToDraw.clear();

		if (startIndex > endIndex)
		{
			updateWholeWnd();
			return;
		}

		m_envsToDraw.reserve(endIndex - startIndex);

		const std::uint32_t startDrawOffsetY = static_cast<std::uint32_t>(scrollOffset) % static_cast<std::uint32_t>(itemHeight);
		float startYPos = shadowSize - shadowOffsetY - startDrawOffsetY;
		const auto drawSize = size();
		std::size_t index = 0;
		for (auto it = m_envs.begin(); it != m_envs.end(); ++it, ++index)
		{
			if (index < startIndex || index > endIndex)
			{
				it->second->setBounds({});
				continue;
			}
			EnvBoxCard* card = it->second.get();
			card->setBounds(D2D1::RectF(shadowSize, startYPos,
			                            drawSize.width - shadowSize - scrollAreaWidth, startYPos + CARD_HEIGHT));
			m_envsToDraw.push_back(card);
			startYPos += itemHeight;
		}
		updateWholeWnd();
	}

	void EnvBoxCardArea::drawImpl(const RenderContext& renderCtx)
	{
		if (m_envsToDraw.empty())
		{
			return;
		}
		for (auto it = m_envsToDraw.begin(); it != m_envsToDraw.end(); ++it)
		{
			EnvBoxCard* card = *it;
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
		}

		if (m_isHovered || m_scrollBar->isThumbPressed())
		{
			m_scrollBar->draw(renderCtx);
		}
	}
}
