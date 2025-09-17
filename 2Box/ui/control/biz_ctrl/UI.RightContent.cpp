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

	constexpr float TIPS_TITLE_START_Y_POS = PADDING + FEATURES_AREA_HEIGHT + PADDING + 150.f;
	constexpr float TIPS_START_Y_POS = TIPS_TITLE_START_Y_POS + 25.f;

	constexpr std::wstring_view MAIN_TIPS{
		L"1、点击【启动进程】按钮，选择需要启动的程序后，或【拖动文件】至此窗口，本软件会自动为其新建或选择合适的环境运行\n\n"
		L"2、.exe后缀的可执行文件会自动选择一个没有重名进程运行的环境运行\n\n"
		L"3、.lnk快捷方式或.url等其他关联了可执行程序的后缀只会选择空环境或新建环境运行（如有需要，用户可自行选择一个环境运行此类文件)\n\n"
		L"4、本软件只能简单的在环境之间隔离，不会阻止环境内的进程访问环境外的资源，也不会阻止环境外的进程感知环境内的进程\n\n"
		L"5、环境中暂时只能查看所有正在运行的进程。但实际会做的更多，比如将其中进程请求读写的一部分路径重定向，包括常见的配置、存档目录\n\n"
		L"6、某些进程的窗口无法任意调整大小，此类情况下的【平铺窗口】功能的效果可能不会很好\n\n"
		L"7、【同步输入】只能同步顶层窗口之间的输入，一般不支持主窗口内包含很多子窗口或子控件的软件"
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

		hr = app().dWriteFactory()->CreateTextLayout(MAIN_TIPS.data(),
		                                             static_cast<UINT32>(MAIN_TIPS.size()),
		                                             app().textFormat().pMainFormat,
		                                             std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
		                                             &m_mainTipsLayout);
		if (FAILED(hr))
		{
			throw std::runtime_error(std::format("CreateTextLayout fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)));
		}
		m_mainTipsLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_CHARACTER);
	}

	void RightContent::onResize(float width, float height)
	{
		m_featuresArea.setBounds(D2D1::RectF(PADDING, PADDING, width - PADDING, PADDING + FEATURES_AREA_HEIGHT));
		m_envDetail.setBounds(D2D1::RectF(PADDING, PADDING + FEATURES_AREA_HEIGHT + MARGIN, width - PADDING, height - PADDING));

		m_mainTipsLayout->SetMaxWidth(width - PADDING * 2);
		m_mainTipsLayout->SetMaxHeight(height - TIPS_START_Y_POS - PADDING);

		DWRITE_TEXT_METRICS textMetrics;
		m_mainTipsWidth = width - PADDING * 2;
		if (SUCCEEDED(m_mainTipsLayout->GetMetrics(&textMetrics)))
		{
			m_mainTipsWidth = textMetrics.width;
		}
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

			constexpr std::wstring_view tipsTitle{L"使用事项:"};
			renderTarget->DrawTextW(tipsTitle.data(),
			                        static_cast<UINT>(tipsTitle.size()),
			                        app().textFormat().pMainFormat,
			                        D2D1::RectF(PADDING, TIPS_TITLE_START_Y_POS, drawSize.width - PADDING, drawSize.height - PADDING), solidBrush);
			app().textFormat().pMainFormat->SetTextAlignment(oldAlignment);

			const float totalWidth = drawSize.width - PADDING * 2;
			renderTarget->DrawTextLayout(D2D1::Point2F(PADDING + (totalWidth - m_mainTipsWidth) / 2.f, TIPS_START_Y_POS), m_mainTipsLayout, solidBrush);
		}
	}
}
