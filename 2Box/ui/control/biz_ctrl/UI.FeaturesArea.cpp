module UI.FeaturesArea;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import Biz.Core;
import MainApp;
import UI.Core;

namespace
{
	class Cell
	{
	public:
		explicit Cell(const RECT& rc) : m_rc(rc)
		{
		}

		void fillWindow(HWND hWnd, bool maintainAspectRatio) const
		{
			RECT finalRc = m_rc;

			if (maintainAspectRatio)
			{
				RECT windowRc{};
				if (GetWindowRect(hWnd, &windowRc))
				{
					const LONG cellWidth = m_rc.right - m_rc.left;
					const LONG cellHeight = m_rc.bottom - m_rc.top;
					const LONG windowWidth = windowRc.right - windowRc.left;
					const LONG windowHeight = windowRc.bottom - windowRc.top;
					if (windowHeight > 0 && cellHeight > 0)
					{
						const double cellAspect = static_cast<double>(cellWidth) / cellHeight;
						const double windowAspect = static_cast<double>(windowWidth) / windowHeight;

						if (windowAspect > cellAspect)
						{
							// 窗口更宽，以宽度为准，高度按比例缩小
							LONG newHeight = static_cast<LONG>(cellWidth / windowAspect);
							finalRc.top = m_rc.top + (cellHeight - newHeight) / 2;
							finalRc.bottom = finalRc.top + newHeight;
						}
						else
						{
							// 窗口更高，以高度为准，宽度按比例缩小
							LONG newWidth = static_cast<LONG>(cellHeight * windowAspect);
							finalRc.left = m_rc.left + (cellWidth - newWidth) / 2;
							finalRc.right = finalRc.left + newWidth;
						}
					}
				}
			}
			SetWindowPos(hWnd, nullptr,
			             finalRc.left, finalRc.top,
			             finalRc.right - finalRc.left, finalRc.bottom - finalRc.top,
			             SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}

	private:
		RECT m_rc;
	};

	class Strip
	{
	public:
		enum class Orientation
		{
			Horizontal,
			Vertical
		};

		Strip(Orientation orientation, const RECT& rc, std::uint32_t count)
			: m_orientation(orientation)
			  , m_rc(rc)
		{
			m_cells.reserve(count);
			if (orientation == Orientation::Horizontal)
			{
				const LONG space = m_rc.right - m_rc.left;
				const LONG spacePerItem = space / count;
				for (std::uint32_t i = 0; i < count; ++i)
				{
					const LONG left = m_rc.left + i * spacePerItem;
					m_cells.emplace_back(RECT{left, m_rc.top, left + spacePerItem, m_rc.bottom});
				}
			}
			else if (orientation == Orientation::Vertical)
			{
				const LONG space = m_rc.bottom - m_rc.top;
				const LONG spacePerItem = space / count;
				for (std::uint32_t i = 0; i < count; ++i)
				{
					const LONG top = m_rc.top + i * spacePerItem;
					m_cells.emplace_back(RECT{m_rc.left, top, m_rc.right, top + spacePerItem});
				}
			}
		}

		const std::vector<Cell>& getCells() const { return m_cells; }

	private:
		Orientation m_orientation;
		RECT m_rc;
		std::vector<Cell> m_cells;
	};

