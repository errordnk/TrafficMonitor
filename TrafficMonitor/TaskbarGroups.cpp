#include "stdafx.h"
#include "TaskbarGroups.h"
#include "Common.h"
#include <d3d11.h>
#include <d2d1_3.h>
#include <shlwapi.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "shlwapi.lib")

static const wchar_t* const INI_SECTION = L"task_bar";
static const wchar_t* const DEFAULT_GROUP_NAME = L"Default";

//////////////////////////////////////////////////////////////////////////////////
// CTaskbarGroupSet

CTaskbarGroupSet::CTaskbarGroupSet()
{
    m_groups.push_back(TaskbarGroup{ DEFAULT_GROUP_NAME, L"", {} });
}

std::wstring CTaskbarGroupSet::ItemKey(const CommonDisplayItem& item)
{
    const wchar_t* key = item.GetItemIniKeyName();
    std::wstring result = item.IsPlugin() ? L"plugin:" : L"builtin:";
    if (key != nullptr)
        result += key;
    return result;
}

bool CTaskbarGroupSet::Contains(int group_index, const CommonDisplayItem& item) const
{
    if (group_index <= 0 || group_index >= GroupCount())
        return false;
    return m_groups[group_index].items.count(ItemKey(item)) > 0;
}

void CTaskbarGroupSet::SetMember(int group_index, const CommonDisplayItem& item, bool member)
{
    if (group_index <= 0 || group_index >= GroupCount())
        return;
    if (member)
        m_groups[group_index].items.insert(ItemKey(item));
    else
        m_groups[group_index].items.erase(ItemKey(item));
}

bool CTaskbarGroupSet::HasCustomGroupItems() const
{
    for (size_t i = 1; i < m_groups.size(); i++)
    {
        if (!m_groups[i].items.empty())
            return true;
    }
    return false;
}

bool CTaskbarGroupSet::InCustomGroup(const CommonDisplayItem& item) const
{
    for (int i = 1; i < GroupCount(); i++)
    {
        if (Contains(i, item))
            return true;
    }
    return false;
}

int CTaskbarGroupSet::AddGroup(const std::wstring& name)
{
    m_groups.push_back(TaskbarGroup{ name, L"", {} });
    return GroupCount() - 1;
}

bool CTaskbarGroupSet::RemoveGroup(int index)
{
    if (index <= 0 || index >= GroupCount())
        return false;
    m_groups.erase(m_groups.begin() + index);
    return true;
}

void CTaskbarGroupSet::LoadFromIni(const CIniHelper& ini)
{
    m_groups.clear();
    window_width = max(0, ini.GetInt(INI_SECTION, L"group_window_width", 0));
    int count = ini.GetInt(INI_SECTION, L"group_count", 1);
    if (count < 1)
        count = 1;
    if (count > 32)
        count = 32;
    for (int i = 0; i < count; i++)
    {
        std::wstring prefix = L"group_" + std::to_wstring(i) + L"_";
        TaskbarGroup group;
        group.name = ini.GetString(INI_SECTION, (prefix + L"name").c_str(), i == 0 ? DEFAULT_GROUP_NAME : L"");
        if (group.name.empty())
            group.name = (i == 0) ? DEFAULT_GROUP_NAME : (L"Group " + std::to_wstring(i + 1));
        group.icon_path = ini.GetString(INI_SECTION, (prefix + L"icon").c_str(), L"");
        if (i > 0)
        {
            std::vector<std::wstring> keys;
            CCommon::StringSplit(ini.GetString(INI_SECTION, (prefix + L"items").c_str(), L""), L',', keys);
            for (const auto& key : keys)
            {
                if (!key.empty())
                    group.items.insert(key);
            }
        }
        m_groups.push_back(group);
    }
}

void CTaskbarGroupSet::SaveToIni(CIniHelper& ini) const
{
    ini.WriteInt(INI_SECTION, L"group_window_width", window_width);
    ini.WriteInt(INI_SECTION, L"group_count", GroupCount());
    for (int i = 0; i < GroupCount(); i++)
    {
        std::wstring prefix = L"group_" + std::to_wstring(i) + L"_";
        ini.WriteString(INI_SECTION, (prefix + L"name").c_str(), m_groups[i].name);
        ini.WriteString(INI_SECTION, (prefix + L"icon").c_str(), m_groups[i].icon_path);
        if (i > 0)
        {
            std::wstring items;
            for (const auto& key : m_groups[i].items)
            {
                if (!items.empty())
                    items.push_back(L',');
                items += key;
            }
            ini.WriteString(INI_SECTION, (prefix + L"items").c_str(), items);
        }
    }
}

bool CTaskbarGroupSet::operator==(const CTaskbarGroupSet& another) const
{
    return window_width == another.window_width && m_groups == another.m_groups;
}

//////////////////////////////////////////////////////////////////////////////////
// TaskbarGroupIcons

namespace
{
    std::map<std::tuple<std::wstring, int, COLORREF>, HICON> g_icon_cache;

