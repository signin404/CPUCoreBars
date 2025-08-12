#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>
#include <chrono> // 用于 std::this_thread::sleep_for

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
int CCpuUsageItem::GetItemWidth() const { return 10; }

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = max(0.0, min(1.0, usage));
}

void CCpuUsageItem::DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode)
{
    // 1. 设置符号颜色为高对比度的黑/白
    COLORREF icon_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    SetTextColor(hDC, icon_color);

    // 2. 设置背景为透明，这样就不会在符号周围绘制方框
    SetBkMode(hDC, TRANSPARENT);

    // 3. 定义要绘制的符号
    const wchar_t* symbol = L"\u26B2"; // Unicode for ⚲ (NEUTER)

    // 4. 创建一个合适的字体
    HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");

    HGDIOBJ hOldFont = SelectObject(hDC, hFont);

    // 5. 绘制文本，并使其在矩形区域内水平和垂直居中
    DrawTextW(hDC, symbol, -1, (LPRECT)&rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 6. 清理资源
    SelectObject(hDC, hOldFont);
    DeleteObject(hFont);
}

void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };

    // 1. 绘制背景
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    // 2. 根据核心索引和使用率选择颜色
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

    if (m_usage >= 0.8) // 80% 及以上
    {
        bar_color = RGB(217, 66, 53); // 高负载红色
    }
    else if (m_usage >= 0.5) // 50% 到 80%
    {
        bar_color = RGB(246, 182, 78); // 中负载黄色
    }

    // 3. 绘制使用率条形图 (在绘制图标之前)
    int bar_height = static_cast<int>(h * m_usage);
    if (bar_height > 0)
    {
        RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
        HBRUSH bar_brush = CreateSolidBrush(bar_color);
        FillRect(dc, &bar_rect, bar_brush);
        DeleteObject(bar_brush);
    }

    // 4. 最后绘制图标，确保它在最顶层
    if (m_is_e_core)
    {
        DrawECoreSymbol(dc, rect, dark_mode);
    }
}


// =================================================================
// CCPUCoreBarsPlugin implementation
// =================================================================

CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin() : m_is_running(true)
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
    m_cpu_usage_buffer.resize(m_num_cores, 0.0);

    // 初始化 PDH
    if (PdhOpenQuery(nullptr, 0, &m_query) == ERROR_SUCCESS)
    {
        m_counters.resize(m_num_cores);
        for (int i = 0; i < m_num_cores; ++i)
        {
            wchar_t counter_path[128];
            swprintf_s(counter_path, L"\\Processor(%d)\\%% Processor Time", i);
            PdhAddCounterW(m_query, counter_path, 0, &m_counters[i]);
        }
    }

    // 启动后台工作线程
    m_worker_thread = std::thread(&CCPUCoreBarsPlugin::WorkerThread, this);
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    m_is_running = false;
    if (m_worker_thread.joinable())
    {
        m_worker_thread.join();
    }

    if (m_query) PdhCloseQuery(m_query);
    for (auto item : m_items) delete item;
}

void CCPUCoreBarsPlugin::WorkerThread()
{
    if (!m_query) return;

    // 第一次调用以初始化计数器，否则首次读取会失败
    PdhCollectQueryData(m_query);

    while (m_is_running)
    {
        // 每秒更新一次数据。这是唯一需要sleep的地方。
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!m_is_running) break;

        // 在这个后台线程中执行可能阻塞的调用
        if (PdhCollectQueryData(m_query) == ERROR_SUCCESS)
        {
            std::vector<double> temp_buffer;
            temp_buffer.reserve(m_num_cores);

            for (int i = 0; i < m_num_cores; ++i)
            {
                PDH_FMT_COUNTERVALUE value;
                if (PdhGetFormattedCounterValue(m_counters[i], PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS)
                {
                    temp_buffer.push_back(value.doubleValue / 100.0);
                }
                else
                {
                    temp_buffer.push_back(0.0);
                }
            }

            // 使用互斥锁安全地更新共享缓冲区
            std::lock_guard<std::mutex> lock(m_data_mutex);
            m_cpu_usage_buffer = temp_buffer;
        }
    }
}

void CCPUCoreBarsPlugin::DataRequired()
{
    // 使用互斥锁安全地从共享缓冲区读取数据
    // 这个函数会被TrafficMonitor的主UI线程频繁调用，所以必须非常快
    std::lock_guard<std::mutex> lock(m_data_mutex);
    for (int i = 0; i < m_num_cores; ++i)
    {
        if (i < m_items.size() && i < m_cpu_usage_buffer.size())
        {
            m_items[i]->SetUsage(m_cpu_usage_buffer[i]);
        }
    }
}

void CCPUCoreBarsPlugin::DetectCoreTypes()
{
    m_core_efficiency.assign(m_num_cores, 1); 
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;

    std::vector<char> buffer(length);
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data();
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, proc_info, &length)) return;

    char* ptr = buffer.data();
    while (ptr < buffer.data() + length)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;
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
                        // 简化版：假设只有一个处理器组
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

IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < m_items.size())
    {
        return m_items[index];
    }
    return nullptr;
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
    case TMI_VERSION: return L"1.6.0"; // 最终稳定版
    default: return L"";
    }
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}