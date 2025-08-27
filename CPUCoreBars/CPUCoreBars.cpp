// CPUCoreBars/CPUCoreBars.cpp - 性能优化和硬件监控集成版本
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>
#include <winevt.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "gdiplus.lib")

// Add .NET references for LibreHardwareMonitor
#using <System.dll>
#using "LibreHardwareMonitorLib.dll"

using namespace Gdiplus;
using namespace System;
using namespace System::Runtime::InteropServices;
using namespace LibreHardwareMonitor::Hardware;

// =================================================================
// Helper functions (adapted from HardwareMonitorHelper and Common)
// =================================================================
namespace HardwareMonitor
{
    // Helper to convert System::String to std::wstring
    std::wstring StringToStdWstring(String^ s)
    {
        if (s == nullptr)
            return L"";
        const wchar_t* chars = (const wchar_t*)(Marshal::StringToHGlobalUni(s)).ToPointer();
        std::wstring os = chars;
        Marshal::FreeHGlobal(IntPtr((void*)chars));
        return os;
    }

    // Helper to get a unique identifier for a sensor
    String^ GetSensorIdentifier(ISensor^ sensor)
    {
        String^ path;
        IHardware^ hardware = sensor->Hardware;
        while (hardware != nullptr)
        {
            path = hardware->Identifier + "/" + path;
            hardware = hardware->Parent;
        }
        path += sensor->SensorType.ToString();
        path += "/";
        path += sensor->Name;
        return path;
    }

    // Helper to format sensor value
    String^ GetSensorValueText(ISensor^ sensor)
    {
        if (sensor->Value.HasValue)
        {
            float value = sensor->Value.Value;
            String^ unit = L"°C";
            String^ formatString = "F0"; // No decimal places for temperature
            return value.ToString(formatString) + unit;
        }
        return "--";
    }
}


// =================================================================
// CCpuUsageItem implementation (unchanged from original)
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
void CCpuUsageItem::SetUsage(double usage) { m_usage = max(0.0, min(1.0, usage)); }

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
// CNvidiaMonitorItem implementation (unchanged from original)
// =================================================================
CNvidiaMonitorItem::CNvidiaMonitorItem() : m_cachedGraphics(nullptr), m_lastHdc(nullptr)
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

CNvidiaMonitorItem::~CNvidiaMonitorItem() { if (m_cachedGraphics) delete m_cachedGraphics; }
const wchar_t* CNvidiaMonitorItem::GetItemName() const { return L"GPU/WHEA"; }
const wchar_t* CNvidiaMonitorItem::GetItemId() const { return L"gpu_system_status"; }
const wchar_t* CNvidiaMonitorItem::GetItemLableText() const { return L""; }
const wchar_t* CNvidiaMonitorItem::GetItemValueText() const { return m_value_text; }
const wchar_t* CNvidiaMonitorItem::GetItemValueSampleText() const { return L"宽度"; }
bool CNvidiaMonitorItem::IsCustomDraw() const { return true; }
int CNvidiaMonitorItem::GetItemWidth() const { return m_width; }
void CNvidiaMonitorItem::SetValue(const wchar_t* value) { wcscpy_s(m_value_text, value); }
void CNvidiaMonitorItem::SetSystemErrorStatus(bool has_error) { m_has_system_error = has_error; }

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
    COLORREF value_text_color = CalculateTextColor(dark_mode);
    SetTextColor(dc, value_text_color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, GetItemValueText(), -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

// =================================================================
// CHardwareMonitorItem implementation - 新增
// =================================================================
CHardwareMonitorItem::CHardwareMonitorItem(const std::wstring& identifier, const std::wstring& label_text)
    : m_identifier(identifier), m_label_text(label_text), m_value_text(L"--"), m_sample_text(L"100°C")
{
    m_name = L"HWMon " + label_text;
    m_id = L"hwmon_" + label_text;
}

const wchar_t* CHardwareMonitorItem::GetItemName() const { return m_name.c_str(); }
const wchar_t* CHardwareMonitorItem::GetItemId() const { return m_id.c_str(); }
const wchar_t* CHardwareMonitorItem::GetItemLableText() const { return m_label_text.c_str(); }
const wchar_t* CHardwareMonitorItem::GetItemValueText() const { return m_value_text.c_str(); }
const wchar_t* CHardwareMonitorItem::GetItemValueSampleText() const { return m_sample_text.c_str(); }
bool CHardwareMonitorItem::IsCustomDraw() const { return false; } // Use default drawing
int CHardwareMonitorItem::GetItemWidth() const { return 0; } // Use default width
void CHardwareMonitorItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) {} // Not used

