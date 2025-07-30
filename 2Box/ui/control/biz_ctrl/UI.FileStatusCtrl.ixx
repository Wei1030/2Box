export module UI.FileStatusCtrl;

import std;
import "sys_defs.h";
import StateMachine;
import WinHttp;
import Coroutine;
import UI.Core;
import UI.LoadingIndicator;

namespace ui
{
	export class FileStatusCtrl;

	namespace fsc_detail
	{
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
	}

	class FileStatusCtrl final : public ControlBase
	{
		using EPainterType = fsc_detail::EPainterType;
		using PainterContext = fsc_detail::PainterContext;

	public:
		explicit FileStatusCtrl(WindowBase* owner)
			: ControlBase(owner)
		{
			initialize();
		}

		explicit FileStatusCtrl(ControlBase* parent)
			: ControlBase(parent)
		{
			initialize();
		}

		virtual ~FileStatusCtrl()
		{
			stopAnaTask();
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

		void startAnaTask();
		void stopAnaTask();
		coro::LazyTask<void> joinAsync();

		bool isFileVerified() const
		{
			return m_painter.currentStateIndex() == EPainterType::Verified;
		}

		void cancelTask()
		{
			m_stopSource.request_stop();
		}

		bool isCancelled() const
		{
			return m_stopSource.stop_requested();
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

		float preferredHeight() const noexcept
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

	public:
		virtual void onDiscardDeviceResources() override
		{
			m_pLoadingIndicator->onDiscardDeviceResources();
		}

	private:
		void initialize()
		{
			m_pLoadingIndicator = std::make_unique<LoadingIndicator>(this);
			m_painter.transferTo<EPainterType::Initial>();
		}

		template <typename PainterEnumType, PainterEnumType>
		friend class fsc_detail::TPainterType;

		virtual HResult createDeviceResourcesImpl(ID2D1HwndRenderTarget* renderTarget) override
		{
			return m_pLoadingIndicator->onCreateDeviceResources(renderTarget);
		}

		virtual void drawImpl(const RenderContext& renderCtx) override;
		void drawBaseContent(const RenderContext& renderCtx) const;
		void drawStatusContent(const RenderContext& renderCtx, const D2D1_COLOR_F& color, std::wstring_view statusText) const;
		void drawTipsContent(const RenderContext& renderCtx, const D2D1_COLOR_F& color, std::wstring_view tipsText) const;
		void drawLoadingContent(const RenderContext& renderCtx) const;
		void drawLoadingIndicator(const RenderContext& renderCtx, float progress) const;
		void drawErrorRoundedArea(const RenderContext& renderCtx) const;
		void drawErrorContent(const RenderContext& renderCtx) const;
		void drawInfoRoundedArea(const RenderContext& renderCtx) const;
		void drawInfoContent(const RenderContext& renderCtx) const;

		///////////////////////////////////////////////////////////
		void drawInitial(const RenderContext& renderCtx) const;
		void drawVerified(const RenderContext& renderCtx) const;
		void drawDownloading(const RenderContext& renderCtx) const;
		void drawWriting(const RenderContext& renderCtx) const;
		void drawError(const RenderContext& renderCtx) const;

	private:
		coro::LazyTask<void> updateTotalSizeInMainThread(std::uint64_t total);
		coro::LazyTask<void> updateCurrentSizeInMainThread(std::uint64_t size);
		coro::LazyTask<void> startAnaTaskImpl();

	private:
		std::wstring m_fileName;
		std::wstring m_downloadServerName;
		std::wstring m_downloadObjName;
		std::wstring m_pdbPath;
		std::wstring m_errorMsg;
		float m_progress{0.f};
		std::unique_ptr<LoadingIndicator> m_pLoadingIndicator{nullptr};
		sm::StateMachine<fsc_detail::TPainterType, EPainterType, PainterContext> m_painter;

	private:
		std::shared_ptr<ms::WinHttpConnection> m_connection;
		std::uint64_t m_totalLength{0};
		std::uint64_t m_currentSize{0};
		coro::AsyncScope m_asyncScope;
		std::stop_source m_stopSource{std::nostopstate};
		coro::SharedTask<void> m_task{coro::SharedTask<void>::reject("task not started")};
	};

	namespace fsc_detail
	{
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
}
