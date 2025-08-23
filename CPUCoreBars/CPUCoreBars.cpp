// CPUCoreBars/CPUCoreBars.cpp - 性能优化版本
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>

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
    : m_core_index(core_index), m_is_e_core(is_e_core)
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
    // 归还GDI对象到对象池
    auto& pool = GDIObjectPool::Instance();
    if (m_cachedBgBrush && m_pooled_bg_color != 0xFFFFFFFF) {
        pool.ReturnBrush(m_pooled_bg_color, m_cachedBgBrush);
    }
    if (m_cachedBarBrush && m_pooled_bar_color != 0xFFFFFFFF) {
        pool.ReturnBrush(m_pooled_bar_color, m_cachedBarBrush);
    }
    
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
    double new_usage = max(0.0, min(1.0, usage));
    
    // 绘图性能优化：只有当使用率变化足够大时才标记需要重绘
    if (abs(new_usage - m_last_drawn_usage) > 0.01) {  // 1% 的阈值
        m_needs_redraw = true;
    }
    
    m_usage = new_usage;
}

void CCpuUsageItem::DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode)
{
    COLORREF icon_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    SetTextColor(hDC, icon_color);
    SetBkMode(hDC, TRANSPARENT);
    const wchar_t* symbol = L"\u2618";
    
    // 使用缓存的静态字体
    if (s_symbolFont) LIKELY {
        HGDIOBJ hOldFont = SelectObject(hDC, s_symbolFont);
        DrawTextW(hDC, symbol, -1, (LPRECT)&rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hDC, hOldFont);
    }
}

void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };
    
    // 绘图性能优化：检查是否需要重绘
    bool rect_changed = (rect.left != m_lastRect.left || rect.top != m_lastRect.top || 
                        rect.right != m_lastRect.right || rect.bottom != m_lastRect.bottom);
    bool dark_mode_changed = (dark_mode != m_lastDarkMode);
    
    if (!m_needs_redraw && !rect_changed && !dark_mode_changed) {
        return;  // 跳过重绘
    }
    
    // 使用对象池获取背景画刷
    COLORREF bg_color = dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255);
    auto& pool = GDIObjectPool::Instance();
    
    if (!m_cachedBgBrush || m_lastBgColor != bg_color || dark_mode_changed) {
        // 归还旧画刷到对象池
        if (m_cachedBgBrush && m_pooled_bg_color != 0xFFFFFFFF) {
            pool.ReturnBrush(m_pooled_bg_color, m_cachedBgBrush);
        }
        
        // 从对象池获取新画刷
        m_cachedBgBrush = pool.GetBrush(bg_color);
        m_lastBgColor = bg_color;
        m_pooled_bg_color = bg_color;
    }
    
    FillRect(dc, &rect, m_cachedBgBrush);

    // 计算条形图颜色（使用内联优化函数）
    COLORREF bar_color = CalculateBarColor();
    
    // 使用对象池获取条形画刷
    if (!m_cachedBarBrush || m_lastBarColor != bar_color) {
        // 归还旧画刷到对象池
        if (m_cachedBarBrush && m_pooled_bar_color != 0xFFFFFFFF) {
            pool.ReturnBrush(m_pooled_bar_color, m_cachedBarBrush);
        }
        
        // 从对象池获取新画刷
        m_cachedBarBrush = pool.GetBrush(bar_color);
        m_lastBarColor = bar_color;
        m_pooled_bar_color = bar_color;
    }

    // 编译器优化：使用整数运算代替浮点
    int bar_height = static_cast<int>(h * m_usage);
    if (bar_height > 0) LIKELY {
        RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
        FillRect(dc, &bar_rect, m_cachedBarBrush);
    }

    if (m_is_e_core) UNLIKELY {
        DrawECoreSymbol(dc, rect, dark_mode);
    }
    
    // 更新缓存状态
    m_needs_redraw = false;
    m_last_drawn_usage = m_usage;
    m_lastRect = rect;
    m_lastDarkMode = dark_mode;
}


