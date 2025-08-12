export module UI.Page:Home;

import "sys_defs.h";
import std;
import UI.Core;
import UI.LeftSidebar;
import UI.RightContent;
// import UI.Button;
// import PELoader;
// import Injector;
// import EssentialData;

namespace ui
{
	// 	void testFunction()
	// 	{
	// 		PROCESS_INFORMATION ProcInfo = {nullptr};
	// 		STARTUPINFO StartupInfo = {0};
	// 		StartupInfo.cb = sizeof(StartupInfo);
	// 		StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	// 		StartupInfo.wShowWindow = SW_SHOW;
	//
	// #ifdef _WIN64
	// 		std::wstring strCmd = L"03-Singleton.exe";
	// #else
	// 		std::wstring strCmd = L"test\\x86\\ConsoleApplication1.exe";
	// #endif
	// 		if (!CreateProcessW(nullptr, strCmd.data(), nullptr, nullptr, 0,
	// 		                    CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, nullptr, nullptr, &StartupInfo, &ProcInfo))
	// 		{
	// 			return;
	// 		}
	// 		try
	// 		{
	// 			injector::inject_memory_dll_to_process(ProcInfo.dwProcessId, biz::get_random_number(), biz::get_injection_dlls(), biz::get_essential_data());
	// 			ResumeThread(ProcInfo.hThread);
	// 		}
	// 		catch (...)
	// 		{
	// 			TerminateProcess(ProcInfo.hProcess, 0);
	// 		}
	// 		CloseHandle(ProcInfo.hThread);
	// 		CloseHandle(ProcInfo.hProcess);
	// 	}

	export class HomePage final : public RendererInterface
	{
	public:
		static constexpr float sidebarWidth = 280.0f;
		static constexpr float sidebarShadowBlur = 3.f;

		explicit HomePage(WindowBase* owner) : m_ownerWnd(owner)
		{
			m_leftSidebar = std::make_unique<LeftSidebar>(owner);
			m_rightContent = std::make_unique<RightContent>(owner);

			const D2D_RECT_F ownerRc = owner->rect();
			m_leftSidebar->setBounds(D2D1::RectF(0.f, 0.f, sidebarWidth, ownerRc.bottom));
			m_rightContent->setBounds(D2D1::RectF(sidebarWidth, 0, ownerRc.right, ownerRc.bottom));
		}

		void onResize(float width, float height) const
		{
			m_leftSidebar->setBounds(D2D1::RectF(0.f, 0.f, sidebarWidth, height));
			m_rightContent->setBounds(D2D1::RectF(sidebarWidth, 0, width, height));
		}

		virtual HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) override
		{
			const D2D1_GRADIENT_STOP stops[2] = {
				{0.0f, D2D1::ColorF(0x000000, 0.05f)},
				{1.0f, D2D1::ColorF(0x000000, 0.f)}
			};
			UniqueComPtr<ID2D1GradientStopCollection> gradientStops;
			HResult hr = renderTarget->CreateGradientStopCollection(
				stops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&gradientStops);
			if (FAILED(hr))
			{
				return hr;
			}

			hr = renderTarget->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(D2D1::Point2F(sidebarWidth, 0), D2D1::Point2F(sidebarWidth + sidebarShadowBlur, 0)),
				gradientStops,
				&m_pLinearGradientBrush);
			if (FAILED(hr))
			{
				return hr;
			}
			return hr;
		}

		virtual void onDiscardDeviceResources() override
		{
			m_pLinearGradientBrush.reset();
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			m_rightContent->draw(renderCtx);
			m_leftSidebar->draw(renderCtx);

			const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
			renderTarget->FillRectangle(D2D1::RectF(sidebarWidth, 0.f, sidebarWidth + sidebarShadowBlur, m_leftSidebar->size().height), m_pLinearGradientBrush);
		}

	private:
		WindowBase* m_ownerWnd{nullptr};
		UniqueComPtr<ID2D1LinearGradientBrush> m_pLinearGradientBrush;
		std::unique_ptr<LeftSidebar> m_leftSidebar;
		std::unique_ptr<RightContent> m_rightContent;
	};
}
