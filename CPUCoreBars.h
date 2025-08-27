// CPUCoreBars/CPUCoreBars.h - 性能优化版本
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
#include <gdiplus.h> 
#include "PluginInterface.h"
#include "nvml.h"
#include "resource.h" // Include resource header for dialog

using namespace Gdiplus;

// =================================================================
// Settings Structure
// =================================================================
struct PluginSettings
{
    // Default values
    DWORD error_check_interval_ms = 60000;
    COLORREF temp_cool = RGB(118, 202, 83);
    COLORREF temp_warm = RGB(246, 182, 78);
    COLORREF temp_hot = RGB(217, 66, 53);
    COLORREF cpu_high_usage = RGB(217, 66, 53);
    COLORREF cpu_med_usage = RGB(246, 182, 78);
    COLORREF cpu_pcore = RGB(38, 160, 218);
    COLORREF cpu_ecore = RGB(118, 202, 83);
    COLORREF gpu_status_error = RGB(217, 66, 53);
    COLORREF gpu_status_ok = RGB(118, 202, 83);
};


// =================================================================
// CPU Core Item - 优化版本
// =================================================================
class CCpuUsageItem : public IPluginItem
{
public:
    CCpuUsageItem(int core_index, bool is_e_core);
    virtual ~CCpuUsageItem();

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
    COLORREF CalculateBarColor() const;
    
    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core;
    
    static HFONT s_symbolFont;
    static int s_fontRefCount;
    
    mutable HBRUSH m_cachedBgBrush;
    mutable HBRUSH m_cachedBarBrush;
    mutable COLORREF m_lastBgColor;
    mutable COLORREF m_lastBarColor;
    mutable bool m_lastDarkMode;
};


// =================================================================
// GPU / System Error Combined Item - 优化版本
// =================================================================
class CNvidiaMonitorItem : public IPluginItem
{
public:
    CNvidiaMonitorItem();
    virtual ~CNvidiaMonitorItem();

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

private:
    inline COLORREF CalculateTextColor(bool dark_mode) const;
    
    wchar_t m_value_text[128];
    int m_width = 100;
    bool m_has_system_error = false;
    
    mutable Graphics* m_cachedGraphics;
    mutable HDC m_lastHdc;
};

// =================================================================
// Temperature Item (for CPU and GPU) - With Custom Colors
// =================================================================
class CTempMonitorItem : public IPluginItem
{
public:
    CTempMonitorItem(const wchar_t* name, const wchar_t* id);
    virtual ~CTempMonitorItem() = default;

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;

    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

    void SetValue(int temp);

private:
    COLORREF GetTemperatureColor() const;

    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    wchar_t m_value_text[32];
    int m_temp = 0;
    int m_width = 0;
};


// =================================================================
// Main Plugin Class - 优化版本
// =================================================================
class CCPUCoreBarsPlugin : public ITMPlugin
{
public:
    static CCPUCoreBarsPlugin& Instance();
    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    void OnMonitorInfo(const ITMPlugin::MonitorInfo& monitor_info) override;
    void OnInitialize(ITrafficMonitor* pApp) override;
    OptionReturn ShowOptionsDialog(void* hParent) override;

    PluginSettings m_settings;

private:
    CCPUCoreBarsPlugin();
    ~CCPUCoreBarsPlugin();
    CCPUCoreBarsPlugin(const CCPUCoreBarsPlugin&) = delete;
    CCPUCoreBarsPlugin& operator=(const CCPUCoreBarsPlugin&) = delete;
    
    void LoadSettings();
    void SaveSettings();

    void UpdateCpuUsage();
    void DetectCoreTypes();
    void InitNVML();
    void ShutdownNVML();
    void UpdateGpuLimitReason();
    void UpdateWheaErrorCount();
    void UpdateNvlddmkmErrorCount();
    DWORD QueryEventLogCount(LPCWSTR provider_name);

    std::vector<IPluginItem*> m_all_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;
    CNvidiaMonitorItem* m_gpu_item = nullptr;
    bool m_nvml_initialized = false;
    HMODULE m_nvml_dll = nullptr;
    nvmlDevice_t m_nvml_device;
    
    CTempMonitorItem* m_cpu_temp_item = nullptr;
    CTempMonitorItem* m_gpu_temp_item = nullptr;
    int m_cpu_temp = 0;
    int m_gpu_temp = 0;

    ULONG_PTR m_gdiplusToken;

    decltype(nvmlInit_v2)* p_nvmlInit;
    decltype(nvmlShutdown)* p_nvmlShutdown;
    decltype(nvmlDeviceGetHandleByIndex_v2)* p_nvmlDeviceGetHandleByIndex;
    decltype(nvmlDeviceGetCurrentClocksThrottleReasons)* p_nvmlDeviceGetCurrentClocksThrottleReasons;
    
    DWORD m_cached_whea_count;
    DWORD m_cached_nvlddmkm_count;
    DWORD m_last_error_check_time;

    wchar_t m_config_file_path[MAX_PATH]{};
    ITrafficMonitor* m_pApp = nullptr;
};