// =================================================================
// CNvidiaMonitorItem implementation - 优化版本
// =================================================================
CNvidiaMonitorItem::CNvidiaMonitorItem()
{
    wcscpy_s(m_value_text, L"N/A");
    m_value_hash = HashString(m_value_text);

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

void CNvidiaMonitorItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    
    // 绘图性能优化：检查是否需要重绘
    RECT rect = { x, y, x + w, y + h };
    bool rect_changed = (rect.left != m_lastRect.left || rect.top != m_lastRect.top || 
                        rect.right != m_lastRect.right || rect.bottom != m_lastRect.bottom);
    bool error_changed = (m_has_system_error != m_last_error_status);
    bool dark_mode_changed = (dark_mode != m_last_dark_mode);
    
    if (!m_needs_redraw && !rect_changed && !error_changed && !dark_mode_changed) {
        return;  // 跳过重绘
    }
    
    const int LEFT_MARGIN = 2;
    int icon_size = min(w, h) - 2;
    int icon_y_offset = (h - icon_size) / 2;

    // 缓存Graphics对象
    if (!m_cachedGraphics || m_lastHdc != dc) UNLIKELY {
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
    
    // 更新缓存状态
    m_needs_redraw = false;
    m_last_error_status = m_has_system_error;
    m_last_dark_mode = dark_mode;
    m_lastRect = rect;
}

void CNvidiaMonitorItem::SetValue(const wchar_t* value)
{
    // 字符串操作优化：只有在值真正改变时才更新
    uint32_t new_hash = HashString(value);
    if (new_hash != m_value_hash) {
        wcscpy_s(m_value_text, value);
        m_value_hash = new_hash;
        m_needs_redraw = true;
    }
}

void CNvidiaMonitorItem::SetSystemErrorStatus(bool has_error)
{
    if (m_has_system_error != has_error) {
        m_has_system_error = has_error;
        m_needs_redraw = true;
    }
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
{
    // 预分配事件缓冲区 - 内存管理优化
    m_event_buffer.reserve(16384);  // 预分配16KB
    m_event_handles.reserve(512);   // 预分配512个句柄
    
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_num_cores = min((int)sys_info.dwNumberOfProcessors, MAX_CORES);
    
    DetectCoreTypes();
    
    // 数据结构优化：同时填充数组和vector
    for (int i = 0; i < m_num_cores; ++i) {
        bool is_e_core = (m_core_efficiency[i] == 0);
        CCpuUsageItem* item = new CCpuUsageItem(i, is_e_core);
        m_items_array[i] = item;
        m_items.push_back(item);
    }
    
    // 初始化字符串缓存
    StringCache::Instance().InitializeCounterPaths(m_num_cores);
    
    if (PdhOpenQuery(nullptr, 0, &m_query) == ERROR_SUCCESS) {
        m_counters.resize(m_num_cores);
        for (int i = 0; i < m_num_cores; ++i) {
            // 字符串操作优化：使用缓存的路径
            const wchar_t* counter_path = StringCache::Instance().GetCounterPath(i);
            if (counter_path) {
                PdhAddCounterW(m_query, counter_path, 0, &m_counters[i]);
            }
        }
        PdhCollectQueryData(m_query);
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
        if (reasons & nvmlClocksThrottleReasonHwThermalSlowdown) { 
            m_gpu_item->SetValue(L"过热"); 
        }
        else if (reasons & nvmlClocksThrottleReasonSwThermalSlowdown) { 
            m_gpu_item->SetValue(L"过热"); 
        }
        else if (reasons & nvmlClocksThrottleReasonHwPowerBrakeSlowdown) { 
            m_gpu_item->SetValue(L"功耗"); 
        }
        else if (reasons & nvmlClocksThrottleReasonSwPowerCap) { 
            m_gpu_item->SetValue(L"功耗"); 
        }
        else if (reasons & nvmlClocksThrottleReasonGpuIdle) { 
            m_gpu_item->SetValue(L"空闲"); 
        }
        else if (reasons == nvmlClocksThrottleReasonApplicationsClocksSetting) { 
            m_gpu_item->SetValue(L"无限"); 
        }
        else { 
            m_gpu_item->SetValue(L"无"); 
        }
    } else {
        m_gpu_item->SetValue(L"错误");
    }
}

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    if (!m_query) return;
    if (PdhCollectQueryData(m_query) == ERROR_SUCCESS) {
        for (int i = 0; i < m_num_cores; ++i) {
            PDH_FMT_COUNTERVALUE value;
            if (PdhGetFormattedCounterValue(m_counters[i], PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) {
                m_items[i]->SetUsage(value.doubleValue / 100.0);
            } else {
                m_items[i]->SetUsage(0.0);
            }
        }
    }
}

void CCPUCoreBarsPlugin::DetectCoreTypes()
{
    // 初始化所有核心为P核心（效率等级1）
    for (int i = 0; i < m_num_cores; ++i) {
        m_core_efficiency[i] = 1;
    }
    
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;

    // 使用预分配的缓冲区或动态分配
    if (m_event_buffer.size() < length) {
        m_event_buffer.resize(length);
    }
    
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc_info = 
        (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)m_event_buffer.data();
    
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, proc_info, &length)) return;

    char* ptr = m_event_buffer.data();
    while (ptr < m_event_buffer.data() + length) {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current_info = 
            (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;
        
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

// 大幅优化的事件日志查询函数
DWORD CCPUCoreBarsPlugin::QueryEventLogCountOptimized(const wchar_t* query)
{
    EVT_HANDLE hResults = EvtQuery(NULL, L"System", query, 
        EvtQueryChannelPath | EvtQueryReverseDirection | EvtQueryTolerateQueryErrors);
    if (hResults == NULL) return 0;

    DWORD total_count = 0;
    
    // 确保事件句柄向量有足够的空间
    if (m_event_handles.size() < 256) {
        m_event_handles.resize(256);
    }
    
    EVT_HANDLE* hEvents = m_event_handles.data();
    DWORD returned = 0;
    
    // 使用更大的批处理大小提高效率
    while (EvtNext(hResults, 256, hEvents, 1000, 0, &returned)) {
        total_count += returned;
        // 批量关闭事件句柄
        for (DWORD i = 0; i < returned; i++) {
            EvtClose(hEvents[i]);
        }
        
        // 如果返回的事件少于请求的数量，说明已经到末尾
        if (returned < 256) break;
    }
    
    EvtClose(hResults);
    return total_count;
}

void CCPUCoreBarsPlugin::UpdateWheaErrorCount()
{
    // 使用缓存的查询字符串
    const wchar_t* query = StringCache::Instance().GetWHEAQuery();
    m_cached_whea_count = QueryEventLogCountOptimized(query);
    m_whea_error_count = m_cached_whea_count;
}

void CCPUCoreBarsPlugin::UpdateNvlddmkmErrorCount()
{
    // 使用缓存的查询字符串
    const wchar_t* query = StringCache::Instance().GetNvlddmkmQuery();
    m_cached_nvlddmkm_count = QueryEventLogCountOptimized(query);
    m_nvlddmkm_error_count = m_cached_nvlddmkm_count;
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}
                