    //Rasterizes an SVG file with Direct2D (ID2D1DeviceContext5, Windows 10 1703+) into a size x size HICON.
    //Returns NULL if the file is not a valid SVG or the system has no SVG support.
    HICON LoadSvgIcon(const std::wstring& path, int size, COLORREF tint)
    {
        using Microsoft::WRL::ComPtr;
        if (size <= 0)
            return NULL;

        //Direct3D device only to get a DXGI device for Direct2D, hardware first and WARP as the fallback
        ComPtr<ID3D11Device> d3d_device;
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                       nullptr, 0, D3D11_SDK_VERSION, &d3d_device, nullptr, nullptr);
        if (FAILED(hr))
            hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                   nullptr, 0, D3D11_SDK_VERSION, &d3d_device, nullptr, nullptr);
        if (FAILED(hr))
            return NULL;
        ComPtr<IDXGIDevice> dxgi_device;
        if (FAILED(d3d_device.As(&dxgi_device)))
            return NULL;

        ComPtr<ID2D1Factory1> factory;
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), nullptr, &factory)))
            return NULL;
        ComPtr<ID2D1Device> d2d_device;
        if (FAILED(factory->CreateDevice(dxgi_device.Get(), &d2d_device)))
            return NULL;
        ComPtr<ID2D1DeviceContext> context;
        if (FAILED(d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context)))
            return NULL;
        ComPtr<ID2D1DeviceContext5> context5;
        if (FAILED(context.As(&context5)))
            return NULL;        //SVG is not supported by this system

        ComPtr<IStream> stream;
        if (FAILED(SHCreateStreamOnFileEx(path.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &stream)))
            return NULL;
        ComPtr<ID2D1SvgDocument> svg;
        if (FAILED(context5->CreateSvgDocument(stream.Get(), D2D1::SizeF(static_cast<float>(size), static_cast<float>(size)), &svg)))
            return NULL;

        //color of "currentColor" is taken from the color attribute of the root element
        ComPtr<ID2D1SvgElement> svg_root;
        svg->GetRoot(&svg_root);
        if (svg_root)
        {
            wchar_t color_text[16]{};
            swprintf_s(color_text, L"#%02x%02x%02x", GetRValue(tint), GetGValue(tint), GetBValue(tint));
            svg_root->SetAttributeValue(L"color", D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, color_text);
        }

        ComPtr<ID2D1Bitmap1> target;
        D2D1_BITMAP_PROPERTIES1 target_props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (FAILED(context->CreateBitmap(D2D1::SizeU(size, size), nullptr, 0, target_props, &target)))
            return NULL;
        context->SetTarget(target.Get());
        context->BeginDraw();
        context->Clear(D2D1::ColorF(0, 0.0f));
        context5->DrawSvgDocument(svg.Get());
        if (FAILED(context->EndDraw()))
            return NULL;

        //copy the result into a CPU readable bitmap
        ComPtr<ID2D1Bitmap1> staging;
        D2D1_BITMAP_PROPERTIES1 staging_props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (FAILED(context->CreateBitmap(D2D1::SizeU(size, size), nullptr, 0, staging_props, &staging)))
            return NULL;
        if (FAILED(staging->CopyFromBitmap(nullptr, target.Get(), nullptr)))
            return NULL;
        D2D1_MAPPED_RECT mapped{};
        if (FAILED(staging->Map(D2D1_MAP_OPTIONS_READ, &mapped)))
            return NULL;

        //top-down 32 bit DIB; icons use straight (not premultiplied) alpha
        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = size;
        bmi.bmiHeader.biHeight = -size;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        BYTE* dst_bits = nullptr;
        HBITMAP color = ::CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&dst_bits), NULL, 0);
        if (color == NULL || dst_bits == nullptr)
        {
            staging->Unmap();
            return NULL;
        }
        for (int y = 0; y < size; y++)
        {
            const BYTE* src = mapped.bits + static_cast<size_t>(y) * mapped.pitch;
            BYTE* dst = dst_bits + static_cast<size_t>(y) * size * 4;
            for (int x = 0; x < size; x++, src += 4, dst += 4)
            {
                BYTE a = src[3];
                if (a == 0 || a == 255)
                {
                    dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = a;
                }
                else
                {
                    dst[0] = static_cast<BYTE>(min(255, src[0] * 255 / a));
                    dst[1] = static_cast<BYTE>(min(255, src[1] * 255 / a));
                    dst[2] = static_cast<BYTE>(min(255, src[2] * 255 / a));
                    dst[3] = a;
                }
            }
        }
        staging->Unmap();

        HBITMAP mask = ::CreateBitmap(size, size, 1, 1, NULL);
        ICONINFO info{};
        info.fIcon = TRUE;
        info.hbmColor = color;
        info.hbmMask = mask;
        HICON icon = ::CreateIconIndirect(&info);
        ::DeleteObject(color);
        if (mask != NULL)
            ::DeleteObject(mask);
        return icon;
    }
}

namespace TaskbarGroupIcons
{
    HICON Get(const std::wstring& path, int size, COLORREF color)
    {
        if (path.empty() || size <= 0)
            return NULL;
        auto key = std::make_tuple(path, size, color);
        auto iter = g_icon_cache.find(key);
        if (iter != g_icon_cache.end())
            return iter->second;
        HICON icon = LoadSvgIcon(path, size, color);
        g_icon_cache[key] = icon;       //a failed load (NULL) is cached too until Clear() is called
        return icon;
    }

    void Clear()
    {
        for (auto& pair : g_icon_cache)
        {
            if (pair.second != NULL)
                ::DestroyIcon(pair.second);
        }
        g_icon_cache.clear();
    }
}
