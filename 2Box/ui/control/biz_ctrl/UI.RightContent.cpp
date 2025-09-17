module UI.RightContent;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;

namespace
{
	constexpr float PADDING = 24.f;
	constexpr float MARGIN = 24.f;
	constexpr float FEATURES_AREA_HEIGHT = 70.f;
	constexpr std::wstring_view IMPORTANT_NOTICE{
		L"用户须知：\n"
		L"本软件为免费软件，仅供个人非商业目的使用。\n"
		L"未经许可，任何单位或个人不得将其用于商业用途，否则须承担相应法律责任。\n"
		L"用户应确保使用行为符合所在国家法律法规，任何违法违规使用所产生的一切后果由用户自行承担。"
	};
	constexpr std::wstring_view MAIN_TIPS{
		L"使用事项说明：\n"
		L"1、点击 [启动进程] 按钮，选择需要启动的程序后，本软件会自动为其\n新建或挑选合适的环境运行\n\n"
		L"2、.exe后缀的可执行文件会自动挑选一个没有重名进程运行的环境运行\n\n"
		L"3、.lnk快捷方式或.url等其他关联了可执行程序的后缀只会挑选空环境\n或新建环境运行"
		L"（如有需要，用户可自行选择一个环境运行此类文件)\n\n"
		L"4、环境中暂时只能查看所有正在运行的进程。但实际会做的更多，比如\n将其中进程请求读写的一部分路径重定向，包括常见的配置、存档目录"
	};
}

namespace ui
{
	void RightContent::initialize()
	{
		HRESULT hr = app().dWriteFactory()->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_SEMI_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			14.f,
			L"",
			&m_importantNoticeFormat);
		if (FAILED(hr))
		{
			throw std::runtime_error(std::format("CreateTextFormat fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)));
		}
		m_importantNoticeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	void RightContent::onResize(float width, float height)
	{
		m_featuresArea.setBounds(D2D1::RectF(PADDING, PADDING, width - PADDING, PADDING + FEATURES_AREA_HEIGHT));
		m_envDetail.setBounds(D2D1::RectF(PADDING, PADDING + FEATURES_AREA_HEIGHT + MARGIN, width - PADDING, height - PADDING));
	}

	void RightContent::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		solidBrush->SetColor(D2D1::ColorF(0xf8f9fa));
		renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height), solidBrush);

		m_featuresArea.draw(renderCtx);

		if (m_envDetail.hasDetail())
		{
			m_envDetail.draw(renderCtx);
		}
		else
		{
			solidBrush->SetColor(D2D1::ColorF(0));
			renderTarget->DrawTextW(IMPORTANT_NOTICE.data(),
			                        static_cast<UINT>(IMPORTANT_NOTICE.size()),
			                        m_importantNoticeFormat,
			                        D2D1::RectF(PADDING, PADDING + FEATURES_AREA_HEIGHT + PADDING, drawSize.width - PADDING, drawSize.height - PADDING), solidBrush);

			DWRITE_TEXT_ALIGNMENT oldAlignment = app().textFormat().pMainFormat->GetTextAlignment();
			app().textFormat().pMainFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

			constexpr float tipsStartYPos = PADDING + FEATURES_AREA_HEIGHT + PADDING + 150.f;
			renderTarget->DrawTextW(MAIN_TIPS.data(),
			                        static_cast<UINT>(MAIN_TIPS.size()),
			                        app().textFormat().pMainFormat,
			                        D2D1::RectF(PADDING, tipsStartYPos, drawSize.width - PADDING, drawSize.height - PADDING), solidBrush);

			app().textFormat().pMainFormat->SetTextAlignment(oldAlignment);
		}
	}
}
