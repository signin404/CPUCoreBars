// CPUCoreBars/CPUCoreBars.cpp - 性能优化版本
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>
#include <winevt.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// =================================================================
// CCpuUsageItem implementation - 优化版本
// =================================================================

// 静态成员变量定义
HFONT CCpuUsageItem::s_symbolFont = nullptr;
int CCpuUsageItem::s_fontRefCount = 0;

CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core) 
    : m_core_index(core_index), m_is_e_core(is_e_core),
      m_cachedBgBrush(nullptr), m_cachedBarBrush(nullptr),
      m_lastBgColor(0), m_lastBarColor(0), m_lastDarkMode(false)
{
    // 初始化静态字体（只创建一次）
    if (s_symbolFont == nullptr) {
        s_symbolFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
    }
    s_fontRefCount++;
    
    swprintf_s(m_item_name, L"CPU Core %d", m_core_index);
    swprintf_s(m_item_id, L"cpu_core_%d", m_core_index);
}

CCpuUsageItem::~CCpuUsageItem()
{
    // 清理缓存的画刷
    if (m_cachedBgBrush) DeleteObject(m_cachedBgBrush);
    if (m_cachedBarBrush) DeleteObject(m_cachedBarBrush);
    
    // 减少字体引用计数
    s_fontRefCount--;
    if (s_fontRefCount == 0 && s_symbolFont) {
        DeleteObject(s_symbolFont);
        s_symbolFont = nullptr;
    }
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
    return 8;
}

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = max(0.0, min(1.0, usage));
}

inline COLORREF CCpuUsageItem::CalculateBarColor() const
{
    // 高使用率优先显示（性能优化：重新排列条件优先级）
    if (m_usage >= 0.9) return RGB(217, 66, 53);  // 红色
    if (m_usage >= 0.5) return RGB(246, 182, 78); // 橙色
    
    // 基于核心索引的颜色
    if (m_core_index >= 12 && m_core_index <= 19) {
        return RGB(217, 66, 53);  // 红色
    }
    return (m_core_index % 2 == 1) ? RGB(38, 160, 218) : RGB(118, 202, 83);
}

void CCpuUsageItem::DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode)
{
    COLORREF icon_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    SetTextColor(hDC, icon_color);
    SetBkMode(hDC, TRANSPARENT);
    const wchar_t* symbol = L"\u2618";
    
    // 使用缓存的静态字体
    if (s_symbolFont) {
        HGDIOBJ hOldFont = SelectObject(hDC, s_symbolFont);
        DrawTextW(hDC, symbol, -1, (LPRECT)&rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hDC, hOldFont);
    }
}

void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };
    
    // 使用缓存的背景画刷
    COLORREF bg_color = dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255);
    if (!m_cachedBgBrush || m_lastBgColor != bg_color || m_lastDarkMode != dark_mode) {
        if (m_cachedBgBrush) DeleteObject(m_cachedBgBrush);
        m_cachedBgBrush = CreateSolidBrush(bg_color);
        m_lastBgColor = bg_color;
        m_lastDarkMode = dark_mode;
    }
    FillRect(dc, &rect, m_cachedBgBrush);

    // 计算条形图颜色
    COLORREF bar_color = CalculateBarColor();
    
    // 使用缓存的条形画刷
    if (!m_cachedBarBrush || m_lastBarColor != bar_color) {
        if (m_cachedBarBrush) DeleteObject(m_cachedBarBrush);
        m_cachedBarBrush = CreateSolidBrush(bar_color);
        m_lastBarColor = bar_color;
    }

    int bar_height = static_cast<int>(h * m_usage);
    if (bar_height > 0) {
        RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
        FillRect(dc, &bar_rect, m_cachedBarBrush);
    }

    if (m_is_e_core) {
        DrawECoreSymbol(dc, rect, dark_mode);
    }
}


