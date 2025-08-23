// CPUCoreBars/CPUCoreBars.h - 性能优化版本
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
// GDI+ headers must be included after windows.h
#include <gdiplus.h> 
#include "PluginInterface.h"
#include "nvml.h"

// Add missing NVML definitions for XID errors if they are not in the included nvml.h
typedef enum nvmlXidError_enum
{
    NVML_XID_NONE = 0
} nvmlXidError_t;

typedef nvmlReturn_t (*nvmlDeviceGetLastXid_t)(nvmlDevice_t device, nvmlXidError_t* xid_type, unsigned long long* xid_id);


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
    // 修改：更新错误检测函数
    void UpdateWheaErrorFromPerfCounter();
    void UpdateNvmlXidError();

    // 原有成员变量
    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    // 新增：WHEA性能计数器句柄
    PDH_HCOUNTER m_whea_error_counter = nullptr;
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
    // 修改：NVML XID错误检测函数指针
    nvmlDeviceGetLastXid_t pfn_nvmlDeviceGetLastXid;
    
    // 修改：移除事件日志查询相关的缓存
    DWORD m_last_error_check_time;
    static const DWORD ERROR_CHECK_INTERVAL_MS = 60000; // 60秒检查间隔
};