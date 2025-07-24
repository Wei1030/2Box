export module UI.FileStatusCtrl;

import std;
import "sys_defs.h";
import MainApp;
import StateMachine;
import UI.ControlBase;
import UI.LoadingIndicator;

namespace ui
{
	export class FileStatusCtrl;

	enum class EPainterType : std::uint8_t
	{
		Initial,
		Verified,
		Downloading,
		Writing,
		Error,
		TotalCount
	};

	template <typename PainterEnumType, PainterEnumType>
	class TPainterType
	{
	};

	struct PainterContext
	{
		FileStatusCtrl& ctrl;
		const RenderContext& renderCtx;
	};

	// ReSharper disable CppMemberFunctionMayBeStatic
	// ReSharper disable CppInconsistentNaming

	class PainterBase
	{
	public:
		void OnEnter(PainterContext&)
		{
		}

		void OnExit(PainterContext&)
		{
		}
	};

	template <>
	class TPainterType<EPainterType, EPainterType::Initial> : public PainterBase
	{
	public:
		inline sm::TNextState<EPainterType> OnUpdate(PainterContext& ctx);
	};

	template <>
	class TPainterType<EPainterType, EPainterType::Verified> : public PainterBase
	{
	public:
		inline sm::TNextState<EPainterType> OnUpdate(PainterContext& ctx);
	};

	template <>
	class TPainterType<EPainterType, EPainterType::Downloading> : public PainterBase
	{
	public:
		inline sm::TNextState<EPainterType> OnUpdate(PainterContext& ctx);
	};

	template <>
	class TPainterType<EPainterType, EPainterType::Writing> : public PainterBase
	{
	public:
		inline sm::TNextState<EPainterType> OnUpdate(PainterContext& ctx);
	};

	template <>
	class TPainterType<EPainterType, EPainterType::Error> : public PainterBase
	{
	public:
		inline sm::TNextState<EPainterType> OnUpdate(PainterContext& ctx);
	};

	// ReSharper restore CppParameterMayBeConstPtrOrRef
	// ReSharper restore CppInconsistentNaming

	class FileStatusCtrl final : public ControlTmplBase<FileStatusCtrl>
	{
	public:
		explicit FileStatusCtrl(WindowBase* owner)
			: ControlTmplBase(owner)
		{
			initialize();
		}

		explicit FileStatusCtrl(ControlBase* parent)
			: ControlTmplBase(parent)
		{
			initialize();
		}

		void setFileName(std::wstring_view fileName)
		{
			m_fileName = fileName;
		}

		void setDownloadServerName(std::wstring_view downloadServerName)
		{
			m_downloadServerName = downloadServerName;
		}

		void setDownloadObjName(std::wstring_view downloadObjName)
		{
			m_downloadObjName = downloadObjName;
		}

		void setPdbPath(std::wstring_view pdbPath)
		{
			m_pdbPath = pdbPath;
		}

		void setProgress(float progress)
		{
			m_progress = progress;
		}

		static constexpr float padding = 15.f;
		static constexpr float margin = 8.f;
		static constexpr float titleFileNameHeight = 20.f;
		static constexpr float statusTextHeight = 18.f;
		static constexpr float tipsTextHeight = 16.f;
		static constexpr float loadingIndicatorHeight = 8.f;
		static constexpr float subAreaPadding = 10.f;
		static constexpr float errorAreaHeight = subAreaPadding * 2.f + statusTextHeight * 4.f + margin * 3.f;
		static constexpr float infoAreaHeight = subAreaPadding * 2.f + tipsTextHeight * 3.f + margin * 2.f;
		static constexpr float infoLabelWidth = 60.f;

		float preferredHeight() const
		{
			switch (m_painter.currentStateIndex())
			{
			case EPainterType::Initial:
			case EPainterType::Verified:
				return padding * 2 + titleFileNameHeight + statusTextHeight + margin;
			case EPainterType::Downloading:
			case EPainterType::Writing:
				return padding * 2 + titleFileNameHeight + statusTextHeight + tipsTextHeight + loadingIndicatorHeight + margin * 3;
			case EPainterType::Error:
				return padding * 2 + titleFileNameHeight + statusTextHeight + errorAreaHeight + infoAreaHeight + margin * 3;
			default:
				return 0.f;
			}
		}

	private:
		void initialize()
		{
			m_pLoadingIndicator = std::make_unique<LoadingIndicator>(this);
			m_painter.transferTo<EPainterType::Initial>();
		}

		friend ControlTmplBase;
		template <typename PainterEnumType, PainterEnumType>
		friend class TPainterType;

