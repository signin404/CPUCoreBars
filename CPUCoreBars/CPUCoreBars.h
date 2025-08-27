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
#include "UpdateVisitor.h" // Include your custom visitor header

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "nvml.lib") // <-- ADD THIS LINE to link the NVML library

#using <System.dll>
#using "LibreHardwareMonitorLib.dll"

// Use namespaces to simplify type names
using namespace Gdiplus;
using namespace LibreHardwareMonitor::Hardware;

// ... (The rest of the file is correct and does not need to be changed) ...
class CCPUCoreBarsPlugin : public ITMPlugin
{
public:
    static CCPUCoreBarsPlugin& Instance();
    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
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
    gcroot<HardwareMonitor::UpdateVisitor^> m_updateVisitor;
    CHardwareMonitorItem* m_cpu_temp_item = nullptr;
    CHardwareMonitorItem* m_gpu_temp_item = nullptr;
};