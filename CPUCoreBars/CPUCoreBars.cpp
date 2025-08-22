// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// =================================================================
// CCpuUsageItem implementation (no changes, can be collapsed)
// =================================================================
// ... (The entire implementation of CCpuUsageItem from the previous step is here)
CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core) : m_core_index(core_index), m_is_e_core(is_e_core) { swprintf_s(m_item_name, L"CPU Core %d", m_core_index); swprintf_s(m_item_id, L"cpu_core_%d", m_core_index); }
const wchar_t* CCpuUsageItem::GetItemName() const { return m_item_name; }
const wchar_t* CCpuUsageItem::GetItemId() const { return m_item_id; }
const wchar_t* CCpuUsageItem::GetItemLableText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueSampleText() const { return L""; }
bool CCpuUsageItem::IsCustomDraw() const { return true; }
int CCpuUsageItem::GetItemWidth() const { return 10; }
void CCpuUsageItem::SetUsage(double usage) { m_usage = max(0.0, min(1.0, usage)); }
void CCpuUsageItem::DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode) { COLORREF icon_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0); SetTextColor(hDC, icon_color); SetBkMode(hDC, TRANSPARENT); const wchar_t* symbol = L"\u26B2"; HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol"); HGDIOBJ hOldFont = SelectObject(hDC, hFont); DrawTextW(hDC, symbol, -1, (LPRECT)&rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE); SelectObject(hDC, hOldFont); DeleteObject(hFont); }
void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) { HDC dc = (HDC)hDC; RECT rect = { x, y, x + w, y + h }; HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255)); FillRect(dc, &rect, bg_brush); DeleteObject(bg_brush); COLORREF bar_color; if (m_core_index >= 12 && m_core_index <= 19) { bar_color = RGB(217, 66, 53); } else if (m_core_index % 2 == 1) { bar_color = RGB(38, 160, 218); } else { bar_color = RGB(118, 202, 83); } if (m_usage >= 0.8) { bar_color = RGB(217, 66, 53); } else if (m_usage >= 0.5) { bar_color = RGB(246, 182, 78); } int bar_height = static_cast<int>(h * m_usage); if (bar_height > 0) { RECT bar_rect = { x, y + (h - bar_height), x + w, y + h }; HBRUSH bar_brush = CreateSolidBrush(bar_color); FillRect(dc, &bar_rect, bar_brush); DeleteObject(bar_brush); } if (m_is_e_core) { DrawECoreSymbol(dc, rect, dark_mode); } }


// =================================================================
// UPDATED: CNvidiaLimitReasonItem implementation
// =================================================================
CNvidiaLimitReasonItem::CNvidiaLimitReasonItem()
{
    wcscpy_s(m_value_text, L"N/A");
}

const wchar_t* CNvidiaLimitReasonItem::GetItemName() const { return L"NVIDIA 限制原因"; }
const wchar_t* CNvidiaLimitReasonItem::GetItemId() const { return L"nvidia_limit_reason"; }

// UPDATED: Replace the text label with a kaomoji icon
const wchar_t* CNvidiaLimitReasonItem::GetItemLableText() const { return L"❄"; }

const wchar_t* CNvidiaLimitReasonItem::GetItemValueText() const { return m_value_text; }
const wchar_t* CNvidiaLimitReasonItem::GetItemValueSampleText() const { return L"硬过热"; }
bool CNvidiaLimitReasonItem::IsCustomDraw() const { return false; }
int CNvidiaLimitReasonItem::GetItemWidth() const { return 0; }
void CNvidiaLimitReasonItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) { }
void CNvidiaLimitReasonItem::SetValue(const wchar_t* value) { wcscpy_s(m_value_text, value); }


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
    // --- CPU Initialization ---
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

    // --- GPU/NVML Initialization ---
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
    if (index < m_num_cores) return m_items[index];
    if (index == m_num_cores && m_gpu_item != nullptr) return m_gpu_item;
    return nullptr;
}

void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
    UpdateGpuLimitReason();
}

const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME: return L"CPU/GPU 性能监视器";
    case TMI_DESCRIPTION: return L"显示CPU核心使用率和NVIDIA GPU性能限制原因。";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"2.2.0"; // Version bump
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
    if (!pfn_nvmlInit || !pfn_nvmlShutdown || !pfn_nvmlDeviceGetHandleByIndex || !pfn_nvmlDeviceGetCurrentClocksThrottleReasons) { ShutdownNVML(); return; }
    if (pfn_nvmlInit() != NVML_SUCCESS) { ShutdownNVML(); return; }
    if (pfn_nvmlDeviceGetHandleByIndex(0, &m_nvml_device) != NVML_SUCCESS) { ShutdownNVML(); return; }
    m_nvml_initialized = true;
    m_gpu_item = new CNvidiaLimitReasonItem();
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
    if (pfn_nvmlDeviceGetCurrentClocksThrottleReasons(m_nvml_device, &reasons) == NVML_SUCCESS)
    {
        if (reasons & nvmlClocksThrottleReasonHwThermalSlowdown) { m_gpu_item->SetValue(L"硬过热"); }
        else if (reasons & nvmlClocksThrottleReasonHwPowerBrakeSlowdown) { m_gpu_item->SetValue(L"硬功耗"); }
        else if (reasons & nvmlClocksThrottleReasonSwPowerCap) { m_gpu_item->SetValue(L"软功耗"); }
        else if (reasons & nvmlClocksThrottleReasonSwThermalSlowdown) { m_gpu_item->SetValue(L"软过热"); }
        else if (reasons & nvmlClocksThrottleReasonGpuIdle) { m_gpu_item->SetValue(L"空闲"); }
        else if (reasons == nvmlClocksThrottleReasonApplicationsClocksSetting) { m_gpu_item->SetValue(L"无限制"); }
        else { m_gpu_item->SetValue(L"无"); }
    }
    else { m_gpu_item->SetValue(L"错误"); }
}

// CPU functions (unchanged)
void CCPUCoreBarsPlugin::UpdateCpuUsage() { if (!m_query) return; if (PdhCollectQueryData(m_query) == ERROR_SUCCESS) { for (int i = 0; i < m_num_cores; ++i) { PDH_FMT_COUNTERVALUE value; if (PdhGetFormattedCounterValue(m_counters[i], PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) { m_items[i]->SetUsage(value.doubleValue / 100.0); } else { m_items[i]->SetUsage(0.0); } } } }
void CCPUCoreBarsPlugin::DetectCoreTypes() { m_core_efficiency.assign(m_num_cores, 1); DWORD length = 0; GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length); if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return; std::vector<char> buffer(length); PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data(); if (!GetLogicalProcessorInformationEx(RelationProcessorCore, proc_info, &length)) return; char* ptr = buffer.data(); while (ptr < buffer.data() + length) { PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr; if (current_info->Relationship == RelationProcessorCore) { BYTE efficiency = current_info->Processor.EfficiencyClass; for (int i = 0; i < current_info->Processor.GroupCount; ++i) { KAFFINITY mask = current_info->Processor.GroupMask[i].Mask; for (int j = 0; j < sizeof(KAFFINITY) * 8; ++j) { if ((mask >> j) & 1) { int logical_proc_index = j; if (logical_proc_index < m_num_cores) { m_core_efficiency[logical_proc_index] = efficiency; } } } } } ptr += current_info->Size; } }

// Exported function (unchanged)
extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}