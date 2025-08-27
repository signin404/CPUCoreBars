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

HFONT CCpuUsageItem::s_symbolFont = nullptr;
int CCpuUsageItem::s_fontRefCount = 0;

CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core) 
    : m_core_index(core_index), m_is_e_core(is_e_core),
      m_cachedBgBrush(nullptr), m_cachedBarBrush(nullptr),
      m_lastBgColor(0), m_lastBarColor(0), m_lastDarkMode(false)
{
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
    if (m_cachedBgBrush) DeleteObject(m_cachedBgBrush);
    if (m_cachedBarBrush) DeleteObject(m_cachedBarBrush);
    
    s_fontRefCount--;
    if (s_fontRefCount == 0 && s_symbolFont) {
        DeleteObject(s_symbolFont);
        s_symbolFont = nullptr;
    }
}

const wchar_t* CCpuUsageItem::GetItemName() const { return m_item_name; }
const wchar_t* CCpuUsageItem::GetItemId() const { return m_item_id; }
const wchar_t* CCpuUsageItem::GetItemLableText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueSampleText() const { return L""; }
bool CCpuUsageItem::IsCustomDraw() const { return true; }
int CCpuUsageItem::GetItemWidth() const { return 8; }

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = max(0.0, min(1.0, usage));
}

inline COLORREF CCpuUsageItem::CalculateBarColor() const
{
    if (m_usage >= 0.9) return RGB(217, 66, 53);
    if (m_usage >= 0.5) return RGB(246, 182, 78);
    if (m_core_index >= 12 && m_core_index <= 19) return RGB(217, 66, 53);
    return (m_core_index % 2 == 1) ? RGB(38, 160, 218) : RGB(118, 202, 83);
}

void CCpuUsageItem::DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode)
{
    COLORREF icon_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    SetTextColor(hDC, icon_color);
    SetBkMode(hDC, TRANSPARENT);
    const wchar_t* symbol = L"\u2618";
    
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
    
    COLORREF bg_color = dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255);
    if (!m_cachedBgBrush || m_lastBgColor != bg_color || m_lastDarkMode != dark_mode) {
        if (m_cachedBgBrush) DeleteObject(m_cachedBgBrush);
        m_cachedBgBrush = CreateSolidBrush(bg_color);
        m_lastBgColor = bg_color;
        m_lastDarkMode = dark_mode;
    }
    FillRect(dc, &rect, m_cachedBgBrush);

    COLORREF bar_color = CalculateBarColor();
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
// CNvidiaMonitorItem implementation
// =================================================================
CNvidiaMonitorItem::CNvidiaMonitorItem() : m_cachedGraphics(nullptr), m_lastHdc(nullptr)
{
    wcscpy_s(m_value_text, L"N/A");
    HDC hdc = GetDC(NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SIZE value_size;
    GetTextExtentPoint32W(hdc, GetItemValueSampleText(), (int)wcslen(GetItemValueSampleText()), &value_size);
    m_width = 18 + 4 + value_size.cx;
    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);
}

CNvidiaMonitorItem::~CNvidiaMonitorItem() { if (m_cachedGraphics) delete m_cachedGraphics; }
const wchar_t* CNvidiaMonitorItem::GetItemName() const { return L"GPU/WHEA"; }
const wchar_t* CNvidiaMonitorItem::GetItemId() const { return L"gpu_system_status"; }
const wchar_t* CNvidiaMonitorItem::GetItemLableText() const { return L""; }
const wchar_t* CNvidiaMonitorItem::GetItemValueText() const { return m_value_text; }
const wchar_t* CNvidiaMonitorItem::GetItemValueSampleText() const { return L"宽度"; }
bool CNvidiaMonitorItem::IsCustomDraw() const { return true; }
int CNvidiaMonitorItem::GetItemWidth() const { return m_width; }

inline COLORREF CNvidiaMonitorItem::CalculateTextColor(bool dark_mode) const 
{
    if (wcscmp(m_value_text, L"过热") == 0) return RGB(217, 66, 53);
    if (wcscmp(m_value_text, L"功耗") == 0) return RGB(246, 182, 78);
    return dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
}

void CNvidiaMonitorItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    const int LEFT_MARGIN = 2;
    int icon_size = min(w, h) - 2;
    int icon_y_offset = (h - icon_size) / 2;

    if (!m_cachedGraphics || m_lastHdc != dc) {
        if (m_cachedGraphics) delete m_cachedGraphics;
        m_cachedGraphics = new Graphics(dc);
        m_cachedGraphics->SetSmoothingMode(SmoothingModeAntiAlias);
        m_lastHdc = dc;
    }
    
    Color circle_color = m_has_system_error ? Color(217, 66, 53) : Color(118, 202, 83);
    SolidBrush circle_brush(circle_color);
    m_cachedGraphics->FillEllipse(&circle_brush, x + LEFT_MARGIN, y + icon_y_offset, icon_size, icon_size);

    RECT text_rect = { x + LEFT_MARGIN + icon_size + 4, y, x + w, y + h };
    SetTextColor(dc, CalculateTextColor(dark_mode));
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, m_value_text, -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void CNvidiaMonitorItem::SetValue(const wchar_t* value) { wcscpy_s(m_value_text, value); }
void CNvidiaMonitorItem::SetSystemErrorStatus(bool has_error) { m_has_system_error = has_error; }

