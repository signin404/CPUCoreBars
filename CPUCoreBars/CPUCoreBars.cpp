// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// =================================================================
// CCpuUsageItem implementation
// =================================================================

CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core) 
    : m_core_index(core_index), m_is_e_core(is_e_core)
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

// **修改**: 增加宽度以容纳更大的图标
int CCpuUsageItem::GetItemWidth() const { return 12; }

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = max(0.0, min(1.0, usage));
}

// **修改**: 放大图标
void CCpuUsageItem::DrawLeafIcon(HDC hDC, const RECT& rect, bool dark_mode)
{
    COLORREF icon_color = dark_mode ? RGB(65, 65, 65) : RGB(225, 225, 225);
    HBRUSH hBrush = CreateSolidBrush(icon_color);
    HPEN hPen = CreatePen(PS_NULL, 0, 0);
    
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);

    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    
    // **修改**: 调整垂直中心和基础大小
    int cx = rect.left + w / 2;
    int cy = rect.top + h / 2 - 1; // 将图标垂直居中
    int size = max(3, w / 2);     // 将基础尺寸从 w/3 增加到 w/2，使其更大

    POINT points[6];
    points[0] = { cx, cy + size };
    points[1] = { cx - size * 3 / 4, cy + size / 4 };
    points[2] = { cx - size / 4, cy - size };
    points[3] = { cx, cy - size * 3 / 2 };
    points[4] = { cx + size / 4, cy - size };
    points[5] = { cx + size * 3 / 4, cy + size / 4 };

    Polygon(hDC, points, 6);

    SelectObject(hDC, hOldBrush);
    SelectObject(hDC, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };

    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    if (m_is_e_core)
    {
        DrawLeafIcon(dc, rect, dark_mode);
    }

    COLORREF bar_color;
    if (m_core_index >= 12 && m_core_index <= 19)
    {
        bar_color = RGB(217, 66, 53);
    }
    else if (m_core_index % 2 == 1)
    {
        bar_color = RGB(38, 160, 218);
    }
    else
    {
        bar_color = RGB(118, 202, 83);
    }

    int bar_height = static_cast<int>(h * m_usage);
    if (bar_height > 0)
    {
        RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
        HBRUSH bar_brush = CreateSolidBrush(bar_color);
        FillRect(dc, &bar_rect, bar_brush);
        DeleteObject(bar_brush);
    }
}


// =================================================================
// CCPUCoreBarsPlugin implementation
// (这部分代码与上一版完全相同，无需修改)
// =================================================================

CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

void CCPUCoreBarsPlugin::DetectCoreTypes()
{
    m_core_efficiency.assign(m_num_cores, 1); 

    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return;
    }

    std::vector<char> buffer(length);
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc_info = 
        (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data();

    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, proc_info, &length))
    {
        return;
    }

    char* ptr = buffer.data();
    while (ptr < buffer.data() + length)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current_info =
            (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

        if (current_info->Relationship == RelationProcessorCore)
        {
            BYTE efficiency = current_info->Processor.EfficiencyClass;
            for (int i = 0; i < current_info->Processor.GroupCount; ++i)
            {
                KAFFINITY mask = current_info->Processor.GroupMask[i].Mask;
                for (int j = 0; j < sizeof(KAFFINITY) * 8; ++j)
                {
                    if ((mask >> j) & 1)
                    {
                        int logical_proc_index = j; 
                        if (logical_proc_index < m_num_cores)
                        {
                            m_core_efficiency[logical_proc_index] = efficiency;
                        }
                    }
                }
            }
        }
        ptr += current_info->Size;
    }
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin()
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_num_cores = sys_info.dwNumberOfProcessors;

    DetectCoreTypes();

    for (int i = 0; i < m_num_cores; ++i)
    {
        bool is_e_core = (m_core_efficiency[i] == 0);
        m_items.push_back(new CCpuUsageItem(i, is_e_core));
    }

    if (PdhOpenQuery(nullptr, 0, &m_query) == ERROR_SUCCESS)
    {
        m_counters.resize(m_num_cores);
        for (int i = 0; i < m_num_cores; ++i)
        {
            wchar_t counter_path[128];
            swprintf_s(counter_path, L"\\Processor(%d)\\%% Processor Time", i);
            PdhAddCounterW(m_query, counter_path, 0, &m_counters[i]);
        }
        PdhCollectQueryData(m_query);
    }
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    if (m_query)
    {
        PdhCloseQuery(m_query);
    }
    for (auto item : m_items)
    {
        delete item;
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
    case TMI_DESCRIPTION: return L"Displays each CPU core usage as a vertical bar with P/E core detection.";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"1.3.1"; // 版本号微调
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    if (!m_query) return;
    if (PdhCollectQueryData(m_query) == ERROR_SUCCESS)
    {
        for (int i = 0; i < m_num_cores; ++i)
        {
            PDH_FMT_COUNTERVALUE value;
            if (PdhGetFormattedCounterValue(m_counters[i], PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS)
            {
                m_items[i]->SetUsage(value.doubleValue / 100.0);
            }
            else
            {
                m_items[i]->SetUsage(0.0);
            }
        }
    }
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}