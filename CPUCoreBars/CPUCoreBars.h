// CPUCoreBars/CPUCoreBars.h - Final Corrected Version
#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <Pdh.h>
#include <gdiplus.h> 
#include "PluginInterface.h"
#include "nvml.h"
#include <gcroot.h>

// IMPORTANT: Import the managed assembly here so all types are fully defined.
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "gdiplus.lib")
#using <System.dll>
#using "LibreHardwareMonitorLib.dll"

// Use the namespaces to simplify type names
using namespace Gdiplus;
using namespace LibreHardwareMonitor::Hardware;

// Forward declaration for the UpdateVisitor class
ref class UpdateVisitor;

// =================================================================
// CPU Core Item
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
    inline COLORREF CalculateBarColor() const;
    
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
// GPU / System Error Combined Item
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
// Generic Hardware Monitor Item
// =================================================================
class CHardwareMonitorItem : public IPluginItem
{
public:
    CHardwareMonitorItem(const std::wstring& identifier, const std::wstring& label_text);
    virtual ~CHardwareMonitorItem() = default;

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
    bool IsCustomDraw() const override { return false; }
    int GetItemWidth() const override { return 0; }
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override {}

    void UpdateValue();

private:
    std::wstring m_identifier;
    std::wstring m_name;
    std::wstring m_id;
    std::wstring m_label_text;
    std::wstring m_value_text;
    std::wstring m_sample_text;
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

    // Public member to allow CHardwareMonitorItem to access the computer object
    gcroot<Computer^> m_computer;

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
    void UpdateWheaErrorCount();
    void UpdateNvlddmkmErrorCount();
    void InitHardwareMonitor();
    void ShutdownHardwareMonitor();
    void UpdateHardwareMonitorItems();
    DWORD QueryEventLogCount(LPCWSTR provider_name);

    std::vector<IPluginItem*> m_plugin_items;
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

    ULONG_PTR m_gdiplusToken;

    decltype(nvmlInit_v2)* pfn_nvmlInit;
    decltype(nvmlShutdown)* pfn_nvmlShutdown;
    decltype(nvmlDeviceGetHandleByIndex_v2)* pfn_nvmlDeviceGetHandleByIndex;
    decltype(nvmlDeviceGetCurrentClocksThrottleReasons)* pfn_nvmlDeviceGetCurrentClocksThrottleReasons;
    
    DWORD m_cached_whea_count;
    DWORD m_cached_nvlddmkm_count;
    DWORD m_last_error_check_time;
    static const DWORD ERROR_CHECK_INTERVAL_MS = 60000;

    gcroot<UpdateVisitor^> m_updateVisitor;
    CHardwareMonitorItem* m_cpu_temp_item = nullptr;
    CHardwareMonitorItem* m_gpu_temp_item = nullptr;
};