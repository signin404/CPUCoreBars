// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
#include "PluginInterface.h"
#include "nvml.h"

// =================================================================
// CPU Core Item (no changes)
// =================================================================
class CCpuUsageItem : public IPluginItem { /* ... unchanged ... */ };

// =================================================================
// UPDATED: NVIDIA GPU / WHEA Combined Item
// =================================================================
class CNvidiaLimitReasonItem : public IPluginItem
{
public:
    CNvidiaLimitReasonItem();
    virtual ~CNvidiaLimitReasonItem() = default;

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;

    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

    void SetValue(const wchar_t* value);
    void SetWheaCount(int count); // <--- New method to receive WHEA count

private:
    wchar_t m_value_text[128];
    int m_width = 100;
    int m_whea_count = 0; // <--- New member to store WHEA count
};


// =================================================================
// UPDATED: Main Plugin Class
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
    
    // Existing methods
    void UpdateCpuUsage();
    void DetectCoreTypes();
    void InitNVML();
    void ShutdownNVML();
    void UpdateGpuLimitReason();

    // New method for WHEA
    void UpdateWheaErrorCount();

    // Members
    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;
    CNvidiaLimitReasonItem* m_gpu_item = nullptr;
    bool m_nvml_initialized = false;
    HMODULE m_nvml_dll = nullptr;
    nvmlDevice_t m_nvml_device;
    
    // New member for WHEA
    int m_whea_error_count = 0;

    // NVML function pointers (unchanged)
    decltype(nvmlInit_v2)* pfn_nvmlInit;
    decltype(nvmlShutdown)* pfn_nvmlShutdown;
    decltype(nvmlDeviceGetHandleByIndex_v2)* pfn_nvmlDeviceGetHandleByIndex;
    decltype(nvmlDeviceGetCurrentClocksThrottleReasons)* pfn_nvmlDeviceGetCurrentClocksThrottleReasons;
};