// =================================================================
// CTempMonitorItem implementation - Final Version
// =================================================================
CTempMonitorItem::CTempMonitorItem(const wchar_t* name, const wchar_t* id)
{
    wcscpy_s(m_item_name, name);
    wcscpy_s(m_item_id, id);
    wcscpy_s(m_value_text, L"N/A");

    HDC hdc = GetDC(NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SIZE value_size;
    GetTextExtentPoint32W(hdc, GetItemValueSampleText(), (int)wcslen(GetItemValueSampleText()), &value_size);
    m_width = value_size.cx + 4;
    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);
}

const wchar_t* CTempMonitorItem::GetItemName() const { return m_item_name; }
const wchar_t* CTempMonitorItem::GetItemId() const { return m_item_id; }
const wchar_t* CTempMonitorItem::GetItemLableText() const { return L""; }
const wchar_t* CTempMonitorItem::GetItemValueText() const { return m_value_text; }
const wchar_t* CTempMonitorItem::GetItemValueSampleText() const { return L"99°C"; }
bool CTempMonitorItem::IsCustomDraw() const { return true; }
int CTempMonitorItem::GetItemWidth() const { return m_width; }

void CTempMonitorItem::SetValue(int temp)
{
    m_temp = temp;
    if (temp > 0) swprintf_s(m_value_text, L"%d°C", temp);
    else wcscpy_s(m_value_text, L"N/A");
}

COLORREF CTempMonitorItem::GetTemperatureColor() const
{
    if (m_temp >= 80) return RGB(217, 66, 53);
    if (m_temp >= 70) return RGB(246, 182, 78);
    return RGB(118, 202, 83);
}

void CTempMonitorItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };

    HBRUSH bg_brush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    SetBkMode(dc, TRANSPARENT);

    COLORREF value_color = (m_temp > 0) ? GetTemperatureColor() : (dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0));
    SetTextColor(dc, value_color);
    DrawTextW(dc, m_value_text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// =================================================================
// CCPUCoreBarsPlugin implementation
// =================================================================
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance() { static CCPUCoreBarsPlugin instance; return instance; }

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
        m_all_items.push_back(new CCpuUsageItem(i, (m_core_efficiency[i] == 0)));
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

    if (m_gpu_item) m_all_items.push_back(m_gpu_item);
    m_cpu_temp_item = new CTempMonitorItem(L"CPU Temperature", L"cpu_temp");
    m_all_items.push_back(m_cpu_temp_item);
    m_gpu_temp_item = new CTempMonitorItem(L"GPU Temperature", L"gpu_temp");
    m_all_items.push_back(m_gpu_temp_item);
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    if (m_query) PdhCloseQuery(m_query);
    for (auto item : m_all_items) delete item;
    ShutdownNVML();
    GdiplusShutdown(m_gdiplusToken);
}

IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < m_all_items.size()) {
        return m_all_items[index];
    }
    return nullptr;
}

void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
    UpdateGpuLimitReason();
    
    if (m_cpu_temp_item) m_cpu_temp_item->SetValue(m_cpu_temp);
    if (m_gpu_temp_item) m_gpu_temp_item->SetValue(m_gpu_temp);

    DWORD current_time = GetTickCount();
    if (current_time - m_last_error_check_time > ERROR_CHECK_INTERVAL_MS) {
        UpdateWheaErrorCount();
        UpdateNvlddmkmErrorCount();
        m_last_error_check_time = current_time;
    }
    
    if (m_gpu_item) {
        m_gpu_item->SetSystemErrorStatus((m_cached_whea_count > 0 || m_cached_nvlddmkm_count > 0));
    }
}

void CCPUCoreBarsPlugin::OnMonitorInfo(const ITMPlugin::MonitorInfo& monitor_info)
{
    m_cpu_temp = monitor_info.cpu_temperature;
    m_gpu_temp = monitor_info.gpu_temperature;
}

