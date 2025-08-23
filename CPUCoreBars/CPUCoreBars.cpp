// CPUCoreBars/CPUCoreBars.cpp - 性能优化版本 (ETW + NVML ECC)
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>
#include <tdh.h> // For ETW GUID

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// NEW: WHEA Provider GUID for ETW
// {C2D578F2-5948-4527-9598-345A212C4ECE}
static const GUID WHEA_PROVIDER_GUID =
{ 0xc2d578f2, 0x5948, 0x4527, { 0x95, 0x98, 0x34, 0x5a, 0x21, 0x2c, 0x4e, 0xce } };


// =================================================================
// CCpuUsageItem implementation - 优化版本 (No changes in this class)
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
// CNvidiaMonitorItem implementation - 优化版本 (MODIFIED)
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
    if (m_cachedGraphics) delete m_cachedGraphics;
}

const wchar_t* CNvidiaMonitorItem::GetItemName() const { return L"GPU/System"; }
const wchar_t* CNvidiaMonitorItem::GetItemId() const { return L"gpu_system_status"; }
const wchar_t* CNvidiaMonitorItem::GetItemLableText() const { return L""; }
const wchar_t* CNvidiaMonitorItem::GetItemValueText() const { return m_value_text; }
const wchar_t* CNvidiaMonitorItem::GetItemValueSampleText() const { return L"宽度"; }
bool CNvidiaMonitorItem::IsCustomDraw() const { return true; }
int CNvidiaMonitorItem::GetItemWidth() const { return m_width; }

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

    // MODIFIED: Use a switch on the status to determine circle color
    Color circle_color;
    switch (m_system_status)
    {
    case SystemStatus::Error:
        circle_color = Color(217, 66, 53); // Red
        break;
    case SystemStatus::Warning:
        circle_color = Color(246, 182, 78); // Orange
        break;
    case SystemStatus::Ok:
    default:
        circle_color = Color(118, 202, 83); // Green
        break;
    }
    
    SolidBrush circle_brush(circle_color);
    m_cachedGraphics->FillEllipse(&circle_brush,
        x + LEFT_MARGIN, y + icon_y_offset, icon_size, icon_size);

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

// MODIFIED: Implementation of the new status setter
void CNvidiaMonitorItem::SetSystemStatus(SystemStatus status)
{
    m_system_status = status;
}


// =================================================================
// CCPUCoreBarsPlugin implementation - 优化版本 (MODIFIED)
// =================================================================
CCPUCoreBarsPlugin* CCPUCoreBarsPlugin::s_instance = nullptr;

CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin()
{
    s_instance = this; // Set static instance pointer
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
    StartWheaEtwListener(); // NEW: Start the listener
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    StopWheaEtwListener(); // NEW: Stop the listener
    if (m_query) PdhCloseQuery(m_query);
    for (auto item : m_items) delete item;
    if (m_gpu_item) delete m_gpu_item;
    ShutdownNVML();
    GdiplusShutdown(m_gdiplusToken);
}

IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    if (index < m_num_cores) return m_items[index];
    if (index == m_num_cores && m_gpu_item != nullptr) return m_gpu_item;
    return nullptr;
}

void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
    UpdateGpuLimitReason();
    // NEW: Call the combined status update function
    UpdateSystemStatus();
}