void CHardwareMonitorItem::UpdateValue()
{
    try
    {
        String^ identifier_str = gcnew String(m_identifier.c_str());
        ISensor^ sensor = nullptr;

        // Simplified sensor search
        auto computer = CCPUCoreBarsPlugin::Instance().m_computer;
        for each (IHardware^ hardware in computer->Hardware)
        {
            for each (ISensor^ s in hardware->Sensors)
            {
                if (HardwareMonitor::GetSensorIdentifier(s) == identifier_str)
                {
                    sensor = s;
                    break;
                }
            }
            if (sensor != nullptr) break;
            for each (IHardware^ subHardware in hardware->SubHardware)
            {
                 for each (ISensor^ s in subHardware->Sensors)
                 {
                    if (HardwareMonitor::GetSensorIdentifier(s) == identifier_str)
                    {
                        sensor = s;
                        break;
                    }
                 }
                 if (sensor != nullptr) break;
            }
        }

        if (sensor != nullptr)
        {
            m_value_text = HardwareMonitor::StringToStdWstring(HardwareMonitor::GetSensorValueText(sensor));
        }
        else
        {
            m_value_text = L"N/A";
        }
    }
    catch (Exception^)
    {
        m_value_text = L"Error";
    }
}

// =================================================================
// CCPUCoreBarsPlugin implementation - 修改
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
        m_plugin_items.push_back(new CCpuUsageItem(i, is_e_core));
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
    InitHardwareMonitor();
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    if (m_query) PdhCloseQuery(m_query);
    for (auto item : m_plugin_items) delete item;
    ShutdownNVML();
    ShutdownHardwareMonitor();
    GdiplusShutdown(m_gdiplusToken);
}

IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    if (index >= 0 && index < m_plugin_items.size()) {
        return m_plugin_items[index];
    }
    return nullptr;
}

int CCPUCoreBarsPlugin::GetItemCount()
{
    return static_cast<int>(m_plugin_items.size());
}

void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
    UpdateGpuLimitReason();
    UpdateHardwareMonitorItems();
    
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
    case TMI_NAME: return L"性能/错误/温度监控";
    case TMI_DESCRIPTION: return L"CPU核心/GPU受限/WHEA错误/硬件温度";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"3.7.0";
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
        ShutdownNVML(); return;
    }
    if (pfn_nvmlInit() != NVML_SUCCESS) {
        ShutdownNVML(); return;
    }
    if (pfn_nvmlDeviceGetHandleByIndex(0, &m_nvml_device) != NVML_SUCCESS) {
        ShutdownNVML(); return;
    }
    m_nvml_initialized = true;
    m_gpu_item = new CNvidiaMonitorItem();
    m_plugin_items.push_back(m_gpu_item);
}

void CCPUCoreBarsPlugin::ShutdownNVML()
{
    if (m_nvml_initialized && pfn_nvmlShutdown) pfn_nvmlShutdown();
    if (m_nvml_dll) FreeLibrary(m_nvml_dll);
    m_nvml_initialized = false;
    m_nvml_dll = nullptr;
}