	class LayoutArea
	{
	public:
		/*
		 * 整体思路：
		 * 先确定最小能容纳数量n的正方形（宽和高相等，每排或每列称之为一条Strip）
		 * 每个Strip中再确定其中有几个cell，按水平排列还是按竖直排列
		 * 
		 * 确定 cell 数量（row和col）的大致逻辑：
		 * 根号n后舍弃小数 = row， 那么：
		 * 1、row * row 不可能大于n
		 * 2、if row * row  == n    =>  col = row
		 * 3、if row * row < n  =>  col = row +1
		 *		此时 if row * col < n  =>  row+=1
		 * 这样一来，row * col 就不可能小于 n 了。
		 * 但是row*col会大于n，多的个数 diff = row * col - n， 则需要挑选Strip，减去其中的cell数量，策略分且仅分两种情况：
		 * 1、row 比 col 小1
		 *		这种情况，strip 按水平排列（即作为row），并从上到下执行 diff 次，对每个strip中的cell数减去 1
		 * 2、row 和 col 数相等
		 *		这种情况，strip 按竖直排列（即作为col），并从左到右执行 diff 次，对每个strip中的cell数减去 1
		 */
		LayoutArea(HMONITOR hMonitor, std::uint32_t n)
		{
			MONITORINFO mi{sizeof(mi)};
			if (!GetMonitorInfoW(hMonitor, &mi))
			{
				return;
			}
			const RECT& rcWork = mi.rcWork;

			std::uint32_t row = static_cast<std::uint32_t>(std::sqrt(n));
			std::uint32_t col;
			if (row * row == n)
			{
				col = row;
			}
			// row * row < n
			else
			{
				col = row + 1;
				if (row * col < n)
				{
					row += 1;
				}
			}
			const std::uint32_t total = row * col;
			const std::uint32_t countDiff = total - n;
			if (row == col)
			{
				const LONG hSpace = rcWork.right - rcWork.left;
				const LONG hSpacePerItem = hSpace / col;
				for (std::uint32_t i = 0; i < col; ++i)
				{
					const LONG left = rcWork.left + i * hSpacePerItem;
					const std::uint32_t cellCount = i < countDiff ? row - 1 : row;
					m_strips.emplace_back(Strip::Orientation::Vertical,
					                      RECT{left, rcWork.top, left + hSpacePerItem, rcWork.bottom},
					                      cellCount);
				}
			}
			// row < col
			else
			{
				const LONG vSpace = rcWork.bottom - rcWork.top;
				const LONG vSpacePerItem = vSpace / row;
				for (std::uint32_t i = 0; i < row; ++i)
				{
					const LONG top = rcWork.top + i * vSpacePerItem;
					const std::uint32_t cellCount = i < countDiff ? col - 1 : col;
					m_strips.emplace_back(Strip::Orientation::Horizontal,
					                      RECT{rcWork.left, top, rcWork.right, top + vSpacePerItem},
					                      cellCount);
				}
			}
		}

		void tileWindow(const std::vector<HWND>& windows, bool maintainAspectRatio) const
		{
			std::uint32_t index = 0;
			for (const Strip& strip : m_strips)
			{
				const std::vector<Cell>& cells = strip.getCells();
				for (const Cell& cell : cells)
				{
					if (index >= windows.size())
					{
						return;
					}
					cell.fillWindow(windows[index], maintainAspectRatio);
					index++;
				}
			}
		}

	private:
		std::vector<Strip> m_strips;
	};

	class LayoutPerMonitor
	{
	public:
		explicit LayoutPerMonitor(const std::vector<void*>& allWindows)
		{
			for (void* window : allWindows)
			{
				HWND hWnd = static_cast<HWND>(window);
				if (IsWindowVisible(hWnd) && IsWindowEnabled(hWnd) && !IsMinimized(hWnd) && (GetWindowExStyle(hWnd) & WS_EX_TOOLWINDOW) == 0)
				{
					if (const HWND owner = GetWindow(hWnd, GW_OWNER); owner && !IsWindowVisible(owner))
					{
						continue;
					}
					if (HMONITOR hMonitor = MonitorFromWindow(hWnd,MONITOR_DEFAULTTONEAREST))
					{
						auto it = m_windows.find(hMonitor);
						if (it != m_windows.end())
						{
							it->second.push_back(hWnd);
						}
						else
						{
							m_windows.insert(std::make_pair(hMonitor, std::vector{hWnd}));
						}
					}
				}
			}
		}

		void tileWindow(bool maintainAspectRatio) const
		{
			for (const auto& pair : m_windows)
			{
				const LayoutArea layout(pair.first, static_cast<std::uint32_t>(pair.second.size()));
				layout.tileWindow(pair.second, maintainAspectRatio);
			}
		}

