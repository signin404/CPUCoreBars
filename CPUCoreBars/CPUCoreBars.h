#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
#include "PluginInterface.h"

// =================================================================
// Minimal NVML Definitions
// =================================================================
typedef enum nvmlReturn_enum { NVML_SUCCESS = 0 } nvmlReturn_t;
typedef struct nvmlDevice_st* nvmlDevice_t;
typedef enum nvmlPstate_enum { NVML_PSTATE_0 = 0, NVML_PSTATE_1 = 1, NVML_PSTATE_2 = 2, NVML_PSTATE_3 = 3, NVML_PSTATE_4 = 4, NVML_PSTATE_5 = 5, NVML_PSTATE_6 = 6, NVML_PSTATE_7 = 7, NVML_PSTATE_8 = 8, NVML_PSTATE_9 = 9, NVML_PSTATE_10 = 10, NVML_PSTATE_11 = 11, NVML_PSTATE_12 = 12, NVML_PSTATE_13 = 13, NVML_PSTATE_14 = 14, NVML_PSTATE_15 = 15, NVML_PSTATE_UNKNOWN = 32 } nvmlPstate_t;
#define nvmlClocksThrottleReasonGpuIdle                   0x0000000000000001ULL
#define nvmlClocksThrottleReasonApplicationsClocksSetting 0x0000000000000002ULL
#define nvmlClocksThrottleReasonSwPowerCap                0x0000000000000004ULL
#define nvmlClocksThrottleReasonHwSlowdown                0x0000000000000008ULL
#define nvmlClocksThrottleReasonHwThermalSlowdown         (nvmlClocksThrottleReasonHwSlowdown | 0x0000000000000010ULL)
#define nvmlClocksThrottleReasonHwPowerBrakeSlowdown      (nvmlClocksThrottleReasonHwSlowdown | 0x0000000000000020ULL)
#define nvmlClocksThrottleReasonSwThermalSlowdown         0x0000000000000080ULL
typedef nvmlReturn_t (*nvmlInit_v2_t)(void);
typedef nvmlReturn_t (*nvmlShutdown_t)(void);
typedef nvmlReturn_t (*nvmlDeviceGetHandleByIndex_v2_t)(unsigned int, nvmlDevice_t*);
typedef nvmlReturn_t (*nvmlDeviceGetCurrentClocksThrottleReasons_t)(nvmlDevice_t, unsigned long long*);
typedef nvmlReturn_t (*nvmlDeviceGetPerformanceState_t)(nvmlDevice_t, nvmlPstate_t*);

// =================================================================
// CPU Core Item (no changes)
// =================================================================
class CCpuUsageItem : public IPluginItem { /* ... same as before ... */ };

// =================================================================
// NEW: Merged NVIDIA Status Item
// =================================================================
class CNvidiaStatusItem : public IPluginItem
{
public:
    CNvidiaStatusItem();
    virtual ~CNvidiaStatusItem() = default;
    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;
    void SetValue(nvmlPstate_t pstate, const wchar_t* reason_text);
private:
    wchar_t m_label_text[16];
    wchar_t m_value_text[128];
};

// =================================================================
// Main Plugin Class (updated to use the merged item)
// =================================================================
class CCPUCoreBarsPlugin : public ITMPlugin
{
public:
    static CCPUCoreBarsPlugin& Instance();
    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
private:
    CCPUCoreBarsPlugin();
    ~CCPUCoreBarsPlugin();
    CCPUCoreBarsPlugin(const CCPUCoreBarsPlugin&) = delete;
    CCPUCoreBarsPlugin& operator=(const CCPUCoreBarsPlugin&) = delete;
    void UpdateCpuUsage();
    void DetectCoreTypes();
    void InitNVML();
    void ShutdownNVML();
    void UpdateGpuStatus();
    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;
    CNvidiaStatusItem* m_gpu_status_item = nullptr; // <-- Merged item
    bool m_nvml_initialized = false;
    HMODULE m_nvml_dll = nullptr;
    nvmlDevice_t m_nvml_device;
    nvmlInit_v2_t pfn_nvmlInit;
    nvmlShutdown_t pfn_nvmlShutdown;
    nvmlDeviceGetHandleByIndex_v2_t pfn_nvmlDeviceGetHandleByIndex;
    nvmlDeviceGetCurrentClocksThrottleReasons_t pfn_nvmlDeviceGetCurrentClocksThrottleReasons;
    nvmlDeviceGetPerformanceState_t pfn_nvmlDeviceGetPerformanceState;
};