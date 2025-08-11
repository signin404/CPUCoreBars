// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>

// CCpuUsageItem implementation (No changes needed)
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
void CCpuUsageItem::SetUsage(double usage) { m_usage = max(0.0, min(1.0, usage)); }


// CCPUCoreBarsPlugin implementation (New, reliable implementation)
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin()
{
    // 1. Dynamically load ntdll.dll and get the function address
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (hNtDll)
    {
        m_pNtQuerySystemInformation = (pNtQuerySystemInformation)GetProcAddress(hNtDll, "NtQuerySystemInformation");
    }

    if (!m_pNtQuerySystemInformation)
    {
        return;
    }

    // 2. Get the number of cores
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_num_cores = sys_info.dwNumberOfProcessors;

    // 3. Create plugin items and data buffers based on the core count
    if (m_num_cores > 0)
    {
        m_items.reserve(m_num_cores);
        m_last_perf_info.resize(m_num_cores);
        for (int i = 0; i < m_num_cores; ++i)
        {
            m_items.push_back(new CCpuUsageItem(i));
        }
    }
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
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
    case TMI_DESCRIPTION: return L"Displays each CPU core usage as a vertical bar.";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"3.0.1"; // Build fix
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    if (!m_pNtQuerySystemInformation || m_num_cores == 0)
    {
        return;
    }

    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> current_perf_info(m_num_cores);
    ULONG return_length;

    if (m_pNtQuerySystemInformation(SystemProcessorPerformanceInformation, current_perf_info.data(),
        m_num_cores * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION), &return_length) == 0) // 0 means STATUS_SUCCESS
    {
        if (m_is_first_run)
        {
            m_last_perf_info = current_perf_info;
            m_is_first_run = false;
            return;
        }

        for (int i = 0; i < m_num_cores; ++i)
        {
            ULARGE_INTEGER uli_total_time_last;
            uli_total_time_last.HighPart = m_last_perf_info[i].KernelTime.HighPart + m_last_perf_info[i].UserTime.HighPart;
            uli_total_time_last.LowPart = m_last_perf_info[i].KernelTime.LowPart + m_last_perf_info[i].UserTime.LowPart;

            ULARGE_INTEGER uli_total_time_current;
            uli_total_time_current.HighPart = current_perf_info[i].KernelTime.HighPart + current_perf_info[i].UserTime.HighPart;
            uli_total_time_current.LowPart = current_perf_info[i].KernelTime.LowPart + current_perf_info[i].UserTime.LowPart;
            
            ULONGLONG total_time_delta = uli_total_time_current.QuadPart - uli_total_time_last.QuadPart;
            ULONGLONG idle_time_delta = current_perf_info[i].IdleTime.QuadPart - m_last_perf_info[i].IdleTime.QuadPart;

            if (total_time_delta > 0)
            {
                double usage = (1.0 - (double)idle_time_delta / (double)total_time_delta);
                m_items[i]->SetUsage(usage);
            }
            else
            {
                m_items[i]->SetUsage(0.0);
            }
        }

        m_last_perf_info = current_perf_info;
    }
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}