	private:
		std::unordered_map<HMONITOR, std::vector<HWND>> m_windows;
	};
}

namespace
{
	constexpr float PADDING = 16.f;
	constexpr float GAP = 8.f;
	constexpr float BUTTON_WIDTH = 78.f;
	constexpr float BUTTON_HEIGHT = 36.f;
	constexpr float CHECKBOX_SIZE = 12.f;
	constexpr float CHECK_BOX_X_POS = PADDING + BUTTON_WIDTH + GAP;
	constexpr float CHECK_BOX_Y_POS = PADDING + (BUTTON_HEIGHT - CHECKBOX_SIZE) / 2;
	constexpr float CHECK_BOX_TIPS_X_POS = CHECK_BOX_X_POS + CHECKBOX_SIZE + 4.f;
	constexpr std::wstring_view CHECK_TIPS{L"平铺时维持窗口当前比例"};
	constexpr std::wstring_view SYNC_TIPS{
		L"同步输入说明：\n"
		L"环境内任意前台窗口中按下 Ctrl+Alt+S 开启输入同步\n"
		L"环境内任意前台窗口中按下 Ctrl+Alt+C 关闭输入同步\n\n"
		L"开启输入同步后，前台窗口会作为 主窗口 将输入同步至其他环境中的所有窗口\n"
		L"切换前台窗口并按下 Ctrl+Alt+S 可直接切换 主窗口\n\n"
		L"注意：仅支持顶层窗口间的输入同步。暂不支持窗口内的子窗口（子控件）\n"
	};
}

namespace ui
{
	void FeaturesArea::initialize()
	{
		m_tileWndBtn.setText(L"平铺窗口");
		m_tileWndBtn.setBackgroundColor(D2D1::ColorF(0xf0f0f0), Button::EState::Normal);
		m_tileWndBtn.setBackgroundColor(D2D1::ColorF(0xe0e0e0), Button::EState::Hover);
		m_tileWndBtn.setBackgroundColor(D2D1::ColorF(0xd5d5d5), Button::EState::Active);
		m_tileWndBtn.setTextColor(D2D1::ColorF(0x333333));
		m_tileWndBtn.setOnClick([this]
		{
			const LayoutPerMonitor layout(biz::env_mgr().getAllToplevelWindows());
			layout.tileWindow(m_bMaintainAspectRatio);
		});

		m_tileCheckbox.setRadius(0.f);
		m_tileCheckbox.setBackgroundColor(D2D1::ColorF(0, 0.f));
		m_tileCheckbox.setBorderColor(D2D1::ColorF{0x333333});
		m_tileCheckbox.setOnClick([this]
		{
			m_bMaintainAspectRatio = !m_bMaintainAspectRatio;
			m_tileCheckbox.update();
		});
		m_tileCheckbox.setDrawCallback(std::bind(&FeaturesArea::drawCheckBox, this, std::placeholders::_1, std::placeholders::_2));

		m_checkBoxTextWidth = 132.f;
		m_checkBoxTextHeight = 15.f;
		if (SUCCEEDED(app().dWriteFactory()->CreateTextLayout(CHECK_TIPS.data(),
			static_cast<UINT32>(CHECK_TIPS.size()),
			app().textFormat().pTipsFormat,
			std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
			&m_checkBoxTextLayout)))
		{
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(m_checkBoxTextLayout->GetMetrics(&textMetrics)))
			{
				m_checkBoxTextWidth = textMetrics.width;
				m_checkBoxTextHeight = textMetrics.height;
			}
		}

		m_syncBtn.setText(L"同步输入");
		m_syncBtn.setBackgroundColor(D2D1::ColorF(0xf0f0f0), Button::EState::Normal);
		m_syncBtn.setBackgroundColor(D2D1::ColorF(0xe0e0e0), Button::EState::Hover);
		m_syncBtn.setBackgroundColor(D2D1::ColorF(0xd5d5d5), Button::EState::Active);
		m_syncBtn.setTextColor(D2D1::ColorF(0x333333));
		m_syncBtn.setOnClick([this]
		{
			MessageBoxW(owner()->nativeHandle(), SYNC_TIPS.data(), MainApp::appName.data(), MB_OK);
		});