const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index) {
    case TMI_NAME: return L"性能/错误监控";
    case TMI_DESCRIPTION: return L"CPU核心条形图/GPU受限&错误/WHEA错误/温度";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"3.9.0";
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::InitNVML()
{
    m_nvml_dll = LoadLibrary(L"nvml.dll");
    if (!m_nvml_dll) return;

    p_nvmlInit = (decltype(p_nvmlInit))GetProcAddress(m_nvml_dll, "nvmlInit_v2");
    p_nvmlShutdown = (decltype(p_nvmlShutdown))GetProcAddress(m_nvml_dll, "nvmlShutdown");
    p_nvmlDeviceGetHandleByIndex = (decltype(p_nvmlDeviceGetHandleByIndex))GetProcAddress(m_nvml_dll, "nvmlDeviceGetHandleByIndex_v2");
    p_nvmlDeviceGetCurrentClocksThrottleReasons = (decltype(p_nvmlDeviceGetCurrentClocksThrottleReasons))GetProcAddress(m_nvml_dll, "nvmlDeviceGetCurrentClocksThrottleReasons");

    if (!p_nvmlInit || !p_nvmlShutdown || !p_nvmlDeviceGetHandleByIndex || !p_nvmlDeviceGetCurrentClocksThrottleReasons) {
        ShutdownNVML(); return;
    }
    if (p_nvmlInit() != NVML_SUCCESS) {
        ShutdownNVML(); return;
    }
    if (p_nvmlDeviceGetHandleByIndex(0, &m_nvml_device) != NVML_SUCCESS) {
        ShutdownNVML(); return;
    }
    m_nvml_initialized = true;
    m_gpu_item = new CNvidiaMonitorItem();
}

void CCPUCoreBarsPlugin::ShutdownNVML()
{
    if (m_nvml_initialized && p_nvmlShutdown) p_nvmlShutdown();
    if (m_nvml_dll) FreeLibrary(m_nvml_dll);
    m_nvml_initialized = false;
    m_nvml_dll = nullptr;
}

void CCPUCoreBarsPlugin::UpdateGpuLimitReason()
{
    if (!m_nvml_initialized || !m_gpu_item) return;
    unsigned long long reasons = 0;
    if (p_nvmlDeviceGetCurrentClocksThrottleReasons(m_nvml_device, &reasons) == NVML_SUCCESS) {
        if (reasons & nvmlClocksThrottleReasonHwThermalSlowdown) m_gpu_item->SetValue(L"过热");
        else if (reasons & nvmlClocksThrottleReasonSwThermalSlowdown) m_gpu_item->SetValue(L"过热");
        else if (reasons & nvmlClocksThrottleReasonHwPowerBrakeSlowdown) m_gpu_item->SetValue(L"功耗");
        else if (reasons & nvmlClocksThrottleReasonSwPowerCap) m_gpu_item->SetValue(L"功耗");
        else if (reasons & nvmlClocksThrottleReasonGpuIdle) m_gpu_item->SetValue(L"空闲");
        else if (reasons == nvmlClocksThrottleReasonApplicationsClocksSetting) m_gpu_item->SetValue(L"无限");
        else m_gpu_item->SetValue(L"无");
    } else {
        m_gpu_item->SetValue(L"错误");
    }
}

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    if (!m_query || PdhCollectQueryData(m_query) != ERROR_SUCCESS) return;
    for (int i = 0; i < m_num_cores; ++i) {
        PDH_FMT_COUNTERVALUE value;
        double usage = 0.0;
        if (PdhGetFormattedCounterValue(m_counters[i], PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) {
            usage = value.doubleValue / 100.0;
        }
        if (auto cpu_item = dynamic_cast<CCpuUsageItem*>(m_all_items[i])) {
            cpu_item->SetUsage(usage);
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
    auto* proc_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data();
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, proc_info, &length)) return;

    char* ptr = buffer.data();
    while (ptr < buffer.data() + length) {
        auto* current_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;
        if (current_info->Relationship == RelationProcessorCore) {
            for (int i = 0; i < current_info->Processor.GroupCount; ++i) {
                KAFFINITY mask = current_info->Processor.GroupMask[i].Mask;
                for (int j = 0; j < sizeof(KAFFINITY) * 8; ++j) {
                    if ((mask >> j) & 1 && j < m_num_cores) {
                        m_core_efficiency[j] = current_info->Processor.EfficiencyClass;
                    }
                }
            }
        }
        ptr += current_info->Size;
    }
}

DWORD CCPUCoreBarsPlugin::QueryEventLogCount(LPCWSTR provider_name)
{
    wchar_t query[256];
    swprintf_s(query, L"*[System[Provider[@Name='%s'] and TimeCreated[timediff(@SystemTime) <= 86400000]]]", provider_name);
    EVT_HANDLE hResults = EvtQuery(NULL, L"System", query, EvtQueryChannelPath | EvtQueryReverseDirection);
    if (!hResults) return 0;

    DWORD total_count = 0;
    EVT_HANDLE hEvents[256];
    DWORD returned = 0;
    while (EvtNext(hResults, ARRAYSIZE(hEvents), hEvents, 1000, 0, &returned)) {
        total_count += returned;
        for (DWORD i = 0; i < returned; i++) EvtClose(hEvents[i]);
    }
    EvtClose(hResults);
    return total_count;
}

void CCPUCoreBarsPlugin::UpdateWheaErrorCount() { m_cached_whea_count = QueryEventLogCount(L"WHEA-Logger"); }
void CCPUCoreBarsPlugin::UpdateNvlddmkmErrorCount() { m_cached_nvlddmkm_count = QueryEventLogCount(L"nvlddmkm"); }

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance() { return &CCPUCoreBarsPlugin::Instance(); }