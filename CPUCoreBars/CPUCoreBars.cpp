// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>
#include <winevt.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wevtapi.lib")

// =================================================================
// CCpuUsageItem implementation
// =================================================================
CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core) 
    : m_core_index(core_index), m_is_e_core(is_e_core)
{
    swprintf_s(m_item_name, L"CPU Core %d", m_core_index);
    swprintf_s(m_item_id, L"cpu_core_%d", m_core_index);
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
    return 8; // UPDATED: Width changed to 8
}

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = max(0.0, min(1.0, usage));
}

void CCpuUsageItem::DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode)
{
    COLORREF icon_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    SetTextColor(hDC, icon_color);
    SetBkMode(hDC, TRANSPARENT);
    const wchar_t* symbol = L"\u2618"; // UPDATED: Icon changed to shamrock
    HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
    HGDIOBJ hOldFont = SelectObject(hDC, hFont);
    DrawTextW(hDC, symbol, -1, (LPRECT)&rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hDC, hOldFont);
    DeleteObject(hFont);
}

void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    COLORREF bar_color;
    if (m_core_index >= 12 && m_core_index <= 19) {
        bar_color = RGB(217, 66, 53);
    } else if (m_core_index % 2 == 1) {
        bar_color = RGB(38, 160, 218);
    } else {
        bar_color = RGB(118, 202, 83);
    }

    if (m_usage >= 0.9) { // UPDATED: Threshold changed to 0.9
        bar_color = RGB(217, 66, 53);
    } else if (m_usage >= 0.5) {
        bar_color = RGB(246, 182, 78);
    }

    int bar_height = static_cast<int>(h * m_usage);
    if (bar_height > 0) {
        RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
        HBRUSH bar_brush = CreateSolidBrush(bar_color);
        FillRect(dc, &bar_rect, bar_brush);
        DeleteObject(bar_brush);
    }

    if (m_is_e_core) {
        DrawECoreSymbol(dc, rect, dark_mode);
    }
}


// =================================================================
// CNvidiaMonitorItem implementation
// =================================================================
CNvidiaMonitorItem::CNvidiaMonitorItem()
{
    wcscpy_s(m_value_text, L"N/A");

    HDC hdc = GetDC(NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    const wchar_t* sample_value = GetItemValueSampleText();
    SIZE value_size;
    GetTextExtentPoint32W(hdc, sample_value, (int)wcslen(sample_value), &value_size);

    const wchar_t* sample_icon = L"🔴"; // Red circle emoji
    SIZE icon_size;
    GetTextExtentPoint32W(hdc, sample_icon, (int)wcslen(sample_icon), &icon_size);
    
    m_width = icon_size.cx + 4 + value_size.cx;

    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);
}

const wchar_t* CNvidiaMonitorItem::GetItemName() const
{
    return L"GPU/WHEA 状态";
}

const wchar_t* CNvidiaMonitorItem::GetItemId() const
{
    return L"gpu_system_status";
}

const wchar_t* CNvidiaMonitorItem::GetItemLableText() const
{
    return L"🟢"; // Green circle emoji
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

    // UPDATED: Icon logic is now a simple choice between two emojis
    const wchar_t* icon_text = m_has_system_error ? L"🔴" : L"🟢";

    COLORREF default_text_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    SetBkMode(dc, TRANSPARENT);

    SIZE current_icon_size;
    GetTextExtentPoint32W(dc, icon_text, (int)wcslen(icon_text), &current_icon_size);
    int icon_width = current_icon_size.cx;

    RECT icon_rect = { x, y, x + icon_width, y + h };
    RECT text_rect = { x + icon_width + 4, y, x + w, y + h };

    // Emojis have their own color, so we don't need to set a text color for them
    DrawTextW(dc, icon_text, -1, &icon_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Determine the color for the value text
    COLORREF value_text_color = default_text_color;
    const wchar_t* current_value = GetItemValueText();
    if (wcscmp(current_value, L"过热") == 0)
    {
        value_text_color = RGB(217, 66, 53);
    }
    else if (wcscmp(current_value, L"功耗") == 0)
    {
        value_text_color = RGB(246, 182, 78);
    }

    SetTextColor(dc, value_text_color);
    DrawTextW(dc, current_value, -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
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
// CCPUCoreBarsPlugin implementation
// =================================================================
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
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
    InitNVML();
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    if (m_query) PdhCloseQuery(m_query);
    for (auto item : m_items) delete item;
    if (m_gpu_item) delete m_gpu_item;
    ShutdownNVML();
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
    UpdateWheaErrorCount();
    UpdateNvlddmkmErrorCount();

    if (m_gpu_item) {
        bool has_error = (m_whea_error_count > 0 || m_nvlddmkm_error_count > 0);
        m_gpu_item->SetSystemErrorStatus(has_error);
    }
}

const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index) {
    case TMI_NAME: return L"性能/错误监控";
    case TMI_DESCRIPTION: return L"CPU核心使用率条形图/GPU受限/WHEA错误";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"3.0.0";
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

void CCPUCoreBarsPlugin::UpdateWheaErrorCount()
{
    LPCWSTR query = L"*[System[Provider[@Name='WHEA-Logger'] and TimeCreated[timediff(@SystemTime) <= 86400000]]]";
    EVT_HANDLE hResults = EvtQuery(NULL, L"System", query, EvtQueryChannelPath | EvtQueryReverseDirection);
    if (hResults == NULL) {
        m_whea_error_count = 0;
        return;
    }

    DWORD dwEventCount = 0;
    EVT_HANDLE hEvents[128];
    DWORD dwReturned = 0;
    while (EvtNext(hResults, ARRAYSIZE(hEvents), hEvents, INFINITE, 0, &dwReturned))
    {
        dwEventCount += dwReturned;
        for (DWORD i = 0; i < dwReturned; i++) {
            EvtClose(hEvents[i]);
        }
    }
    m_whea_error_count = dwEventCount;
    EvtClose(hResults);
}

void CCPUCoreBarsPlugin::UpdateNvlddmkmErrorCount()
{
    LPCWSTR query = L"*[System[Provider[@Name='nvlddmkm'] and TimeCreated[timediff(@SystemTime) <= 86400000]]]";
    EVT_HANDLE hResults = EvtQuery(NULL, L"System", query, EvtQueryChannelPath | EvtQueryReverseDirection);
    if (hResults == NULL) {
        m_nvlddmkm_error_count = 0;
        return;
    }

    DWORD dwEventCount = 0;
    EVT_HANDLE hEvents[128];
    DWORD dwReturned = 0;
    while (EvtNext(hResults, ARRAYSIZE(hEvents), hEvents, INFINITE, 0, &dwReturned))
    {
        dwEventCount += dwReturned;
        for (DWORD i = 0; i < dwReturned; i++) {
            EvtClose(hEvents[i]);
        }
    }
    m_nvlddmkm_error_count = dwEventCount;
    EvtClose(hResults);
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}