		for (Button* bgBtn : {&m_setBackgroundBtn, &m_clearBackgroundBtn})
		{
			bgBtn->setBackgroundColor(D2D1::ColorF(0xf0f0f0), Button::EState::Normal);
			bgBtn->setBackgroundColor(D2D1::ColorF(0xe0e0e0), Button::EState::Hover);
			bgBtn->setBackgroundColor(D2D1::ColorF(0xd5d5d5), Button::EState::Active);
			bgBtn->setTextColor(D2D1::ColorF(0x333333));
		}
		m_clearBackgroundBtn.setText(L"清除背景");
		m_setBackgroundBtn.setOnClick([this] { onSetBackgroundBtnClick(); });
		m_clearBackgroundBtn.setOnClick([this] { onClearBackgroundBtnClick(); });
		updateBackgroundButtons();
	}

	void FeaturesArea::updateBackgroundButtons()
	{
		m_setBackgroundBtn.setText(ui::has_app_background_image() ? L"更换背景" : L"设置背景");
		m_clearBackgroundBtn.update();
	}

	void FeaturesArea::onSetBackgroundBtnClick()
	{
		const std::optional<std::wstring> selectedPath = ui::select_image_file(owner());
		if (!selectedPath.has_value())
		{
			return;
		}
		if (!ui::validate_image_file(selectedPath.value()))
		{
			MessageBoxW(owner()->nativeHandle(),
			            L"无法加载该图片，请选择有效的图片文件（支持 png/jpg/jpeg/bmp/gif/tif 等格式）。",
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR);
			return;
		}

		// 复制到 Env\data 目录后再使用，避免原图片被移动或删除后背景失效
		const std::wstring dataDir = biz::app_data_dir();
		const std::optional<std::wstring> copiedFileName = ui::import_background_image(dataDir, selectedPath.value());
		if (!copiedFileName.has_value())
		{
			MessageBoxW(owner()->nativeHandle(),
			            L"复制背景图片到数据目录失败，请检查磁盘空间或文件权限。",
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR);
			return;
		}
		const std::wstring imagePath = (std::filesystem::path{dataDir} / copiedFileName.value()).native();
		if (!ui::set_app_background_image(imagePath))
		{
			MessageBoxW(owner()->nativeHandle(),
			            L"无法加载复制后的背景图片。",
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR);
			return;
		}
		try
		{
			biz::set_app_setting(ui::BACKGROUND_IMAGE_SETTING_NAME, copiedFileName.value());
		}
		catch (const std::exception& e)
		{
			const std::wstring errorMsg = utf8_to_wide_string(e.what());
			MessageBoxW(owner()->nativeHandle(),
			            std::format(L"保存背景图片设置失败：{}", errorMsg).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR);
		}
		updateBackgroundButtons();
		owner()->invalidateRect();
	}

	void FeaturesArea::onClearBackgroundBtnClick()
	{
		if (!ui::has_app_background_image())
		{
			return;
		}
		ui::clear_app_background_image();
		try
		{
			biz::set_app_setting(ui::BACKGROUND_IMAGE_SETTING_NAME, L"");
		}
		catch (const std::exception& e)
		{
			const std::wstring errorMsg = utf8_to_wide_string(e.what());
			MessageBoxW(owner()->nativeHandle(),
			            std::format(L"清除背景图片设置失败：{}", errorMsg).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR);
		}
		ui::remove_background_image_files(biz::app_data_dir());
		updateBackgroundButtons();
		owner()->invalidateRect();
	}

