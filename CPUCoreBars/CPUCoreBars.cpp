#include "CPUCoreBars.h"
#include <string>
#include <Pdh.h>
#include <chrono>

#pragma comment(lib, "pdh.lib")

// CCpuUsageItem 的声明和实现与上一版完全相同，此处省略
// ... (将上一版完整的 CCpuUsageItem 声明和实现代码粘贴到这里)
class CCpuUsageItem : public IPluginItem { public: CCpuUsageItem(int core_index, bool is_e_core); virtual ~CCpuUsageItem() = default; const wchar_t* GetItemName() const override; const wchar_t* GetItemId() const override; const wchar_t* GetItemLableText() const override; const wchar_t* GetItemValueText() const override; const wchar_t* GetItemValueSampleText() const override; bool IsCustomDraw() const override; int GetItemWidth() const override; void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override; void SetUsage(double usage); void SetIsECore(bool is_e_core); private: void DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode); int m_core_index; double m_usage = 0.0; wchar_t m_item_name[32]; wchar_t m_item_id[32]; bool m_is_e_core; };
CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core) : m_core_index(core_index), m_is_e_core(is_e_core) { swprintf_s(m_item_name, L"CPU Core %d", m_core_index); swprintf_s(m_item_id, L"cpu_core_%d", m_core_index); }
const wchar_t* CCpuUsageItem::GetItemName() const { return m_item_name; }
const wchar_t* CCpuUsageItem::GetItemId() const { return m_item_id; }
const wchar_t* CCpuUsageItem::GetItemLableText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueSampleText() const { return L""; }
bool CCpuUsageItem::IsCustomDraw() const { return true; }
int CCpuUsageItem::GetItemWidth() const { return 10; }
void CCpuUsageItem::SetUsage(double usage) { m_usage = max(0.0, min(1.0, usage)); }
void CCpuUsageItem::SetIsECore(bool is_e_core) { m_is_e_core = is_e_core; }
void CCpuUsageItem::DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode) { COLORREF icon_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0); SetTextColor(hDC, icon_color); SetBkMode(hDC, TRANSPARENT); const wchar_t* symbol = L"\u26B2"; HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol"); HGDIOBJ hOldFont = SelectObject(hDC, hFont); DrawTextW(hDC, symbol, -1, (LPRECT)&rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE); SelectObject(hDC, hOldFont); DeleteObject(hFont); }
void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) { HDC dc = (HDC)hDC; RECT rect = { x, y, x + w, y + h }; HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255)); FillRect(dc, &rect, bg_brush); DeleteObject(bg_brush); COLORREF bar_color; if (m_core_index >= 12 && m_core_index <= 19) { bar_color = RGB(217, 66, 53); } else if (m_core_index % 2 == 1) { bar_color = RGB(38, 160, 218); } else { bar_color = RGB(118, 202, 83); } if (m_usage >= 0.8) { bar_color = RGB(217, 66, 53); } else if (m_usage >= 0.5) { bar_color = RGB(246, 182, 78); } int bar_height = static_cast<int>(h * m_usage); if (bar_height > 0) { RECT bar_rect = { x, y + (h - bar_height), x + w, y + h }; HBRUSH bar_brush = CreateSolidBrush(bar_color); FillRect(dc, &bar_rect, bar_brush); DeleteObject(bar_brush); } if (m_is_e_core) { DrawECoreSymbol(dc, rect, dark_mode); } }


// =================================================================
// CCPUCoreBarsPlugin implementation (最终修复版)
// =================================================================

CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

// 构造函数现在是真正的“零初始化”
CCPUCoreBarsPlugin::CCPUCoreBarsPlugin() 
    : m_is_running(true), m_is_initialized(false)
{
    // 只启动后台线程，不做任何其他事情
    m_worker_thread = std::thread(&CCPUCoreBarsPlugin::WorkerThread, this);
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    m_is_running = false;
    if (m_worker_thread.joinable())
    {
        m_worker_thread.join();
    }
    // 安全地清理 m_items
    std::lock_guard<std::mutex> lock(m_data_mutex);
    for (auto item : m_items) delete item;
    m_items.clear();
}

void CCPUCoreBarsPlugin::WorkerThread()
{
    // --- 1. 初始化阶段 (所有工作都在后台) ---
    // 1a. 获取核心数 (第一个可能阻塞的调用)
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    int num_cores = sys_info.dwNumberOfProcessors;

    // 1b. 检测核心类型
    std::vector<BYTE> core_efficiency(num_cores, 1);
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        std::vector<char> buffer(length);
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data();
        if (GetLogicalProcessorInformationEx(RelationProcessorCore, proc_info, &length)) {
            char* ptr = buffer.data();
            while (ptr < buffer.data() + length) {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;
                if (current_info->Relationship == RelationProcessorCore) {
                    BYTE efficiency = current_info->Processor.EfficiencyClass;
                    for (int i = 0; i < current_info->Processor.GroupCount; ++i) {
                        KAFFINITY mask = current_info->Processor.GroupMask[i].Mask;
                        for (int j = 0; j < sizeof(KAFFINITY) * 8; ++j) {
                            if ((mask >> j) & 1) {
                                int logical_proc_index = j;
                                if (logical_proc_index < num_cores) {
                                    core_efficiency[logical_proc_index] = efficiency;
                                }
                            }
                        }
                    }
                }
                ptr += current_info->Size;
            }
        }
    }

    // 1c. 初始化 PDH
    PDH_HQUERY query = nullptr;
    std::vector<PDH_HCOUNTER> counters(num_cores);
    if (PdhOpenQuery(nullptr, 0, &query) != ERROR_SUCCESS) return;
    for (int i = 0; i < num_cores; ++i) {
        wchar_t counter_path[128];
        swprintf_s(counter_path, L"\\Processor(%d)\\%% Processor Time", i);
        if (PdhAddCounterW(query, counter_path, 0, &counters[i]) != ERROR_SUCCESS) {
            PdhCloseQuery(query);
            return;
        }
    }

    // 1d. **关键**: 现在安全地创建和填充监控项列表
    {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        m_cpu_usage_buffer.resize(num_cores, 0.0);
        for (int i = 0; i < num_cores; ++i) {
            bool is_e_core = (core_efficiency[i] == 0);
            m_items.push_back(new CCpuUsageItem(i, is_e_core));
        }
    }

    // 1e. 标记初始化完成
    m_is_initialized = true;
    PdhCollectQueryData(query);

    // --- 2. 数据采集主循环 ---
    while (m_is_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!m_is_running) break;
        if (PdhCollectQueryData(query) == ERROR_SUCCESS) {
            std::lock_guard<std::mutex> lock(m_data_mutex);
            for (int i = 0; i < num_cores; ++i) {
                PDH_FMT_COUNTERVALUE value;
                if (PdhGetFormattedCounterValue(counters[i], PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) {
                    m_cpu_usage_buffer[i] = value.doubleValue / 100.0;
                } else {
                    m_cpu_usage_buffer[i] = 0.0;
                }
            }
        }
    }
    PdhCloseQuery(query);
}

void CCPUCoreBarsPlugin::DataRequired()
{
    if (!m_is_initialized) return;
    std::lock_guard<std::mutex> lock(m_data_mutex);
    for (size_t i = 0; i < m_items.size(); ++i)
    {
        m_items[i]->SetUsage(m_cpu_usage_buffer[i]);
    }
}

// GetItem 现在必须使用互斥锁来安全地访问 m_items
IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    std::lock_guard<std::mutex> lock(m_data_mutex);
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
    case TMI_VERSION: return L"1.9.0"; // 最终稳定版
    default: return L"";
    }
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}