const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index) {
    case TMI_NAME: return L"性能/错误监控 v2";
    case TMI_DESCRIPTION: return L"CPU核心/GPU受限/WHEA&ECC错误";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"4.0.0";
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
    // NEW: Get address for ECC function
    pfn_nvmlDeviceGetTotalEccErrors = (decltype(pfn_nvmlDeviceGetTotalEccErrors))GetProcAddress(m_nvml_dll, "nvmlDeviceGetTotalEccErrors");

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
    if (m_nvml_initialized && pfn_nvmlShutdown) pfn_nvmlShutdown();
    if (m_nvml_dll) FreeLibrary(m_nvml_dll);
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

// NEW: Combined function to check NVML ECC and WHEA ETW flag
void CCPUCoreBarsPlugin::UpdateSystemStatus()
{
    if (!m_nvml_initialized || !m_gpu_item) return;

    bool has_new_uncorrected_errors = false;
    bool has_new_corrected_errors = false;

    // Check for NVML ECC errors if the function pointer is valid
    if (pfn_nvmlDeviceGetTotalEccErrors)
    {
        unsigned long long corrected = 0;
        unsigned long long uncorrected = 0;
        if (pfn_nvmlDeviceGetTotalEccErrors(m_nvml_device, NVML_MEMORY_ERROR_TYPE_UNCORRECTED, NVML_VOLATILE_ECC, &uncorrected) == NVML_SUCCESS)
        {
            if (uncorrected > m_last_uncorrected_ecc_count)
            {
                has_new_uncorrected_errors = true;
                m_last_uncorrected_ecc_count = uncorrected;
            }
        }
        if (pfn_nvmlDeviceGetTotalEccErrors(m_nvml_device, NVML_MEMORY_ERROR_TYPE_CORRECTED, NVML_VOLATILE_ECC, &corrected) == NVML_SUCCESS)
        {
            if (corrected > m_last_corrected_ecc_count)
            {
                has_new_corrected_errors = true;
                m_last_corrected_ecc_count = corrected;
            }
        }
    }

    // Determine status based on priority: Red > Orange > Green
    if (m_has_whea_error || has_new_uncorrected_errors)
    {
        m_gpu_item->SetSystemStatus(CNvidiaMonitorItem::SystemStatus::Error);
    }
    else if (has_new_corrected_errors)
    {
        m_gpu_item->SetSystemStatus(CNvidiaMonitorItem::SystemStatus::Warning);
    }
    else
    {
        m_gpu_item->SetSystemStatus(CNvidiaMonitorItem::SystemStatus::Ok);
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

// NEW: ETW callback function (static)
VOID WINAPI CCPUCoreBarsPlugin::EtwEventCallback(PEVENT_RECORD pEventRecord)
{
    // The static instance pointer is set in the constructor
    if (s_instance && IsEqualGUID(pEventRecord->EventHeader.ProviderId, WHEA_PROVIDER_GUID))
    {
        // A WHEA error was detected. Set the flag.
        // This flag is volatile, ensuring visibility across threads.
        s_instance->m_has_whea_error = true;
    }
}

// NEW: ETW listener thread function (static)
DWORD WINAPI CCPUCoreBarsPlugin::EtwThreadProc(LPVOID lpParam)
{
    CCPUCoreBarsPlugin* pThis = static_cast<CCPUCoreBarsPlugin*>(lpParam);
    
    EVENT_TRACE_LOGFILEW trace_logfile = { 0 };
    trace_logfile.LoggerName = KERNEL_LOGGER_NAMEW;
    trace_logfile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    trace_logfile.EventRecordCallback = EtwEventCallback;

    TRACEHANDLE trace_handle = OpenTraceW(&trace_logfile);
    if (trace_handle == INVALID_PROCESSTRACE_HANDLE) {
        return GetLastError();
    }
    
    // This is a blocking call that will process events until the trace is closed.
    ProcessTrace(&trace_handle, 1, NULL, NULL);

    CloseTrace(trace_handle);
    return ERROR_SUCCESS;
}

// NEW: Function to start the ETW listener
void CCPUCoreBarsPlugin::StartWheaEtwListener()
{
    if (m_etw_thread_handle) return; // Already running

    ULONG status = ERROR_SUCCESS;
    EVENT_TRACE_PROPERTIES* p_session_properties = nullptr;
    ULONG buffer_size = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(KERNEL_LOGGER_NAMEW);
    p_session_properties = (EVENT_TRACE_PROPERTIES*)malloc(buffer_size);
    if (p_session_properties == nullptr) return;

    ZeroMemory(p_session_properties, buffer_size);
    p_session_properties->Wnode.BufferSize = buffer_size;
    p_session_properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    p_session_properties->Wnode.ClientContext = 1; // Use QueryPerformanceCounter for timestamps
    p_session_properties->Wnode.Guid = SystemTraceControlGuid;
    p_session_properties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    p_session_properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    // Start a real-time trace session for the kernel logger
    status = StartTraceW(&m_etw_session_handle, KERNEL_LOGGER_NAMEW, p_session_properties);
    if (status != ERROR_SUCCESS && status != ERROR_ALREADY_EXISTS) {
        free(p_session_properties);
        return;
    }

    // Enable the WHEA provider for our session
    status = EnableTraceEx2(m_etw_session_handle, &WHEA_PROVIDER_GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_VERBOSE, 0, 0, 0, NULL);
    if (status != ERROR_SUCCESS) {
        ControlTraceW(m_etw_session_handle, KERNEL_LOGGER_NAMEW, p_session_properties, EVENT_TRACE_CONTROL_STOP);
        free(p_session_properties);
        return;
    }

    free(p_session_properties);

    // Create the thread that will consume the events
    m_etw_thread_handle = CreateThread(nullptr, 0, EtwThreadProc, this, 0, nullptr);
}

// NEW: Function to stop the ETW listener
void CCPUCoreBarsPlugin::StopWheaEtwListener()
{
    if (m_etw_session_handle) {
        EVENT_TRACE_PROPERTIES* p_session_properties = nullptr;
        ULONG buffer_size = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(KERNEL_LOGGER_NAMEW);
        p_session_properties = (EVENT_TRACE_PROPERTIES*)malloc(buffer_size);
        if (p_session_properties) {
            ZeroMemory(p_session_properties, buffer_size);
            p_session_properties->Wnode.BufferSize = buffer_size;
            p_session_properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
            
            // Disabling the provider and stopping the session will unblock ProcessTrace
            EnableTraceEx2(m_etw_session_handle, &WHEA_PROVIDER_GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, NULL);
            ControlTraceW(m_etw_session_handle, KERNEL_LOGGER_NAMEW, p_session_properties, EVENT_TRACE_CONTROL_STOP);
            free(p_session_properties);
        }
    }

    if (m_etw_thread_handle) {
        WaitForSingleObject(m_etw_thread_handle, 5000); // Wait up to 5s for the thread to exit
        CloseHandle(m_etw_thread_handle);
        m_etw_thread_handle = nullptr;
    }
}


extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}