	void FeaturesArea::onResize(float width, float height)
	{
		m_tileWndBtn.setBounds(D2D1::RectF(PADDING, PADDING, PADDING + BUTTON_WIDTH, PADDING + BUTTON_HEIGHT));
		m_tileCheckbox.setBounds(D2D1::RectF(CHECK_BOX_X_POS, CHECK_BOX_Y_POS, CHECK_BOX_X_POS + CHECKBOX_SIZE, CHECK_BOX_Y_POS + CHECKBOX_SIZE));
		const float syncBtnXPos = CHECK_BOX_TIPS_X_POS + m_checkBoxTextWidth + PADDING * 4.f;
		m_syncBtn.setBounds(D2D1::RectF(syncBtnXPos, PADDING, syncBtnXPos + BUTTON_WIDTH, PADDING + BUTTON_HEIGHT));
		// 背景图片相关按钮优先靠右放置；窗口过窄时紧随“同步输入”按钮排列，避免重叠
		const float setBtnXPosAfterSync = syncBtnXPos + BUTTON_WIDTH + GAP;
		const float setBtnXPosRightAligned = width - PADDING - BUTTON_WIDTH - GAP - BUTTON_WIDTH;
		const float setBtnXPos = setBtnXPosRightAligned >= setBtnXPosAfterSync ? setBtnXPosRightAligned : setBtnXPosAfterSync;
		m_setBackgroundBtn.setBounds(D2D1::RectF(setBtnXPos, PADDING,
		                                         setBtnXPos + BUTTON_WIDTH, PADDING + BUTTON_HEIGHT));
		const float clearBackgroundBtnXPos = setBtnXPos + BUTTON_WIDTH + GAP;
		m_clearBackgroundBtn.setBounds(D2D1::RectF(clearBackgroundBtnXPos, PADDING,
		                                           clearBackgroundBtnXPos + BUTTON_WIDTH, PADDING + BUTTON_HEIGHT));
	}

	void FeaturesArea::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto [width, height] = size();

		const D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(0.f, 0.f, width, height), 8.f, 8.f);
		// 设置了背景图片时面板底色半透明，让背景图片透出
		solidBrush->SetColor(ui::has_app_background_image() ? D2D1::ColorF(0xffffff, 0.14f) : D2D1::ColorF(0xffffff));
		renderTarget->FillRoundedRectangle(roundedRect, solidBrush);
		solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
		renderTarget->DrawRoundedRectangle(roundedRect, solidBrush);

		m_tileWndBtn.draw(renderCtx);
		m_tileCheckbox.draw(renderCtx);

		solidBrush->SetColor(D2D1::ColorF(0x333333));
		if (m_checkBoxTextLayout)
		{
			const float paddingTop = CHECK_BOX_Y_POS + (CHECKBOX_SIZE - m_checkBoxTextHeight) * 0.5f;
			renderTarget->DrawTextLayout(D2D1::Point2F(CHECK_BOX_TIPS_X_POS, paddingTop), m_checkBoxTextLayout, solidBrush);
		}
		else
		{
			renderTarget->DrawTextW(CHECK_TIPS.data(), static_cast<UINT>(CHECK_TIPS.size()),
			                        app().textFormat().pTipsFormat,
			                        D2D1::RectF(CHECK_BOX_TIPS_X_POS, CHECK_BOX_Y_POS - 2.f, width - PADDING, CHECK_BOX_Y_POS + 2.f), solidBrush);
		}

		m_syncBtn.draw(renderCtx);
		m_setBackgroundBtn.draw(renderCtx);
		m_clearBackgroundBtn.draw(renderCtx);
	}

	void FeaturesArea::drawCheckBox(const RenderContext& renderCtx, Button::EState) const
	{
		if (!m_bMaintainAspectRatio)
		{
			return;
		}
		const D2D1_RECT_F& bounds = m_tileCheckbox.getBounds();
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const float width = bounds.right - bounds.left;
		const float height = bounds.bottom - bounds.top;
		const float paddingLr = width * 0.236f;
		const float paddingTb = height * 0.236f;
		const D2D1_POINT_2F pt1{D2D1::Point2F(paddingLr, height * 0.5f)};
		const D2D1_POINT_2F pt2{D2D1::Point2F(width * 0.5f, height - paddingTb)};
		const D2D1_POINT_2F pt3{D2D1::Point2F(width - paddingLr, paddingTb)};

		solidBrush->SetColor(D2D1::ColorF(0x555555));
		renderTarget->DrawLine(pt1, pt2, solidBrush);
		renderTarget->DrawLine(pt2, pt3, solidBrush);
	}
}
