#include "CPUUsage.h"
#include <string>
#include <Pdh.h>
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// Helper function to convert string to wstring
std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

// CCpuUsageItem
CCpuUsageItem::CCpuUsageItem(int core_index) : m_core_index(core_index), m_usage(0.0)
{
    swprintf_s(m_item_name, L"CPU Core %d", m_core_index);
    swprintf_s(m_item_id, L"cpu_core_%d", m_core_index);
}

CCpuUsageItem::~CCpuUsageItem()
{
}

const wchar_t* CCpuUsageItem::GetItemName() const
{
    return m_item_name;
}

const wchar_t* CCpuUsageItem::GetItemId() const
{
    return m_item_id;
}

const wchar_t* CCpuUsageItem::GetItemLableText() const
{
    return L"";
}

const wchar_t* CCpuUsageItem::GetItemValueText() const
{
    return L"";
}

const wchar_t* CCpuUsageItem::GetItemValueSampleText() const
{
    return L"";
}

bool CCpuUsageItem::IsCustomDraw() const
{
    return true;
}

int CCpuUsageItem::GetItemWidth() const
{
    return 8; // Width of the bar
}

void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };

    // Draw background
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(0, 0, 0) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    // Draw bar
    int bar_height = (int)(h * m_usage);
    RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
    HBRUSH bar_brush = CreateSolidBrush(dark_mode ? RGB(0, 255, 0) : RGB(0, 128, 0));
    FillRect(dc, &bar_rect, bar_brush);
    DeleteObject(bar_brush);
}

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = usage;
}

// CCPUUsagePlugin
CCPUUsagePlugin& CCPUUsagePlugin::Instance()
{
    static CCPUUsagePlugin instance;
    return instance;
}

CCPUUsagePlugin::CCPUUsagePlugin()
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_num_cores = sys_info.dwNumberOfProcessors;

    for (int i = 0; i < m_num_cores; ++i)
    {
        m_items.push_back(new CCpuUsageItem(i));
    }

    m_last_idle_time.QuadPart = 0;
    m_last_kernel_time.QuadPart = 0;
    m_last_user_time.QuadPart = 0;
    m_last_core_times.resize(m_num_cores);
    for (int i = 0; i < m_num_cores; ++i)
    {
        m_last_core_times[i].QuadPart = 0;
    }
}

CCPUUsagePlugin::~CCPUUsagePlugin()
{
    for (auto item : m_items)
    {
        delete item;
    }
}

IPluginItem* CCPUUsagePlugin::GetItem(int index)
{
    if (index >= 0 && index < m_items.size())
    {
        return m_items[index];
    }
    return nullptr;
}

void CCPUUsagePlugin::DataRequired()
{
    GetCpuUsage();
}

const wchar_t* CCPUUsagePlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:
        return L"CPU Core Usage Plugin";
    case TMI_DESCRIPTION:
        return L"Displays the usage of each CPU core as a vertical bar graph.";
    case TMI_AUTHOR:
        return L"Your Name";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2025";
    case TMI_URL:
        return L"";
    case TMI_VERSION:
        return L"1.0";
    default:
        return L"";
    }
}

void CCPUUsagePlugin::GetCpuUsage()
{
    // This is a simplified example. For accurate per-core usage,
    // a more sophisticated method using PDH (Performance Data Helper) is recommended.
    // The following is a placeholder to demonstrate the drawing.
    for (int i = 0; i < m_num_cores; ++i)
    {
        // Replace with actual per-core usage calculation
        double usage = (double)(rand() % 101) / 100.0;
        m_items[i]->SetUsage(usage);
    }
}


// Exported function
extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUUsagePlugin::Instance();
}