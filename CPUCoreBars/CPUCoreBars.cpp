// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>

// CCpuUsageItem implementation
CCpuUsageItem::CCpuUsageItem(int core_index) : m_core_index(core_index)
{
    swprintf_s(m_item_name, L"CPU Core %d", m_core_index);
    swprintf_s(m_item_id, L"cpu_core_%d", m_core_index);
}

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
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    int bar_height = static_cast<int>(h * m_usage);
    RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
    HBRUSH bar_brush = CreateSolidBrush(dark_mode ? RGB(0, 200, 0) : RGB(0, 128, 0));
    FillRect(dc, &bar_rect, bar_brush);
    DeleteObject(bar_brush);
}

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = max(0.0, min(1.0, usage)); // Clamp usage between 0.0 and 1.0
}

// CCPUCoreBarsPlugin implementation
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin()
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_num_cores = sys_info.dwNumberOfProcessors;

    for (int i = 0; i < m_num_cores; ++i)
    {
        m_items.push_back(new CCpuUsageItem(i));
    }
}

IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < m_items.size())
    {
        return m_items[index];
    }
    return nullptr;
}

void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
}

const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME: return L"CPU Core Usage Bars";
    case TMI_DESCRIPTION: return L"Displays each CPU core usage as a vertical bar.";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"1.0.0";
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    // For demonstration, we use random data.
    // For a real plugin, you would use PDH or other system calls here.
    for (auto& item : m_items)
    {
        double random_usage = (static_cast<double>(rand() % 101)) / 100.0;
        item->SetUsage(random_usage);
    }
}

// Exported function
extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}