void CCPUCoreBarsPlugin::InitHardwareMonitor()
{
    try
    {
        m_updateVisitor = gcnew UpdateVisitor();
        m_computer = gcnew Computer();
        m_computer->Open();
        m_computer->IsCpuEnabled = true;
        m_computer->IsGpuEnabled = true;
        m_computer->Accept(m_updateVisitor);

        ISensor^ cpu_temp_sensor = nullptr;
        ISensor^ gpu_temp_sensor = nullptr;

        for each (IHardware^ hardware in m_computer->Hardware)
        {
            // Find CPU Temperature Sensor ("Core Max" or "Package")
            if (hardware->HardwareType == HardwareType::Cpu)
            {
                for each (ISensor^ sensor in hardware->Sensors)
                {
                    if (sensor->SensorType == SensorType::Temperature && sensor->Name->Contains("Max"))
                    {
                        cpu_temp_sensor = sensor;
                        break;
                    }
                }
                // Fallback to first temperature sensor if "Max" not found
                if (cpu_temp_sensor == nullptr)
                {
                    for each (ISensor^ sensor in hardware->Sensors)
                    {
                        if (sensor->SensorType == SensorType::Temperature)
                        {
                            cpu_temp_sensor = sensor;
                            break;
                        }
                    }
                }
            }

            // Find GPU Temperature Sensor ("GPU Core")
            if (hardware->HardwareType == HardwareType::GpuNvidia || hardware->HardwareType == HardwareType::GpuAmd || hardware->HardwareType == HardwareType::GpuIntel)
            {
                for each (ISensor^ sensor in hardware->Sensors)
                {
                    if (sensor->SensorType == SensorType::Temperature && sensor->Name->Equals("GPU Core"))
                    {
                        gpu_temp_sensor = sensor;
                        break;
                    }
                }
                 // Fallback to first temperature sensor if "GPU Core" not found
                if (gpu_temp_sensor == nullptr)
                {
                    for each (ISensor^ sensor in hardware->Sensors)
                    {
                        if (sensor->SensorType == SensorType::Temperature)
                        {
                            gpu_temp_sensor = sensor;
                            break;
                        }
                    }
                }
            }
        }

        if (cpu_temp_sensor != nullptr)
        {
            std::wstring identifier = HardwareMonitor::StringToStdWstring(HardwareMonitor::GetSensorIdentifier(cpu_temp_sensor));
            m_cpu_temp_item = new CHardwareMonitorItem(identifier, L"CPU Max");
            m_plugin_items.push_back(m_cpu_temp_item);
        }

        if (gpu_temp_sensor != nullptr)
        {
            std::wstring identifier = HardwareMonitor::StringToStdWstring(HardwareMonitor::GetSensorIdentifier(gpu_temp_sensor));
            m_gpu_temp_item = new CHardwareMonitorItem(identifier, L"GPU");
            m_plugin_items.push_back(m_gpu_temp_item);
        }
    }
    catch (Exception^)
    {
        // Handle initialization error
    }
}

void CCPUCoreBarsPlugin::ShutdownHardwareMonitor()
{
    if (m_computer)
    {
        m_computer->Close();
        m_computer = nullptr;
    }
}

void CCPUCoreBarsPlugin::UpdateHardwareMonitorItems()
{
    if (m_computer)
    {
        m_computer->Accept(m_updateVisitor);
        if (m_cpu_temp_item) m_cpu_temp_item->UpdateValue();
        if (m_gpu_temp_item) m_gpu_temp_item->UpdateValue();
    }
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

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    if (!m_query) return;
    if (PdhCollectQueryData(m_query) == ERROR_SUCCESS) {
        for (int i = 0; i < m_num_cores; ++i) {
            PDH_FMT_COUNTERVALUE value;
            if (PdhGetFormattedCounterValue(m_counters[i], PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) {
                // Accessing the item through the generic vector requires a cast
                static_cast<CCpuUsageItem*>(m_plugin_items[i])->SetUsage(value.doubleValue / 100.0);
            } else {
                static_cast<CCpuUsageItem*>(m_plugin_items[i])->SetUsage(0.0);
            }
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

DWORD CCPUCoreBarsPlugin::QueryEventLogCount(LPCWSTR provider_name)
{
    wchar_t query[256];
    swprintf_s(query, L"*[System[Provider[@Name='%s'] and TimeCreated[timediff(@SystemTime) <= 86400000]]]", provider_name);
    EVT_HANDLE hResults = EvtQuery(NULL, L"System", query, EvtQueryChannelPath | EvtQueryReverseDirection);
    if (hResults == NULL) return 0;
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