// =================================================================
// CNvidiaMonitorItem implementation - 优化版本
// =================================================================
CNvidiaMonitorItem::CNvidiaMonitorItem()
    : m_cachedGraphics(nullptr), m_lastHdc(nullptr)
{
    wcscpy_s(m_value_text, L"N/A");

    HDC hdc = GetDC(NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    const wchar_t* sample_value = GetItemValueSampleText();
    SIZE value_size;
    GetTextExtentPoint32W(hdc, sample_value, (int)wcslen(sample_value), &value_size);
    
    m_width = 18 + 4 + value_size.cx;

    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);
}

CNvidiaMonitorItem::~CNvidiaMonitorItem()
{
    if (m_cachedGraphics) {
        delete m_cachedGraphics;
    }
}

const wchar_t* CNvidiaMonitorItem::GetItemName() const
{
    return L"GPU/WHEA";
}

const wchar_t* CNvidiaMonitorItem::GetItemId() const
{
    return L"gpu_system_status";
}

const wchar_t* CNvidiaMonitorItem::GetItemLableText() const
{
    return L"";
}

const wchar_t* CNvidiaMonitorItem::GetItemValueText() const
{
    return m_value_text;
}

const wchar_t* CNvidiaMonitorItem::GetItemValueSampleText() const
{
    return L"宽度";
}

bool CNvidiaMonitorItem::IsCustomDraw() const
{
    return true;
}

int CNvidiaMonitorItem::GetItemWidth() const
{
    return m_width;
}

inline COLORREF CNvidiaMonitorItem::CalculateTextColor(bool dark_mode) const 
{
    const wchar_t* current_value = GetItemValueText();
    if (wcscmp(current_value, L"过热") == 0) return RGB(217, 66, 53);
    if (wcscmp(current_value, L"功耗") == 0) return RGB(246, 182, 78);
    return dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
}

void CNvidiaMonitorItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    
    const int LEFT_MARGIN = 2;
    int icon_size = min(w, h) - 2;
    int icon_y_offset = (h - icon_size) / 2;

    // 缓存Graphics对象
    if (!m_cachedGraphics || m_lastHdc != dc) {
        if (m_cachedGraphics) delete m_cachedGraphics;
        m_cachedGraphics = new Graphics(dc);
        m_cachedGraphics->SetSmoothingMode(SmoothingModeAntiAlias);
        m_lastHdc = dc;
    }
    
    // 使用缓存的Graphics对象绘制圆形
    Color circle_color = m_has_system_error ? Color(217, 66, 53) : Color(118, 202, 83);
    SolidBrush circle_brush(circle_color);
    m_cachedGraphics->FillEllipse(&circle_brush, 
        x + LEFT_MARGIN, y + icon_y_offset, icon_size, icon_size);

    // --- 绘制文本 ---
    RECT text_rect = { x + LEFT_MARGIN + icon_size + 4, y, x + w, y + h };
    COLORREF value_text_color = CalculateTextColor(dark_mode);
    
    SetTextColor(dc, value_text_color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, GetItemValueText(), -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void CNvidiaMonitorItem::SetValue(const wchar_t* value)
{
    wcscpy_s(m_value_text, value);
}

void CNvidiaMonitorItem::SetSystemErrorStatus(bool has_error)
{
    m_has_system_error = has_error;
}


// =================================================================
// CCPUCoreBarsPlugin implementation - 优化版本
// =================================================================
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin()
    : m_cached_whea_count(0), m_cached_nvlddmkm_count(0), m_last_error_check_time(0)
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

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
        // *** 优化：使用单个通配符查询替换循环添加 ***
        PdhAddCounterW(m_query, L"\\Processor(*)\\%% Processor Time", 0, &m_cpu_usage_counter);
        PdhCollectQueryData(m_query); // 第一次收集以初始化
    }
    InitNVML();
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    if (m_query) PdhCloseQuery(m_query);
    for (auto item : m_items) delete item;
    if (m_gpu_item) delete m_gpu_item;
    ShutdownNVML();
    GdiplusShutdown(m_gdiplusToken);
}

IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    if (index < m_num_cores) {
        return m_items[index];
    }
    if (index == m_num_cores && m_gpu_item != nullptr) {
        return m_gpu_item;
    }
    return nullptr;
}

void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
    UpdateGpuLimitReason();
    
    // 减少事件日志查询频率 - 30秒检查一次
    DWORD current_time = GetTickCount();
    if (current_time - m_last_error_check_time > ERROR_CHECK_INTERVAL_MS) {
        UpdateWheaErrorCount();
        UpdateNvlddmkmErrorCount();
        m_last_error_check_time = current_time;
    }
    
    if (m_gpu_item) {
        bool has_error = (m_cached_whea_count > 0 || m_cached_nvlddmkm_count > 0);
        m_gpu_item->SetSystemErrorStatus(has_error);
    }
}

const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index) {
    case TMI_NAME: return L"性能/错误监控";
    case TMI_DESCRIPTION: return L"CPU核心条形图/GPU受限&错误/WHEA错误";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"3.6.0";
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::InitNVML()
{
    m_nvml_dll = LoadLibrary(L"nvml.dll");
    if (!m_nvml_dll) return;

    pfn_nvmlInit = (decltype(pfn_nvmlInit))GetProcAddress(m_nvml_dll, "nvmlInit_v2");
    pfn_nvmlShutdown = (decltype(pfn_nvmlShutdown))GetProcAddress(m_nvml_dll, "nvmlShutdown");
    pfn_nvmlDeviceGetHandleByIndex = (decltype(pfn_nvmlDeviceGetHandleByIndex))GetProcAddress(m_nvml_dll, "nvmlDeviceGetHandleByIndex_v2");
    pfn_nvmlDeviceGetCurrentClocksThrottleReasons = (decltype(pfn_nvmlDeviceGetCurrentClocksThrottleReasons))GetProcAddress(m_nvml_dll, "nvmlDeviceGetCurrentClocksThrottleReasons");

    if (!pfn_nvmlInit || !pfn_nvmlShutdown || !pfn_nvmlDeviceGetHandleByIndex || !pfn_nvmlDeviceGetCurrentClocksThrottleReasons) {
        ShutdownNVML();
        return;
    }
    if (pfn_nvmlInit() != NVML_SUCCESS) {
        ShutdownNVML();
        return;
    }
    if (pfn_nvmlDeviceGetHandleByIndex(0, &m_nvml_device) != NVML_SUCCESS) {
        ShutdownNVML();
        return;
    }
    m_nvml_initialized = true;
    m_gpu_item = new CNvidiaMonitorItem();
}

void CCPUCoreBarsPlugin::ShutdownNVML()
{
    if (m_nvml_initialized && pfn_nvmlShutdown) {
        pfn_nvmlShutdown();
    }
    if (m_nvml_dll) {
        FreeLibrary(m_nvml_dll);
    }
    m_nvml_initialized = false;
    m_nvml_dll = nullptr;
}

void CCPUCoreBarsPlugin::UpdateGpuLimitReason()
{
    if (!m_nvml_initialized || !m_gpu_item) return;

    unsigned long long reasons = 0;
    if (pfn_nvmlDeviceGetCurrentClocksThrottleReasons(m_nvml_device, &reasons) == NVML_SUCCESS) {
        if (reasons & nvmlClocksThrottleReasonHwThermalSlowdown) { m_gpu_item->SetValue(L"过热"); }
        else if (reasons & nvmlClocksThrottleReasonSwThermalSlowdown) { m_gpu_item->SetValue(L"过热"); }
        else if (reasons & nvmlClocksThrottleReasonHwPowerBrakeSlowdown) { m_gpu_item->SetValue(L"功耗"); }
        else if (reasons & nvmlClocksThrottleReasonSwPowerCap) { m_gpu_item->SetValue(L"功耗"); }
        else if (reasons & nvmlClocksThrottleReasonGpuIdle) { m_gpu_item->SetValue(L"空闲"); }
        else if (reasons == nvmlClocksThrottleReasonApplicationsClocksSetting) { m_gpu_item->SetValue(L"无限"); }
        else { m_gpu_item->SetValue(L"无"); }
    } else {
        m_gpu_item->SetValue(L"错误");
    }
}

// *** 优化：使用 PdhGetFormattedCounterArray 进行批量查询 ***
void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    if (!m_query || !m_cpu_usage_counter) return;
    
    if (PdhCollectQueryData(m_query) != ERROR_SUCCESS) return;

    DWORD buffer_size = 0;
    DWORD item_count = 0;
    
    // 第一次调用，获取所需的缓冲区大小和项目数
    PDH_STATUS status = PdhGetFormattedCounterArrayW(m_cpu_usage_counter, PDH_FMT_DOUBLE, &buffer_size, &item_count, nullptr);

    if (status == PDH_MORE_DATA) {
        std::vector<BYTE> buffer(buffer_size);
        PDH_FMT_COUNTERVALUE_ITEM_W* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());

        // 第二次调用，获取实际数据
        status = PdhGetFormattedCounterArrayW(m_cpu_usage_counter, PDH_FMT_DOUBLE, &buffer_size, &item_count, items);
        if (status == ERROR_SUCCESS) {
            for (DWORD i = 0; i < item_count; ++i) {
                // `szName` 包含核心的索引号，例如 "0", "1", ...
                int core_index = _wtoi(items[i].szName);
                if (core_index >= 0 && core_index < m_num_cores) {
                    m_items[core_index]->SetUsage(items[i].FmtValue.doubleValue / 100.0);
                }
            }
        }
    } 
    else if (status != ERROR_SUCCESS) {
        // 如果发生错误，或没有数据，则将所有核心使用率设为0
        for (int i = 0; i < m_num_cores; ++i) {
            m_items[i]->SetUsage(0.0);
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
    while (ptr < buffer.data() + length) {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;
        if (current_info->Relationship == RelationProcessorCore) {
            BYTE efficiency = current_info->Processor.EfficiencyClass;
            for (int i = 0; i < current_info->Processor.GroupCount; ++i) {
                KAFFINITY mask = current_info->Processor.GroupMask[i].Mask;
                for (int j = 0; j < sizeof(KAFFINITY) * 8; ++j) {
                    if ((mask >> j) & 1) {
                        int logical_proc_index = j;
                        if (logical_proc_index < m_num_cores) {
                            m_core_efficiency[logical_proc_index] = efficiency;
                        }
                    }
                }
            }
        }
        ptr += current_info->Size;
    }
}

// 优化的事件日志查询函数
DWORD CCPUCoreBarsPlugin::QueryEventLogCount(LPCWSTR provider_name)
{
    // 使用更高效的查询字符串
    wchar_t query[256];
    swprintf_s(query, 
        L"*[System[Provider[@Name='%s'] and TimeCreated[timediff(@SystemTime) <= 86400000]]]",
        provider_name);
        
    EVT_HANDLE hResults = EvtQuery(NULL, L"System", query, 
        EvtQueryChannelPath | EvtQueryReverseDirection);
    if (hResults == NULL) return 0;

    DWORD total_count = 0;
    EVT_HANDLE hEvents[256]; // 增大批处理大小
    DWORD returned = 0;
    
    while (EvtNext(hResults, ARRAYSIZE(hEvents), hEvents, 1000, 0, &returned)) {
        total_count += returned;
        // 批量关闭事件句柄
        for (DWORD i = 0; i < returned; i++) {
            EvtClose(hEvents[i]);
        }
    }
    
    EvtClose(hResults);
    return total_count;
}

void CCPUCoreBarsPlugin::UpdateWheaErrorCount()
{
    m_cached_whea_count = QueryEventLogCount(L"WHEA-Logger");
    m_whea_error_count = m_cached_whea_count;
}

void CCPUCoreBarsPlugin::UpdateNvlddmkmErrorCount()
{
    m_cached_nvlddmkm_count = QueryEventLogCount(L"nvlddmkm");
    m_nvlddmkm_error_count = m_cached_nvlddmkm_count;
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}