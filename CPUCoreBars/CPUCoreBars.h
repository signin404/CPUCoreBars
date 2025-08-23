// CPUCoreBars/CPUCoreBars.h - 性能优化版本 (ETW + NVML ECC)
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
#include <evntrace.h> // For ETW
#include <evntcons.h> // For ETW

// GDI+ headers must be included after windows.h
#include <gdiplus.h> 
#include "PluginInterface.h"
#include "nvml.h"

using namespace Gdiplus;

// =================================================================
// CPU Core Item - 优化版本 (No changes in this class)
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
// GPU / System Error Combined Item - 优化版本 (MODIFIED)
// =================================================================
class CNvidiaMonitorItem : public IPluginItem
{
public:
    // NEW: System status enumeration
    enum class SystemStatus { Ok, Warning, Error };

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
    // MODIFIED: Use the new enum for setting status
    void SetSystemStatus(SystemStatus status);

private:
    inline COLORREF CalculateTextColor(bool dark_mode) const;
    
    wchar_t m_value_text[128];
    int m_width = 100;
    // MODIFIED: Replaced bool with the new enum
    SystemStatus m_system_status = SystemStatus::Ok;
    
    mutable Graphics* m_cachedGraphics;
    mutable HDC m_lastHdc;
};


// =================================================================
// Main Plugin Class - 优化版本 (MODIFIED)
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
    // NEW: Combined status update function
    void UpdateSystemStatus();

    // NEW: ETW listener functions
    void StartWheaEtwListener();
    void StopWheaEtwListener();
    static DWORD WINAPI EtwThreadProc(LPVOID lpParam);
    static VOID WINAPI EtwEventCallback(PEVENT_RECORD pEventRecord);

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;
    CNvidiaMonitorItem* m_gpu_item = nullptr;
    bool m_nvml_initialized = false;
    HMODULE m_nvml_dll = nullptr;
    nvmlDevice_t m_nvml_device;
    
    ULONG_PTR m_gdiplusToken;

    // NVML Function Pointers
    decltype(nvmlInit_v2)* pfn_nvmlInit;
    decltype(nvmlShutdown)* pfn_nvmlShutdown;
    decltype(nvmlDeviceGetHandleByIndex_v2)* pfn_nvmlDeviceGetHandleByIndex;
    decltype(nvmlDeviceGetCurrentClocksThrottleReasons)* pfn_nvmlDeviceGetCurrentClocksThrottleReasons;
    // NEW: NVML ECC function pointer
    decltype(nvmlDeviceGetTotalEccErrors)* pfn_nvmlDeviceGetTotalEccErrors;

    // NEW: ETW member variables
    HANDLE m_etw_thread_handle = nullptr;
    TRACEHANDLE m_etw_session_handle = 0;
    volatile bool m_has_whea_error = false;
    static CCPUCoreBarsPlugin* s_instance; // Static instance pointer for the callback

    // NEW: NVML ECC counters to track new errors
    unsigned long long m_last_corrected_ecc_count = 0;
    unsigned long long m_last_uncorrected_ecc_count = 0;
};