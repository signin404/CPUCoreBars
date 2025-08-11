// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>

// CCpuUsageItem implementation
CCpuUsageItem::CCpuUsageItem(int core_index) : m_core_index(core_index)
{
    // 修复：显示的核心编号应该从1开始，而不是从0开始
    swprintf_s(m_item_name, L"CPU Core %d", m_core_index + 1);
    swprintf_s(m_item_id, L"cpu_core_%d", m_core_index);
}

const wchar_t* CCpuUsageItem::GetItemName() const { return m_item_name; }
const wchar_t* CCpuUsageItem::GetItemId() const { return m_item_id; }
const wchar_t* CCpuUsageItem::GetItemLableText() const { return L""; }

const wchar_t* CCpuUsageItem::GetItemValueText() const 
{ 
    // 修复：提供有意义的数值文本显示CPU使用率百分比
    static wchar_t value_text[16];
    swprintf_s(value_text, L"%.1f%%", m_usage * 100.0);
    return value_text;
}

const wchar_t* CCpuUsageItem::GetItemValueSampleText() const 
{ 
    // 修复：提供示例文本用于计算显示宽度
    return L"100.0%"; 
}

bool CCpuUsageItem::IsCustomDraw() const { return true; }

int CCpuUsageItem::GetItemWidth() const { return 8; }

void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };
    
    // 背景色
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);
    
    // 计算bar高度
    int bar_height = static_cast<int>(h * m_usage);
    if (bar_height > 0)
    {
        RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
        
        // 根据使用率选择颜色
        COLORREF bar_color;
        if (m_usage < 0.5)
        {
            // 低使用率：绿色
            bar_color = dark_mode ? RGB(0, 200, 0) : RGB(0, 128, 0);
        }
        else if (m_usage < 0.8)
        {
            // 中等使用率：橙色
            bar_color = dark_mode ? RGB(255, 165, 0) : RGB(255, 140, 0);
        }
        else
        {
            // 高使用率：红色
            bar_color = dark_mode ? RGB(255, 100, 100) : RGB(200, 0, 0);
        }
        
        HBRUSH bar_brush = CreateSolidBrush(bar_color);
        FillRect(dc, &bar_rect, bar_brush);
        DeleteObject(bar_brush);
    }
}

void CCpuUsageItem::SetUsage(double usage) 
{ 
    m_usage = max(0.0, min(1.0, usage)); 
}


// CCPUCoreBarsPlugin implementation
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin()
{
    // 1. 动态加载 ntdll.dll 并获取函数地址
    HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (hNtDll)
    {
        m_pNtQuerySystemInformation = (pNtQuerySystemInformation)GetProcAddress(hNtDll, "NtQuerySystemInformation");
    }

    if (!m_pNtQuerySystemInformation)
    {
        return;
    }

    // 2. 获取CPU核心数
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_num_cores = sys_info.dwNumberOfProcessors;

    // 3. 根据核心数创建插件项目和数据缓冲区
    if (m_num_cores > 0)
    {
        m_items.reserve(m_num_cores);
        m_last_perf_info.resize(m_num_cores);
        
        // 修复：确保索引与实际核心对应
        for (int i = 0; i < m_num_cores; ++i)
        {
            // 传入的core_index从0开始，但显示名称会在CCpuUsageItem构造函数中调整为从1开始
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
    // 修复：确保索引范围检查正确
    if (index >= 0 && index < static_cast<int>(m_items.size()))
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
    case TMI_NAME: 
        return L"CPU Core Usage Bars";
    case TMI_DESCRIPTION: 
        return L"Displays each CPU core usage as a vertical bar. Core numbers start from 1.";
    case TMI_AUTHOR: 
        return L"Your Name";
    case TMI_COPYRIGHT: 
        return L"Copyright (C) 2025";
    case TMI_URL: 
        return L"";
    case TMI_VERSION: 
        return L"3.0.2"; // 版本号更新，修复核心编号问题
    default: 
        return L"";
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

    NTSTATUS status = m_pNtQuerySystemInformation(
        SystemProcessorPerformanceInformation, 
        current_perf_info.data(),
        m_num_cores * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION), 
        &return_length
    );

    if (status == 0) // 0 means STATUS_SUCCESS
    {
        if (m_is_first_run)
        {
            m_last_perf_info = current_perf_info;
            m_is_first_run = false;
            return;
        }

        // 修复：确保核心索引正确对应
        for (int i = 0; i < m_num_cores; ++i)
        {
            // 计算总时间
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
                // 修复：确保索引对应正确的核心
                if (i < static_cast<int>(m_items.size()))
                {
                    m_items[i]->SetUsage(usage);
                }
            }
            else
            {
                if (i < static_cast<int>(m_items.size()))
                {
                    m_items[i]->SetUsage(0.0);
                }
            }
        }

        m_last_perf_info = current_perf_info;
    }
}

// 修复：添加调试信息帮助函数（可选，用于调试）
const wchar_t* CCPUCoreBarsPlugin::GetTooltipInfo()
{
    static wchar_t tooltip_text[256];
    swprintf_s(tooltip_text, L"CPU Core Bars Plugin\nMonitoring %d cores", m_num_cores);
    return tooltip_text;
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}