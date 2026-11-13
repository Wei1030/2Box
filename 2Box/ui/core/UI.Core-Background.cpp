module;

#define NOMINMAX
// WIC 需要先包含 windows.h
#include <windows.h>
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")

module UI.Core;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import std;

namespace ui
{
	namespace
	{
		// 背景图片副本固定存放在数据目录下的该文件名（保留原扩展名）
		constexpr std::wstring_view BACKGROUND_FILE_STEM = L"background";

		struct BackgroundImageState
		{
			std::wstring path;
			// 解码后的图片源，保留它以便设备资源重建时重新创建 D2D 位图
			UniqueComPtr<IWICFormatConverter> wicSource;
			UniqueComPtr<ID2D1Bitmap> bitmap;
		};

		BackgroundImageState g_backgroundImageState{};

		std::optional<UniqueComPtr<IWICFormatConverter>> decode_image_file(std::wstring_view imagePath)
		{
			const std::wstring path{imagePath};
			try
			{
				UniqueComPtr<IWICImagingFactory> factory;
				if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
				                           __uuidof(IWICImagingFactory), reinterpret_cast<void**>(&factory))))
				{
					return std::nullopt;
				}

				UniqueComPtr<IWICBitmapDecoder> decoder;
				if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
				                                              WICDecodeMetadataCacheOnDemand, &decoder)))
				{
					return std::nullopt;
				}

				UniqueComPtr<IWICBitmapFrameDecode> frame;
				if (FAILED(decoder->GetFrame(0, &frame)))
				{
					return std::nullopt;
				}

				// 完整解码到内存，避免运行期间一直占用图片文件
				UniqueComPtr<IWICBitmap> wicBitmap;
				if (FAILED(factory->CreateBitmapFromSource(frame.get(), WICBitmapCacheOnLoad, &wicBitmap)))
				{
					return std::nullopt;
				}

				UniqueComPtr<IWICFormatConverter> converter;
				if (FAILED(factory->CreateFormatConverter(&converter)))
				{
					return std::nullopt;
				}
				if (FAILED(converter->Initialize(wicBitmap.get(), GUID_WICPixelFormat32bppPBGRA,
				                                 WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom)))
				{
					return std::nullopt;
				}
				return std::optional<UniqueComPtr<IWICFormatConverter>>{std::move(converter)};
			}
			catch (...)
			{
				return std::nullopt;
			}
		}

		bool is_background_file_name(const std::wstring& fileName)
		{
			if (fileName.size() <= BACKGROUND_FILE_STEM.size())
			{
				return false;
			}
			if (fileName.compare(0, BACKGROUND_FILE_STEM.size(), BACKGROUND_FILE_STEM) != 0)
			{
				return false;
			}
			return fileName[BACKGROUND_FILE_STEM.size()] == L'.';
		}

		std::vector<std::filesystem::path> find_background_files(std::wstring_view dataDir)
		{
			namespace fs = std::filesystem;
			std::vector<fs::path> result;
			try
			{
				for (const fs::directory_entry& entry : fs::directory_iterator(fs::path{dataDir}))
				{
					if (!entry.is_regular_file())
					{
						continue;
					}
					if (is_background_file_name(entry.path().filename().native()))
					{
						result.push_back(entry.path());
					}
				}
			}
			catch (...)
			{
			}
			return result;
		}
	}

	bool validate_image_file(std::wstring_view imagePath)
	{
		return decode_image_file(imagePath).has_value();
	}

	std::optional<std::wstring> import_background_image(std::wstring_view dataDir, std::wstring_view sourceImagePath)
	{
		namespace fs = std::filesystem;
		const fs::path source{sourceImagePath};
		const fs::path dataDirPath{dataDir};
		std::optional<fs::path> tempPath;
		try
		{
			if (!fs::is_regular_file(source))
			{
				return std::nullopt;
			}
			const std::wstring extension = source.extension().native();
			if (extension.empty())
			{
				return std::nullopt;
			}

			fs::create_directories(dataDirPath);
			const fs::path target = dataDirPath / (std::wstring{BACKGROUND_FILE_STEM} + extension);
			const fs::path temp = dataDirPath / (std::wstring{BACKGROUND_FILE_STEM} + extension + L".tmp");
			tempPath = temp;

			fs::remove(temp);
			fs::copy_file(source, temp, fs::copy_options::overwrite_existing);
			// 删除数据目录下旧的背景图片副本（扩展名可能不同）
			for (const fs::path& old : find_background_files(dataDir))
			{
				if (old != temp)
				{
					fs::remove(old);
				}
			}
			fs::rename(temp, target);
			tempPath.reset();
			return target.filename().native();
		}
		catch (...)
		{
			if (tempPath.has_value())
			{
				try
				{
					fs::remove(tempPath.value());
				}
				catch (...)
				{
				}
			}
			return std::nullopt;
		}
	}

	void remove_background_image_files(std::wstring_view dataDir)
	{
		namespace fs = std::filesystem;
		for (const fs::path& path : find_background_files(dataDir))
		{
			try
			{
				fs::remove(path);
			}
			catch (...)
			{
			}
		}
	}

	bool set_app_background_image(std::wstring_view imagePath)
	{
		clear_app_background_image();
		std::optional<UniqueComPtr<IWICFormatConverter>> source = decode_image_file(imagePath);
		if (!source.has_value())
		{
			return false;
		}
		g_backgroundImageState.path = std::wstring{imagePath};
		g_backgroundImageState.wicSource = std::move(source.value());
		return true;
	}

	void clear_app_background_image()
	{
		g_backgroundImageState.bitmap.reset();
		g_backgroundImageState.wicSource.reset();
		g_backgroundImageState.path.clear();
	}

	bool has_app_background_image()
	{
		return !g_backgroundImageState.path.empty() && g_backgroundImageState.wicSource;
	}

	void reset_app_background_device_resources()
	{
		g_backgroundImageState.bitmap.reset();
	}

	void draw_app_background(const RenderContext& renderCtx, const D2D1_RECT_F& contentRect)
	{
		if (!has_app_background_image())
		{
			return;
		}

		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		if (!renderTarget)
		{
			return;
		}

		// D2D 位图依赖窗口渲染目标，设备资源重建后需要重新创建
		if (!g_backgroundImageState.bitmap)
		{
			if (FAILED(renderTarget->CreateBitmapFromWicBitmap(g_backgroundImageState.wicSource.get(),
			                                                    &g_backgroundImageState.bitmap)))
			{
				return;
			}
		}

		ID2D1Bitmap* const bitmap = g_backgroundImageState.bitmap.get();
		if (!bitmap)
		{
			return;
		}
		const D2D1_SIZE_F imageSize = bitmap->GetSize();
		const float contentWidth = contentRect.right - contentRect.left;
		const float contentHeight = contentRect.bottom - contentRect.top;
		if (imageSize.width <= 0.f || imageSize.height <= 0.f || contentWidth <= 0.f || contentHeight <= 0.f)
		{
			return;
		}

		// 保持宽高比铺满内容区域（居中裁剪）
		const float scale = std::max(contentWidth / imageSize.width, contentHeight / imageSize.height);
		const float drawWidth = imageSize.width * scale;
		const float drawHeight = imageSize.height * scale;
		const D2D1_RECT_F destRect = D2D1::RectF(
			contentRect.left + (contentWidth - drawWidth) * 0.5f,
			contentRect.top + (contentHeight - drawHeight) * 0.5f,
			contentRect.left + (contentWidth + drawWidth) * 0.5f,
			contentRect.top + (contentHeight + drawHeight) * 0.5f);

		renderTarget->PushAxisAlignedClip(contentRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		renderTarget->DrawBitmap(bitmap, destRect, 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
		renderTarget->PopAxisAlignedClip();
	}
}