		WindowBase::HResult createDeviceResourcesImpl(ID2D1HwndRenderTarget* renderTarget) const
		{
			return m_pLoadingIndicator->onCreateDeviceResources(renderTarget);
		}

		void discardDeviceResourcesImpl() const
		{
			m_pLoadingIndicator->onDiscardDeviceResources();
		}

		void drawImpl(const RenderContext& renderCtx)
		{
			PainterContext ctx{*this, renderCtx};
			m_painter.setCtx(&ctx);
			m_painter.update();
		}

		void drawBaseContent(const RenderContext& renderCtx) const
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			const auto drawSize = size();
			const D2D1_ROUNDED_RECT round = D2D1::RoundedRect(
				D2D1::RectF(0, 0, drawSize.width, drawSize.height),
				4.f, 4.f);
			solidBrush->SetColor(D2D1::ColorF(0xE6E6E6));
			renderTarget->DrawRoundedRectangle(round, solidBrush);
			solidBrush->SetColor(D2D1::ColorF(0xFDFDFD));
			renderTarget->FillRoundedRectangle(round, solidBrush);

			const std::wstring fileNameText = std::format(L"文件名: {}", m_fileName);
			solidBrush->SetColor(D2D1::ColorF(0x333333));
			renderTarget->DrawTextW(fileNameText.c_str(),
			                        static_cast<UINT32>(fileNameText.length()),
			                        app().textFormat().pTitleFormat,
			                        D2D1::RectF(padding, padding, drawSize.width - padding, padding + titleFileNameHeight),
			                        solidBrush);
		}

