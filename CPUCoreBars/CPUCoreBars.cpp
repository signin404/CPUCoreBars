// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// =================================================================
// CCpuUsageItem implementation (no changes)
// =================================================================
// ... (omitted for brevity)

// =================================================================
// CNvidiaLimitReasonItem implementation (no changes)
// =================================================================
// ... (omitted for brevity)


// =================================================================
// NEW: CNvidiaPStateItem implementation
// =================================================================
CNvidiaPStateItem::CNvidiaPStateItem()
{
    wcscpy_s(m_label_text, L"P-State:");
    wcscpy_s(m_value_text, L"N/A");
}

const wchar_t* CNvidiaPStateItem::GetItemName() const { return L"NVIDIA 性能状态"; }
const wchar_t* CNvidiaPStateItem::GetItemId() const { return L"nvidia_pstate"; }
const wchar_t* CNvidiaPStateItem::GetItemLableText() const { return m_label_text; } // Returns dynamic label
const wchar_t* CNvidiaPStateItem::GetItemValueText() const { return m_value_text; }
const wchar_t* CNvidiaPStateItem::GetItemValueSampleText() const { return L"硬过热"; } // Use a long string for width
bool CNvidiaPStateItem::IsCustomDraw() const { return false; }
int CNvidiaPStateItem::GetItemWidth() const { return 0; }
void CNvidiaPStateItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) { }

void CNvidiaPStateItem::SetValue(nvmlPstate_t pstate, const wchar_t* reason_text)
{
    if (pstate <= NVML_PSTATE_15)
    {
        swprintf_s(m_label_text, L"P%d:", pstate);
    }
    else
    {
        wcscpy_s(m_label_text, L"P?:");
    }
    wcscpy_s(m_value_text, reason_text);
}


// =================================================================
// CCPUCoreBarsPlugin implementation (updated for P-State)
// =================================================================
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance() { /* ... */ }

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin()
{
    // --- CPU Initialization ---
    // ... (omitted for brevity)

    // --- GPU/NVML Initialization ---
    InitNVML();
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    // ... (CPU cleanup)
    if (m_gpu_limit_item) delete m_gpu_limit_item;
    if (m_gpu_pstate_item) delete m_gpu_pstate_item; // Cleanup new item
    ShutdownNVML();
}

// UPDATED: GetItem handles one more GPU item
IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    if (index < m_num_cores) return m_items[index];
    
    int gpu_item_index = index - m_num_cores;
    if (gpu_item_index == 0 && m_gpu_limit_item) return m_gpu_limit_item;
    if (gpu_item_index == 1 && m_gpu_pstate_item) return m_gpu_pstate_item;

    return nullptr;
}

// UPDATED: DataRequired calls the new combined function
void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
    UpdateGpuStatus();
}

const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_VERSION: return L"2.2.0"; // Version bump
    // ... (other info)
    }
}

// UPDATED: InitNVML loads the new function
void CCPUCoreBarsPlugin::InitNVML()
{
    m_nvml_dll = LoadLibrary(L"nvml.dll");
    if (!m_nvml_dll) return;

    pfn_nvmlInit = (decltype(pfn_nvmlInit))GetProcAddress(m_nvml_dll, "nvmlInit_v2");
    pfn_nvmlShutdown = (decltype(pfn_nvmlShutdown))GetProcAddress(m_nvml_dll, "nvmlShutdown");
    pfn_nvmlDeviceGetHandleByIndex = (decltype(pfn_nvmlDeviceGetHandleByIndex))GetProcAddress(m_nvml_dll, "nvmlDeviceGetHandleByIndex_v2");
    pfn_nvmlDeviceGetCurrentClocksThrottleReasons = (decltype(pfn_nvmlDeviceGetCurrentClocksThrottleReasons))GetProcAddress(m_nvml_dll, "nvmlDeviceGetCurrentClocksThrottleReasons");
    pfn_nvmlDeviceGetPerformanceState = (decltype(pfn_nvmlDeviceGetPerformanceState))GetProcAddress(m_nvml_dll, "nvmlDeviceGetPerformanceState");

    if (!pfn_nvmlInit || !pfn_nvmlShutdown || !pfn_nvmlDeviceGetHandleByIndex || 
        !pfn_nvmlDeviceGetCurrentClocksThrottleReasons || !pfn_nvmlDeviceGetPerformanceState) // Check new function
    {
        ShutdownNVML();
        return;
    }

    if (pfn_nvmlInit() != NVML_SUCCESS) { ShutdownNVML(); return; }
    if (pfn_nvmlDeviceGetHandleByIndex(0, &m_nvml_device) != NVML_SUCCESS) { ShutdownNVML(); return; }

    m_nvml_initialized = true;
    m_gpu_limit_item = new CNvidiaLimitReasonItem();
    m_gpu_pstate_item = new CNvidiaPStateItem(); // Create new item
}

void CCPUCoreBarsPlugin::ShutdownNVML() { /* ... (no changes) ... */ }

// UPDATED: Combined GPU update function
void CCPUCoreBarsPlugin::UpdateGpuStatus()
{
    if (!m_nvml_initialized) return;

    // --- Step 1: Get Throttle Reasons ---
    unsigned long long reasons = 0;
    wchar_t reason_text[128] = L"错误"; // Default to error
    bool reasons_ok = (pfn_nvmlDeviceGetCurrentClocksThrottleReasons(m_nvml_device, &reasons) == NVML_SUCCESS);

    if (reasons_ok)
    {
        if (reasons & nvmlClocksThrottleReasonHwThermalSlowdown) {
            wcscpy_s(reason_text, L"硬过热");
        } else if (reasons & nvmlClocksThrottleReasonHwPowerBrakeSlowdown) {
            wcscpy_s(reason_text, L"硬功耗");
        } else if (reasons & nvmlClocksThrottleReasonSwPowerCap) {
            wcscpy_s(reason_text, L"软功耗");
        } else if (reasons & nvmlClocksThrottleReasonSwThermalSlowdown) {
            wcscpy_s(reason_text, L"软过热");
        } else if (reasons & nvmlClocksThrottleReasonGpuIdle) {
            wcscpy_s(reason_text, L"空闲");
        } else if (reasons == nvmlClocksThrottleReasonApplicationsClocksSetting) {
            wcscpy_s(reason_text, L"无限制");
        } else {
            wcscpy_s(reason_text, L"无");
        }
    }
    
    // Update the limit reason item
    if (m_gpu_limit_item)
    {
        m_gpu_limit_item->SetValue(reason_text);
    }

    // --- Step 2: Get P-State ---
    nvmlPstate_t pstate = NVML_PSTATE_UNKNOWN;
    bool pstate_ok = (pfn_nvmlDeviceGetPerformanceState(m_nvml_device, &pstate) == NVML_SUCCESS);

    // Update the P-State item
    if (m_gpu_pstate_item)
    {
        // If getting P-State fails, use the reason text we already have (e.g., "Error")
        // If it succeeds, also use the reason text. They are linked.
        m_gpu_pstate_item->SetValue(pstate_ok ? pstate : NVML_PSTATE_UNKNOWN, reason_text);
    }
}

// --- Other functions (UpdateCpuUsage, DetectCoreTypes, etc.) are unchanged ---
// ... (omitted for brevity)

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}