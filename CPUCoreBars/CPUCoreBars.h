#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
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
// NVIDIA GPU Limit Reason Item
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

private:
    wchar_t m_value_text[128];
};

// =================================================================
// NVIDIA P-State Item
// =================================================================
class CNvidiaPStateItem : public IPluginItem
{
public:
    CNvidiaPStateItem();
    virtual ~CNvidiaPStateItem() = default;

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
    void UpdateGpuStatus();

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;

    CNvidiaLimitReasonItem* m_gpu_limit_item = nullptr;
    CNvidiaPStateItem* m_gpu_pstate_item = nullptr;

    bool m_nvml_initialized = false;
    HMODULE m_nvml_dll = nullptr;
    nvmlDevice_t m_nvml_device;

    decltype(nvmlInit_v2)* pfn_nvmlInit;
    decltype(nvmlShutdown)* pfn_nvmlShutdown;
    decltype(nvmlDeviceGetHandleByIndex_v2)* pfn_nvmlDeviceGetHandleByIndex;
    decltype(nvmlDeviceGetCurrentClocksThrottleReasons)* pfn_nvmlDeviceGetCurrentClocksThrottleReasons;
    decltype(nvmlDeviceGetPerformanceState)* pfn_nvmlDeviceGetPerformanceState;
};