// CPUCoreBars/CPUCoreBars.h - Correct
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

// IMPORTANT: Import the managed assembly here so all types are fully defined.
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "gdiplus.lib")
#using <System.dll>
#using "LibreHardwareMonitorLib.dll"

// Use namespaces to simplify type names
using namespace Gdiplus;
using namespace LibreHardwareMonitor::Hardware;

// ... (rest of the file is unchanged) ...
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
    
    // ... (rest of the private members are unchanged) ...

    // Correctly qualify your custom UpdateVisitor with its namespace
    gcroot<HardwareMonitor::UpdateVisitor^> m_updateVisitor;
    CHardwareMonitorItem* m_cpu_temp_item = nullptr;
    CHardwareMonitorItem* m_gpu_temp_item = nullptr;
};