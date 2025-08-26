// CPUCoreBars/CPUCoreBars.cpp - 性能优化版本 + 温度监控
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>
#include <winevt.h>
#include <algorithm>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// =================================================================
// CCpuTemperatureItem implementation - 新增温度监控项
// =================================================================
CCpuTemperatureItem::CCpuTemperatureItem()
{
    wcscpy_s(m_temp_text, L"--°C");
    
    // 计算显示宽度
    HDC hdc = GetDC(NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    SIZE temp_size;
    GetTextExtentPoint32W(hdc, L"100°C", 5, &temp_size);
    m_width = temp_size.cx + 4; // 添加少量边距
    
    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);
}

CCpuTemperatureItem::~CCpuTemperatureItem()
{
}

const wchar_t* CCpuTemperatureItem::GetItemName() const
{
    return L"CPU温度";
}

const wchar_t* CCpuTemperatureItem::GetItemId() const
{
    return L"cpu_max_temperature";
}

const wchar_t* CCpuTemperatureItem::GetItemLableText() const
{
    return L"";
}

const wchar_t* CCpuTemperatureItem::GetItemValueText() const
{
    return m_temp_text;
}

const wchar_t* CCpuTemperatureItem::GetItemValueSampleText() const
{
    return L"100°C";
}

bool CCpuTemperatureItem::IsCustomDraw() const
{
    return true;
}

int CCpuTemperatureItem::GetItemWidth() const
{
    return m_width;
}

inline COLORREF CCpuTemperatureItem::CalculateTextColor(bool dark_mode) const
{
    // 根据温度确定颜色
    if (m_max_temperature >= 85.0) return RGB(217, 66, 53);   // 红色 - 危险温度
    if (m_max_temperature >= 70.0) return RGB(246, 182, 78);  // 橙色 - 警告温度
    if (m_max_temperature >= 50.0) return RGB(118, 202, 83);  // 绿色 - 正常温度
    
    // 默认颜色（未知或低温）
    return dark_mode ? RGB(200, 200, 200) : RGB(100, 100, 100);
}

void CCpuTemperatureItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    
    RECT text_rect = { x, y, x + w, y + h };
    COLORREF temp_color = CalculateTextColor(dark_mode);
    
    SetTextColor(dc, temp_color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, m_temp_text, -1, &text_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void CCpuTemperatureItem::SetMaxTemperature(double temp)
{
    m_max_temperature = temp;
    if (temp > 0) {
        swprintf_s(m_temp_text, L"%.0f°C", temp);
    } else {
        wcscpy_s(m_temp_text, L"--°C");
    }
}

void CCpuTemperatureItem::SetCoreTemperatures(const std::vector<double>& temps)
{
    m_core_temperatures = temps;
}

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
// CCPUCoreBarsPlugin implementation - 优化版本 + 温度监控
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
    m_core_temperatures.resize(m_num_cores, 0.0);
    
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
    
    InitNVML();
    InitTemperatureMonitoring();
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    if (m_query) PdhCloseQuery(m_query);
    for (auto item : m_items) delete item;
    if (m_gpu_item) delete m_gpu_item;
    if (m_temp_item) delete m_temp_item;
    ShutdownNVML();
    ShutdownWinRing0();
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
    if (index == m_num_cores + 1 && m_temp_item != nullptr) {
        return m_temp_item;
    }
    return nullptr;
}

void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
    UpdateCpuTemperatures();
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
    case TMI_NAME: return L"性能/温度/错误监控";
    case TMI_DESCRIPTION: return L"CPU核心条形图/温度监控/GPU受限&错误/WHEA错误";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"4.0.0";
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::InitTemperatureMonitoring()
{
    // 尝试初始化WinRing0驱动
    if (InitializeWinRing0()) {
        m_temp_item = new CCpuTemperatureItem();
    }
}

bool CCPUCoreBarsPlugin::InitializeWinRing0()
{
    // 尝试加载WinRing0x64.dll或WinRing0.dll
    m_winring0_dll = LoadLibraryW(L"WinRing0x64.dll");
    if (!m_winring0_dll) {
        m_winring0_dll = LoadLibraryW(L"WinRing0.dll");
    }
    
    if (!m_winring0_dll) {
        return false;
    }
    
    // 获取函数指针
    pfn_InitializeOls = (InitializeOlsType)GetProcAddress(m_winring0_dll, "InitializeOls");
    pfn_DeinitializeOls = (DeinitializeOlsType)GetProcAddress(m_winring0_dll, "DeinitializeOls");
    pfn_Rdmsr = (RdmsrType)GetProcAddress(m_winring0_dll, "Rdmsr");
    
    if (!pfn_InitializeOls || !pfn_DeinitializeOls || !pfn_Rdmsr) {
        FreeLibrary(m_winring0_dll);
        m_winring0_dll = nullptr;
        return false;
    }
    
    // 初始化驱动
    if (!pfn_InitializeOls()) {
        FreeLibrary(m_winring0_dll);
        m_winring0_dll = nullptr;
        return false;
    }
    
    m_winring0_initialized = true;
    return true;
}

void CCPUCoreBarsPlugin::ShutdownWinRing0()
{
    if (m_winring0_initialized && pfn_DeinitializeOls) {
        pfn_DeinitializeOls();
    }
    if (m_winring0_dll) {
        FreeLibrary(m_winring0_dll);
        m_winring0_dll = nullptr;
    }
    m_winring0_initialized = false;
}

void CCPUCoreBarsPlugin::UpdateCpuTemperatures()
{
    if (!m_winring0_initialized || !m_temp_item) {
        return;
    }
    
    double max_temp = 0.0;
    std::vector<double> temps;
    temps.reserve(m_num_cores);
    
    // 读取每个核心的温度
    for (int i = 0; i < m_num_cores; ++i) {
        double core_temp = ReadMSRTemperature(i);
        temps.push_back(core_temp);
        if (core_temp > max_temp) {
            max_temp = core_temp;
        }
    }
    
    // 更新温度监控项
    m_temp_item->SetMaxTemperature(max_temp);
    m_temp_item->SetCoreTemperatures(temps);
    m_core_temperatures = temps;
}

double CCPUCoreBarsPlugin::ReadMSRTemperature(int core_id)
{
    if (!m_winring0_initialized || !pfn_Rdmsr) {
        return 0.0;
    }
    
    // Intel CPU温度读取 - 使用MSR 0x1B1 (IA32_PACKAGE_THERM_STATUS)
    // 对于单独的核心，使用 MSR 0x1A2 (IA32_THERM_STATUS)
    DWORD eax, edx;
    
    // 先尝试读取包温度
    if (pfn_Rdmsr(0x1B1, &eax, &edx)) {
        // 提取温度数据 (位22:16)
        DWORD temp_data = (eax >> 16) & 0x7F;
        if (temp_data > 0) {
            // Intel CPU: TCC_ACTIVATION_TEMP - Digital_Readout
            // 通常TCC_ACTIVATION_TEMP为100°C
            return 100.0 - (double)temp_data;
        }
    }
    
    // 如果包温度读取失败，尝试读取核心温度
    if (pfn_Rdmsr(0x1A2, &eax, &edx)) {
        DWORD temp_data = (eax >> 16) & 0x7F;
        if (temp_data > 0) {
            return 100.0 - (double)temp_data;
        }
    }
    
    return 0.0;
}