		void drawStatusContent(const RenderContext& renderCtx, const D2D1_COLOR_F& color, std::wstring_view statusText) const
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			constexpr float yPos = padding + titleFileNameHeight + margin;
			solidBrush->SetColor(color);
			renderTarget->DrawTextW(statusText.data(),
			                        static_cast<UINT32>(statusText.length()),
			                        app().textFormat().pMainFormat,
			                        D2D1::RectF(padding, yPos, size().width - padding, yPos + statusTextHeight),
			                        solidBrush);
		}

		void drawTipsContent(const RenderContext& renderCtx, const D2D1_COLOR_F& color, std::wstring_view tipsText) const
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + margin * 2;
			solidBrush->SetColor(color);
			renderTarget->DrawTextW(tipsText.data(),
			                        static_cast<UINT32>(tipsText.length()),
			                        app().textFormat().pTipsFormat,
			                        D2D1::RectF(padding, yPos, size().width - padding, yPos + tipsTextHeight),
			                        solidBrush);
		}

		void drawLoadingContent(const RenderContext& renderCtx) const
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + tipsTextHeight + margin * 3;
			const D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
				D2D1::RectF(padding, yPos, size().width - padding, yPos + loadingIndicatorHeight),
				4.f, 4.f);
			solidBrush->SetColor(D2D1::ColorF(0xE0E5EC));
			renderTarget->FillRoundedRectangle(&trackRect, solidBrush);
		}

		void drawLoadingIndicator(const RenderContext& renderCtx, float progress) const
		{
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + tipsTextHeight + margin * 3;
			const float contentWidth = size().width - padding * 2;
			const float filledWidth = progress * contentWidth;
			m_pLoadingIndicator->setBounds(D2D1::RectF(padding, yPos, padding + filledWidth, yPos + loadingIndicatorHeight));
			m_pLoadingIndicator->draw(renderCtx);
		}

		void drawErrorRoundedArea(const RenderContext& renderCtx) const
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + margin * 2;
			D2D1_RECT_F errorBgRect = D2D1::RectF(padding, yPos, size().width - padding, yPos + errorAreaHeight);
			const D2D1_ROUNDED_RECT errorBgRoundedRect = D2D1::RoundedRect(errorBgRect, 4.f, 4.f);
			solidBrush->SetColor(D2D1::ColorF(0xfdf6f6));
			renderTarget->FillRoundedRectangle(&errorBgRoundedRect, solidBrush);

			errorBgRect.right = errorBgRect.left + 3.f;
			renderTarget->PushAxisAlignedClip(errorBgRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			solidBrush->SetColor(D2D1::ColorF(0xd83b01));
			renderTarget->FillRoundedRectangle(&errorBgRoundedRect, solidBrush);
			renderTarget->PopAxisAlignedClip();
		}

		void drawErrorContent(const RenderContext& renderCtx) const
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			const auto drawSize = size();
			const float contentRight = drawSize.width - padding;

			constexpr float xPos = padding + subAreaPadding;
			{
				constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + margin * 2 + subAreaPadding;
				const std::wstring errorMsg = std::format(L"错误描述：{}", m_errorMsg);
				renderTarget->DrawTextW(errorMsg.c_str(),
				                        static_cast<UINT32>(errorMsg.length()),
				                        app().textFormat().pErrorMsgFormat,
				                        D2D1::RectF(xPos, yPos, contentRight, yPos + statusTextHeight),
				                        solidBrush);
			}
			{
				constexpr float yPos = padding + titleFileNameHeight + statusTextHeight * 2 + margin * 3 + subAreaPadding;
				constexpr std::wstring_view message = L"您可以尝试以下解决方案：";
				renderTarget->DrawTextW(message.data(),
				                        static_cast<UINT32>(message.length()),
				                        app().textFormat().pErrorMsgFormat,
				                        D2D1::RectF(xPos, yPos, contentRight, yPos + statusTextHeight),
				                        solidBrush);
			}
			{
				constexpr float yPos = padding + titleFileNameHeight + statusTextHeight * 3 + margin * 4 + subAreaPadding;
				constexpr std::wstring_view message = L"1、重试下载";
				renderTarget->DrawTextW(message.data(),
				                        static_cast<UINT32>(message.length()),
				                        app().textFormat().pErrorMsgFormat,
				                        D2D1::RectF(xPos, yPos, contentRight, yPos + statusTextHeight),
				                        solidBrush);
			}
			{
				constexpr float yPos = padding + titleFileNameHeight + statusTextHeight * 4 + margin * 5 + subAreaPadding;
				constexpr std::wstring_view message = L"2、手动下载文件并放置到指定目录";
				renderTarget->DrawTextW(message.data(),
				                        static_cast<UINT32>(message.length()),
				                        app().textFormat().pErrorMsgFormat,
				                        D2D1::RectF(xPos, yPos, contentRight, yPos + statusTextHeight),
				                        solidBrush);
			}
		}

		void drawInfoRoundedArea(const RenderContext& renderCtx) const
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + errorAreaHeight + margin * 3;
			const D2D1_ROUNDED_RECT infoRoundedRect = D2D1::RoundedRect(
				D2D1::RectF(padding, yPos, size().width - padding, yPos + infoAreaHeight),
				4.f, 4.f);
			solidBrush->SetColor(D2D1::ColorF(0xf3f2f1));
			renderTarget->FillRoundedRectangle(&infoRoundedRect, solidBrush);
		}

		void drawInfoContent(const RenderContext& renderCtx) const
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			const auto drawSize = size();
			const float contentRight = drawSize.width - padding;
			constexpr float xPos = padding + subAreaPadding;
			constexpr float labelRight = xPos + infoLabelWidth;

			{
				constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + errorAreaHeight + margin * 3 + subAreaPadding;
				constexpr std::wstring_view label = L"下载URL：";
				solidBrush->SetColor(D2D1::ColorF(0x605e5c));
				renderTarget->DrawTextW(label.data(),
				                        static_cast<UINT32>(label.length()),
				                        app().textFormat().pTipsFormat,
				                        D2D1::RectF(xPos, yPos, labelRight, yPos + tipsTextHeight),
				                        solidBrush);
				const std::wstring value = std::format(L"https://{}/{}", m_downloadServerName, m_downloadObjName);
				solidBrush->SetColor(D2D1::ColorF(0x333333));
				renderTarget->DrawTextW(value.c_str(),
				                        static_cast<UINT32>(value.length()),
				                        app().textFormat().pTipsFormat,
				                        D2D1::RectF(labelRight, yPos, contentRight, yPos + tipsTextHeight),
				                        solidBrush);
			}
			{
				constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + errorAreaHeight + margin * 4 + subAreaPadding + tipsTextHeight;
				constexpr std::wstring_view label = L"目标目录：";
				solidBrush->SetColor(D2D1::ColorF(0x605e5c));
				renderTarget->DrawTextW(label.data(),
				                        static_cast<UINT32>(label.length()),
				                        app().textFormat().pTipsFormat,
				                        D2D1::RectF(xPos, yPos, labelRight, yPos + tipsTextHeight),
				                        solidBrush);
				solidBrush->SetColor(D2D1::ColorF(0x333333));
				renderTarget->DrawTextW(m_pdbPath.c_str(),
				                        static_cast<UINT32>(m_pdbPath.length()),
				                        app().textFormat().pTipsFormat,
				                        D2D1::RectF(labelRight, yPos, contentRight, yPos + tipsTextHeight),
				                        solidBrush);
			}
			{
				constexpr float yPos = padding + titleFileNameHeight + statusTextHeight + errorAreaHeight + margin * 5 + subAreaPadding + tipsTextHeight * 2;
				constexpr std::wstring_view label = L"文件名：";
				solidBrush->SetColor(D2D1::ColorF(0x605e5c));
				renderTarget->DrawTextW(label.data(),
				                        static_cast<UINT32>(label.length()),
				                        app().textFormat().pTipsFormat,
				                        D2D1::RectF(xPos, yPos, labelRight, yPos + tipsTextHeight),
				                        solidBrush);
				solidBrush->SetColor(D2D1::ColorF(0x333333));
				renderTarget->DrawTextW(m_fileName.c_str(),
				                        static_cast<UINT32>(m_fileName.length()),
				                        app().textFormat().pTipsFormat,
				                        D2D1::RectF(labelRight, yPos, contentRight, yPos + tipsTextHeight),
				                        solidBrush);
			}
		}

		void drawInitial(const RenderContext& renderCtx) const
		{
			drawBaseContent(renderCtx);
			drawStatusContent(renderCtx, D2D1::ColorF(0xe3a21a), L"验证文件中...");
		}

		void drawVerified(const RenderContext& renderCtx) const
		{
			drawBaseContent(renderCtx);
			drawStatusContent(renderCtx, D2D1::ColorF(0x107c10), L"文件已存在，验证通过");
		}

		void drawDownloading(const RenderContext& renderCtx) const
		{
			drawBaseContent(renderCtx);
			drawStatusContent(renderCtx, D2D1::ColorF(0x0078d7), std::format(L"正在下载: {:.0f}%", m_progress * 100.f));
			drawTipsContent(renderCtx, D2D1::ColorF(0x605e5c), std::format(L"下载源: https://{}/{}", m_downloadServerName, m_downloadObjName));
			drawLoadingContent(renderCtx);
			drawLoadingIndicator(renderCtx, m_progress);
		}

		void drawWriting(const RenderContext& renderCtx) const
		{
			drawBaseContent(renderCtx);
			drawStatusContent(renderCtx, D2D1::ColorF(0x0078d7), L"下载完成，正在写入");
			drawTipsContent(renderCtx, D2D1::ColorF(0x605e5c), std::format(L"写入位置: {}", m_pdbPath));
			drawLoadingIndicator(renderCtx, 1.f);
		}

		void drawError(const RenderContext& renderCtx) const
		{
			drawBaseContent(renderCtx);
			drawStatusContent(renderCtx, D2D1::ColorF(0xd83b01), L"发生错误");
			drawErrorRoundedArea(renderCtx);
			drawErrorContent(renderCtx);
			drawInfoRoundedArea(renderCtx);
			drawInfoContent(renderCtx);
		}

	private:
		std::wstring m_fileName;
		std::wstring m_downloadServerName;
		std::wstring m_downloadObjName;
		std::wstring m_pdbPath;
		std::wstring m_errorMsg;
		float m_progress{0.f};
		std::unique_ptr<LoadingIndicator> m_pLoadingIndicator{nullptr};
		sm::StateMachine<TPainterType, EPainterType, PainterContext> m_painter;
	};

	// ReSharper disable CppMemberFunctionMayBeStatic
	// ReSharper disable CppParameterMayBeConstPtrOrRef

	sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Initial>::OnUpdate(PainterContext& ctx)
	{
		ctx.ctrl.drawInitial(ctx.renderCtx);
		return {EPainterType::Initial};
	}

	sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Verified>::OnUpdate(PainterContext& ctx)
	{
		ctx.ctrl.drawVerified(ctx.renderCtx);
		return {EPainterType::Verified};
	}

	sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Downloading>::OnUpdate(PainterContext& ctx)
	{
		ctx.ctrl.drawDownloading(ctx.renderCtx);
		return {EPainterType::Downloading};
	}

	sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Writing>::OnUpdate(PainterContext& ctx)
	{
		ctx.ctrl.drawWriting(ctx.renderCtx);
		return {EPainterType::Writing};
	}

	sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Error>::OnUpdate(PainterContext& ctx)
	{
		ctx.ctrl.drawError(ctx.renderCtx);
		return {EPainterType::Error};
	}

	// ReSharper restore CppParameterMayBeConstPtrOrRef
	// ReSharper restore CppMemberFunctionMayBeStatic
}
