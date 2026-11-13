module;
#define NOMINMAX
#include <shobjidl_core.h>
module UI.Core;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif
import MainApp;

namespace ui
{
	std::optional<std::wstring> select_file_impl(const WindowBase* owner, const COMDLG_FILTERSPEC* fileTypes, UINT fileTypeCount)
	{
		UniqueComPtr<IFileOpenDialog> fileOpen;
		HResult hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&fileOpen));
		if (FAILED(hr))
		{
			MessageBoxW(owner->nativeHandle(),
			            std::format(L"创建文件选择对话框失败! CoCreateInstance fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return std::nullopt;
		}
		fileOpen->SetFileTypes(fileTypeCount, fileTypes);
		fileOpen->SetFileTypeIndex(1);
		DWORD dwOptions = 0;
		fileOpen->GetOptions(&dwOptions);
		fileOpen->SetOptions(dwOptions | FOS_STRICTFILETYPES | FOS_FORCEFILESYSTEM);
		hr = fileOpen->Show(owner->nativeHandle());
		if (FAILED(hr))
		{
			if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
			{
				MessageBoxW(owner->nativeHandle(),
				            std::format(L"显示文件选择对话框失败! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
				            MainApp::appName.data(),
				            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			}
			return std::nullopt;
		}
		UniqueComPtr<IShellItem> item;
		hr = fileOpen->GetResult(&item);
		if (FAILED(hr))
		{
			MessageBoxW(owner->nativeHandle(),
			            std::format(L"无法获取选择的文件! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return std::nullopt;
		}
		PWSTR pszFilePath;
		hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
		if (FAILED(hr))
		{
			MessageBoxW(owner->nativeHandle(),
			            std::format(L"无法获取选择的文件路径! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return std::nullopt;
		}
		std::wstring procFullPath{pszFilePath};
		CoTaskMemFree(pszFilePath);
		return procFullPath;
	}

	std::optional<std::wstring> select_file(const WindowBase* owner)
	{
		constexpr COMDLG_FILTERSPEC rgSpec[] =
		{
			{L"可执行文件和快捷方式", L"*.exe;*.lnk;*.url"},
			{L"可执行文件", L"*.exe"},
			{L"所有文件", L"*.*"}
		};
		return select_file_impl(owner, rgSpec, ARRAYSIZE(rgSpec));
	}

	std::optional<std::wstring> select_image_file(const WindowBase* owner)
	{
		constexpr COMDLG_FILTERSPEC rgSpec[] =
		{
			{L"图片文件", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff;*.ico"},
			{L"PNG 图片", L"*.png"},
			{L"JPEG 图片", L"*.jpg;*.jpeg"},
			{L"BMP 图片", L"*.bmp"},
			{L"所有文件", L"*.*"}
		};
		return select_file_impl(owner, rgSpec, ARRAYSIZE(rgSpec));
	}

	namespace
	{
		// 提示文本输入对话框中控件ID，用于定位控件
		constexpr WORD PROMPT_INPUT_EDIT_ID = 1001;
		constexpr WORD PROMPT_INPUT_LABEL_ID = 1002;

		// 标准对话框模板中预定义系统类的原子值（见DLGITEMTEMPLATE文档）
		constexpr WORD DIALOG_CLASS_BUTTON = 0x0080;
		constexpr WORD DIALOG_CLASS_EDIT = 0x0081;
		constexpr WORD DIALOG_CLASS_STATIC = 0x0082;

		// 类数组开始处表示“后面跟一个原子值”的标记
		constexpr WORD DIALOG_CLASS_ORDINAL_MARKER = 0xFFFF;

		// DS_SETFONT指定的字体（磅）
		constexpr WORD DIALOG_FONT_POINT_SIZE = 9;

		struct PromptTextInputContext
		{
			std::wstring initialText;
			std::size_t maxLength;
			std::wstring result;
		};

		struct ControlTemplate
		{
			DWORD style;
			short x;
			short y;
			short cx;
			short cy;
			WORD id;
			WORD classAtom;
			std::wstring_view text;
		};

		// 对话框模板中所有数值都是16位小端数据（x86/x64），这里逐字节写入以便精确控制偏移
		void append_word(std::vector<std::uint8_t>& buffer, WORD value)
		{
			buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
			buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
		}

		void append_dword(std::vector<std::uint8_t>& buffer, DWORD value)
		{
			append_word(buffer, static_cast<WORD>(value & 0xFFFF));
			append_word(buffer, static_cast<WORD>((value >> 16) & 0xFFFF));
		}

		// 写入以NUL结尾的Unicode字符串；不要求输入本身以NUL结尾
		void append_string(std::vector<std::uint8_t>& buffer, std::wstring_view text)
		{
			for (const wchar_t ch : text)
			{
				append_word(buffer, static_cast<WORD>(ch));
			}
			append_word(buffer, 0);
		}

		// DLGITEMTEMPLATE必须DWORD对齐；类、标题数组要求WORD对齐（逐WORD写入自然满足）
		void align_dword(std::vector<std::uint8_t>& buffer)
		{
			while (buffer.size() % sizeof(DWORD) != 0)
			{
				buffer.push_back(0);
			}
		}

		void append_control(std::vector<std::uint8_t>& buffer, const ControlTemplate& control)
		{
			align_dword(buffer);

			// DLGITEMTEMPLATE头（固定18字节）
			append_dword(buffer, control.style);
			append_dword(buffer, 0); // dwExtendedStyle
			append_word(buffer, static_cast<WORD>(control.x));
			append_word(buffer, static_cast<WORD>(control.y));
			append_word(buffer, static_cast<WORD>(control.cx));
			append_word(buffer, static_cast<WORD>(control.cy));
			append_word(buffer, control.id);

			// 类数组：0xFFFF标记 + 预定义系统类的原子值
			append_word(buffer, DIALOG_CLASS_ORDINAL_MARKER);
			append_word(buffer, control.classAtom);

			// 标题数组
			append_string(buffer, control.text);

			// 创建数据数组：没有创建数据，只写一个大小为0的WORD
			append_word(buffer, 0);
		}

		// 运行时构建标准格式（非DIALOGEX）的对话框模板，全部尺寸使用对话框单位，随字体/DPI缩放
		std::vector<std::uint8_t> build_prompt_text_input_template(std::wstring_view title, std::wstring_view promptLabel)
		{
			constexpr DWORD DIALOG_STYLE = static_cast<DWORD>(WS_POPUP | WS_CAPTION | WS_SYSMENU |
				DS_MODALFRAME | DS_CENTER | DS_SETFONT);
			constexpr WORD DIALOG_ITEM_COUNT = 4; // 提示文本、编辑框、确定、取消

			// 布局（对话框单位）：编辑框宽180（>=要求的180），左右各留7的边距
			constexpr short DIALOG_WIDTH = 194;
			constexpr short CONTROL_MARGIN = 7;
			constexpr short CONTROL_WIDTH = DIALOG_WIDTH - CONTROL_MARGIN * 2; // 180
			constexpr short PROMPT_LABEL_TOP = 7;
			constexpr short PROMPT_LABEL_HEIGHT = 10;
			constexpr short EDIT_TOP = 20;
			constexpr short EDIT_HEIGHT = 14;
			constexpr short BUTTON_TOP = EDIT_TOP + EDIT_HEIGHT + 8; // 42
			constexpr short BUTTON_WIDTH = 40;
			constexpr short BUTTON_HEIGHT = 14;
			constexpr short BUTTON_GAP = 6;
			constexpr short CANCEL_LEFT = DIALOG_WIDTH - CONTROL_MARGIN - BUTTON_WIDTH; // 147
			constexpr short OK_LEFT = CANCEL_LEFT - BUTTON_GAP - BUTTON_WIDTH; // 101
			constexpr short DIALOG_HEIGHT = BUTTON_TOP + BUTTON_HEIGHT + CONTROL_MARGIN; // 63

			std::vector<std::uint8_t> buffer;
			buffer.reserve(256);

			// DLGTEMPLATE头（固定18字节）
			append_dword(buffer, DIALOG_STYLE);
			append_dword(buffer, 0); // dwExtendedStyle
			append_word(buffer, DIALOG_ITEM_COUNT);
			append_word(buffer, 0); // x
			append_word(buffer, 0); // y
			append_word(buffer, static_cast<WORD>(DIALOG_WIDTH));
			append_word(buffer, static_cast<WORD>(DIALOG_HEIGHT));

			append_word(buffer, 0); // 菜单数组：无菜单
			append_word(buffer, 0); // 窗口类数组：使用系统预定义的对话框窗口类
			append_string(buffer, title);

			// DS_SETFONT：16位字号 + 字体名字符串
			append_word(buffer, DIALOG_FONT_POINT_SIZE);
			append_string(buffer, L"Segoe UI");

			append_control(buffer, ControlTemplate{
				              static_cast<DWORD>(WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX),
				              CONTROL_MARGIN, PROMPT_LABEL_TOP, CONTROL_WIDTH, PROMPT_LABEL_HEIGHT,
				              PROMPT_INPUT_LABEL_ID, DIALOG_CLASS_STATIC, promptLabel
			              });
			append_control(buffer, ControlTemplate{
				              static_cast<DWORD>(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL),
				              CONTROL_MARGIN, EDIT_TOP, CONTROL_WIDTH, EDIT_HEIGHT,
				              PROMPT_INPUT_EDIT_ID, DIALOG_CLASS_EDIT, std::wstring_view{}
			              });
			append_control(buffer, ControlTemplate{
				              static_cast<DWORD>(WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON),
				              OK_LEFT, BUTTON_TOP, BUTTON_WIDTH, BUTTON_HEIGHT,
				              static_cast<WORD>(IDOK), DIALOG_CLASS_BUTTON, L"确定"
			              });
			append_control(buffer, ControlTemplate{
				              static_cast<DWORD>(WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON),
				              CANCEL_LEFT, BUTTON_TOP, BUTTON_WIDTH, BUTTON_HEIGHT,
				              static_cast<WORD>(IDCANCEL), DIALOG_CLASS_BUTTON, L"取消"
			              });

			return buffer;
		}

		void focus_and_select_edit(HWND edit)
		{
			if (!edit)
			{
				return;
			}
			SetFocus(edit);
			SendMessageW(edit, EM_SETSEL, static_cast<WPARAM>(0), static_cast<LPARAM>(-1));
		}

		std::wstring trim_name_text(std::wstring_view text)
		{
			constexpr std::wstring_view TRIMMED_CHARS = L" \t\r\n";
			const std::size_t firstPos = text.find_first_not_of(TRIMMED_CHARS);
			if (firstPos == std::wstring_view::npos)
			{
				return {};
			}
			const std::size_t lastPos = text.find_last_not_of(TRIMMED_CHARS);
			return std::wstring{text.substr(firstPos, lastPos - firstPos + 1)};
		}

		bool contains_control_char(std::wstring_view text)
		{
			for (const wchar_t ch : text)
			{
				if (std::iswcntrl(static_cast<std::wint_t>(ch)) != 0)
				{
					return true;
				}
			}
			return false;
		}

		// 模态文本输入对话框的窗口过程。注意：DLGPROC没有lParam，所以用DWLP_USER保存上下文
		INT_PTR CALLBACK prompt_text_input_proc(HWND dlg, UINT message, WPARAM wParam, LPARAM lParam)
		{
			try
			{
				switch (message)
				{
				case WM_INITDIALOG:
				{
					auto* context = reinterpret_cast<PromptTextInputContext*>(lParam);
					if (!context)
					{
						EndDialog(dlg, 0);
						return TRUE;
					}
					SetWindowLongPtrW(dlg, DWLP_USER, reinterpret_cast<LONG_PTR>(context));

					HWND edit = GetDlgItem(dlg, PROMPT_INPUT_EDIT_ID);
					if (edit)
					{
						SetWindowTextW(edit, context->initialText.c_str());

						// 限制输入长度：1 ~ 255
						std::size_t editLimit = context->maxLength;
						if (editLimit < 1)
						{
							editLimit = 1;
						}
						else if (editLimit > 255)
						{
							editLimit = 255;
						}
						SendMessageW(edit, EM_SETLIMITTEXT, static_cast<WPARAM>(editLimit), 0);

						// 编辑框是模板中的第一个TABSTOP控件，这里返回FALSE并自己设置焦点
						focus_and_select_edit(edit);
					}
					return FALSE;
				}
				case WM_COMMAND:
				{
					if (HIWORD(wParam) != BN_CLICKED)
					{
						return FALSE;
					}

					const WORD controlId = LOWORD(wParam);
					if (controlId == IDCANCEL)
					{
						EndDialog(dlg, 0);
						return TRUE;
					}
					if (controlId != IDOK)
					{
						return FALSE;
					}

					auto* context = reinterpret_cast<PromptTextInputContext*>(GetWindowLongPtrW(dlg, DWLP_USER));
					HWND edit = GetDlgItem(dlg, PROMPT_INPUT_EDIT_ID);
					if (!context || !edit)
					{
						EndDialog(dlg, 0);
						return TRUE;
					}

					std::wstring rawText(static_cast<std::size_t>(GetWindowTextLengthW(edit)) + 1, L'\0');
					const int copiedLength = GetWindowTextW(edit, rawText.data(), static_cast<int>(rawText.size()));
					rawText.resize(copiedLength > 0 ? static_cast<std::size_t>(copiedLength) : 0);

					std::wstring trimmedText = trim_name_text(rawText);
					if (trimmedText.empty())
					{
						MessageBoxW(dlg, L"名称不能为空", MainApp::appName.data(), MB_OK | MB_ICONWARNING);
						focus_and_select_edit(edit);
						return TRUE;
					}
					if (contains_control_char(trimmedText))
					{
						MessageBoxW(dlg, L"名称不能包含控制字符", MainApp::appName.data(), MB_OK | MB_ICONWARNING);
						focus_and_select_edit(edit);
						return TRUE;
					}

					context->result = std::move(trimmedText);
					EndDialog(dlg, 1);
					return TRUE;
				}
				case WM_CLOSE:
				{
					EndDialog(dlg, 0);
					return TRUE;
				}
				default:
					return FALSE;
				}
			}
			catch (...)
			{
				// 绝不让异常逃出窗口过程，否则模态对话框的消息循环会处于未定义状态
				EndDialog(dlg, 0);
				return TRUE;
			}
		}
	}

	std::optional<std::wstring> prompt_text_input(HWND owner, std::wstring_view title, std::wstring_view promptLabel,
	                                              std::wstring_view initialText, std::size_t maxLength)
	{
		try
		{
			std::vector<std::uint8_t> dialogTemplate = build_prompt_text_input_template(title, promptLabel);
			PromptTextInputContext context{std::wstring{initialText}, maxLength, {}};
			const INT_PTR dialogResult = DialogBoxIndirectParamW(
				app().moduleInstance(),
				reinterpret_cast<LPCDLGTEMPLATEW>(dialogTemplate.data()),
				owner,
				prompt_text_input_proc,
				reinterpret_cast<LPARAM>(&context));
			// 1：点击“确定”；0：取消/关闭/owner无效；-1：模板非法等其他创建失败
			if (dialogResult != 1)
			{
				return std::nullopt;
			}
			return std::optional<std::wstring>{std::move(context.result)};
		}
		catch (...)
		{
			return std::nullopt;
		}
	}
}
