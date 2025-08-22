// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
#include <gdiplus.h> // <-- Include GDI+ headers
#include "PluginInterface.h"
#include "nvml.h"

// =================================================================
// CPU Core Item
// =================================================================
class CCpuUsageItem : public IPluginItem
{
public:
    CCpuUsageItem(int core_index, bool is_e_core);
    virtual ~CCpuUsageItem() = default;

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

    void SetUsage(double usage);

private:
    void DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode);
    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core;
};


// =================================================================
// GPU / System Error Combined Item
// =================================================================
class CNvidiaMonitorItem : public IPluginItem
{
public:
    CNvidiaMonitorItem();
    virtual ~CNvidiaMonitorItem() = default;

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;

    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

    void SetValue(const wchar_t* value);
    void SetSystemErrorStatus(bool has_error);
    void SetPState(nvmlPstates_t p_state);

private:
    wchar_t m_value_text[128];
    int m_width = 100;
    bool m_has_system_error = false;
    nvmlPstates_t m_p_state = NVML_PSTATE_UNKNOWN;
};


// =================================================================
// Main Plugin Class
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
    void UpdateGpuLimitReason();
    void UpdateGpuPState();
    void UpdateWheaErrorCount();
    void UpdateNvlddmkmErrorCount();

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;
    CNvidiaMonitorItem* m_gpu_item = nullptr;
    bool m_nvml_initialized = false;
    HMODULE m_nvml_dll = nullptr;
    nvmlDevice_t m_nvml_device;
    int m_whea_error_count = 0;
    int m_nvlddmkm_error_count = 0;
    nvmlPstates_t m_p_state = NVML_PSTATE_UNKNOWN;

    ULONG_PTR m_gdiplusToken; // <-- GDI+ token

    decltype(nvmlInit_v2)* pfn_nvmlInit;
    decltype(nvmlShutdown)* pfn_nvmlShutdown;
    decltype(nvmlDeviceGetHandleByIndex_v2)* pfn_nvmlDeviceGetHandleByIndex;
    decltype(nvmlDeviceGetCurrentClocksThrottleReasons)* pfn_nvmlDeviceGetCurrentClocksThrottleReasons;
    decltype(nvmlDeviceGetPerformanceState)* pfn_nvmlDeviceGetPerformanceState;
};