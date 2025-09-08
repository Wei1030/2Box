module UI.RightContent;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

namespace
{
	constexpr float PADDING = 24.f;
	constexpr float MARGIN = 24.f;
	constexpr float FEATURES_AREA_HEIGHT = 70.f;
}

namespace ui
{
	void RightContent::showEnvInfo(const std::shared_ptr<biz::Env>& env)
	{
		m_env = env;
		std::vector<std::shared_ptr<biz::ProcessInfo>> allProc = env->getAllProcesses();
		for (const std::shared_ptr<biz::ProcessInfo>& proc : allProc)
		{
			m_processes.insert(std::make_pair(proc->getProcessId(), proc));
		}

		update();
	}

	void RightContent::hideEnvInfo()
	{
		m_env.reset();
		m_processes.clear();

		update();
	}

	void RightContent::procCountChange(biz::Env::EProcEvent e, const std::shared_ptr<biz::ProcessInfo>& proc)
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

	void RightContent::onResize(float width, float height)
	{
		m_featuresArea.setBounds(D2D1::RectF(PADDING, PADDING, width - PADDING, PADDING + FEATURES_AREA_HEIGHT));
		m_processList.setBounds(D2D1::RectF(PADDING, PADDING + FEATURES_AREA_HEIGHT + MARGIN, width - PADDING, height - PADDING));
	}

	void RightContent::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		solidBrush->SetColor(D2D1::ColorF(0xf8f9fa));
		renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height), solidBrush);

		m_featuresArea.draw(renderCtx);

		if (m_env)
		{
			m_processList.draw(renderCtx);
		}
	}
}
