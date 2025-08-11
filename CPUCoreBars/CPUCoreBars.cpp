// CPUCoreBars/CPUCoreBars.cpp (更新后)
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// CCpuUsageItem implementation
CCpuUsageItem::CCpuUsageItem(int core_index) : m_core_index(core_index)
{
    swprintf_s(m_item_name, L"CPU Core %d", m_core_index);
    swprintf_s(m_item_id, L"cpu_core_%d", m_core_index);
    // --- 新增：设置一个默认颜色 ---
    m_color = RGB(0, 128, 0); // 默认为绿色
}

// ... (GetItemName, GetItemId 等函数保持不变) ...
const wchar_t* CCpuUsageItem::GetItemName() const { return m_item_name; }
const wchar_t* CCpuUsageItem::GetItemId() const { return m_item_id; }
const wchar_t* CCpuUsageItem::GetItemLableText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueSampleText() const { return L""; }
bool CCpuUsageItem::IsCustomDraw() const { return true; }
int CCpuUsageItem::GetItemWidth() const { return 8; }


void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };
    // 背景色仍然可以根据深色模式变化
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    int bar_height = static_cast<int>(h * m_usage);
    RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
    // --- 修改：使用 m_color 绘制条形图 ---
    HBRUSH bar_brush = CreateSolidBrush(m_color);
    FillRect(dc, &bar_rect, bar_brush);
    DeleteObject(bar_brush);
}

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = max(0.0, min(1.0, usage));
}

// --- 新增：颜色接口的实现 ---
COLORREF CCpuUsageItem::GetItemColor() const
{
    return m_color;
}

void CCpuUsageItem::SetItemColor(COLORREF color)
{
    m_color = color;
}


// CCPUCoreBarsPlugin implementation
// ... (构造函数、析构函数等保持不变) ...
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance() { static CCPUCoreBarsPlugin instance; return instance; }
CCPUCoreBarsPlugin::CCPUCoreBarsPlugin() { /* ... */ }
CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin() { /* ... */ }
IPluginItem* CCPUCoreBarsPlugin::GetItem(int index) { /* ... */ }
void CCPUCoreBarsPlugin::DataRequired() { UpdateCpuUsage(); }
void CCPUCoreBarsPlugin::UpdateCpuUsage() { /* ... */ }


const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME: return L"CPU Core Usage Bars";
    case TMI_DESCRIPTION: return L"Displays each CPU core usage as a vertical bar with configurable colors.";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"1.2.0"; // 增加了新功能，更新版本号
    default: return L"";
    }
}

// Exported function (保持不变)
extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}