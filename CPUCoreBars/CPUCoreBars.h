// CPUCoreBars/CPUCoreBars.h - 性能优化版本
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
// GDI+ headers must be included after windows.h
#include <gdiplus.h> 
#include "PluginInterface.h"
#include "nvml.h"
// WMI headers for temperature monitoring
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")


using namespace Gdiplus;

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
    
    // 新增：内联函数声明
    inline COLORREF CalculateBarColor() const;
    
    // 原有成员变量
    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core;
    
    // 新增：静态字体缓存
    static HFONT s_symbolFont;
    static int s_fontRefCount;
    
    // 新增：GDI对象缓存
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
    // 新增：内联函数声明
    inline COLORREF CalculateTextColor(bool dark_mode) const;
    
    // 原有成员变量
    wchar_t m_value_text[128];
    int m_width = 100;
    bool m_has_system_error = false;
    
    // 新增：Graphics对象缓存
    mutable Graphics* m_cachedGraphics;
    mutable HDC m_lastHdc;
};

// =================================================================
// CPU Temperature Item
// =================================================================
class CCpuTemperatureItem : public IPluginItem
{
public:
    CCpuTemperatureItem();
    virtual ~CCpuTemperatureItem() = default;

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;

    bool IsCustomDraw() const override { return false; } // Use default text drawing
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override {}; // Not used if IsCustomDraw is false

    void SetTemperature(int temp_celsius);
    COLORREF GetItemValueColor(bool dark_mode) const override;

private:
    wchar_t m_value_text[16];
    int m_width;
    int m_temperature = -1; // In Celsius
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

private:
    CCPUCoreBarsPlugin();
    ~CCPUCoreBarsPlugin();
    CCPUCoreBarsPlugin(const CCPUCoreBarsPlugin&) = delete;
    CCPUCoreBarsPlugin& operator=(const CCPUCoreBarsPlugin&) = delete;
    
    // 原有函数
    void UpdateCpuUsage();
    void DetectCoreTypes();
    void InitNVML();
    void ShutdownNVML();
    void UpdateGpuLimitReason();
    void UpdateWheaErrorCount();
    void UpdateNvlddmkmErrorCount();
    void InitWMI();
    void ShutdownWMI();
    void UpdateCpuTemperature();
    
    // 新增：优化的事件日志查询函数
    DWORD QueryEventLogCount(LPCWSTR provider_name);

    // 原有成员变量
    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;
    CNvidiaMonitorItem* m_gpu_item = nullptr;
    CCpuTemperatureItem* m_temp_item = nullptr; // New item
    bool m_nvml_initialized = false;
    HMODULE m_nvml_dll = nullptr;
    nvmlDevice_t m_nvml_device;
    int m_whea_error_count = 0;
    int m_nvlddmkm_error_count = 0;

    ULONG_PTR m_gdiplusToken;
    
    // WMI members for temperature
    IWbemServices* m_pWbemSvc = nullptr;

    decltype(nvmlInit_v2)* pfn_nvmlInit;
    decltype(nvmlShutdown)* pfn_nvmlShutdown;
    decltype(nvmlDeviceGetHandleByIndex_v2)* pfn_nvmlDeviceGetHandleByIndex;
    decltype(nvmlDeviceGetCurrentClocksThrottleReasons)* pfn_nvmlDeviceGetCurrentClocksThrottleReasons;
    
    // 新增：事件日志查询缓存和频率控制
    DWORD m_cached_whea_count;
    DWORD m_cached_nvlddmkm_count;
    DWORD m_last_error_check_time;
    static const DWORD ERROR_CHECK_INTERVAL_MS = 60000; // 60秒检查间隔
};