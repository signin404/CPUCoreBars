// CPUCoreBars/CPUCoreBars.h - 完全优化版本
#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <Pdh.h>
#include <gdiplus.h>
#include "PluginInterface.h"
#include "nvml.h"

using namespace Gdiplus;

// 编译器优化
#ifdef _MSC_VER
#pragma optimize("gt", on)
#endif

// =================================================================
// GPU状态枚举 - 优化字符串比较
// =================================================================
enum class GpuStatus : int {
    Overheat = 0,
    PowerLimit = 1,
    Idle = 2,
    Unlimited = 3,
    None = 4,
    Error = 5
};

// =================================================================
// Graphics对象池 - 优化GDI+对象管理
// =================================================================
class GraphicsPool {
private:
    struct GraphicsWrapper {
        std::unique_ptr<Graphics> graphics;
        bool inUse = false;
        HDC hdc = nullptr;
    };
    std::vector<GraphicsWrapper> m_pool;
    mutable std::mutex m_mutex;

public:
    Graphics* Acquire(HDC hdc);
    void Release(Graphics* g);
    static GraphicsPool& Instance();
};

// =================================================================
// 事件日志监控器 - 异步查询优化
// =================================================================
class EventLogMonitor {
private:
    std::thread m_workerThread;
    std::atomic<bool> m_running{true};
    std::atomic<DWORD> m_wheaCount{0};
    std::atomic<DWORD> m_nvlddmkmCount{0};
    
    DWORD QueryEventLogCount(LPCWSTR provider_name);
    void MonitoringThread();

public:
    EventLogMonitor();
    ~EventLogMonitor();
    void StartMonitoring();
    void StopMonitoring();
    DWORD GetWheaCount() const { return m_wheaCount.load(); }
    DWORD GetNvlddmkmCount() const { return m_nvlddmkmCount.load(); }
};

// =================================================================
// CPU Core Item - 完全优化版本
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

    __forceinline void SetUsage(double usage) {
        m_usage = max(0.0, min(1.0, usage));
    }

private:
    void DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode);
    __forceinline COLORREF CalculateBarColor() const;
    void EnsureBackBuffer(int w, int h);
    
    // 成员变量
    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core;
    
    // 静态字体缓存
    static HFONT s_symbolFont;
    static int s_fontRefCount;
    
    // GDI对象缓存
    mutable HBRUSH m_cachedBgBrush;
    mutable HBRUSH m_cachedBarBrush;
    mutable COLORREF m_lastBgColor;
    mutable COLORREF m_lastBarColor;
    mutable bool m_lastDarkMode;
    
    // 双缓冲
    HDC m_memDC = nullptr;
    HBITMAP m_memBitmap = nullptr;
    int m_lastWidth = 0;
    int m_lastHeight = 0;
    
    // 颜色查找表
    static constexpr struct {
        double threshold;
        COLORREF color;
    } s_colorMap[] = {
        {0.9, RGB(217, 66, 53)},   // 红色
        {0.5, RGB(246, 182, 78)},  // 橙色
        {0.0, RGB(118, 202, 83)}   // 默认绿色
    };
};

// =================================================================
// GPU / System Error Combined Item - 完全优化版本
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

    void SetStatus(GpuStatus status);
    void SetSystemErrorStatus(bool has_error);

private:
    __forceinline COLORREF CalculateTextColor(bool dark_mode) const;
    
    // 成员变量
    GpuStatus m_status = GpuStatus::None;
    int m_width = 100;
    bool m_has_system_error = false;
    
    // 状态字符串表
    static const wchar_t* s_statusStrings[];
    static const COLORREF s_statusColors[];
    
    // 常量
    static constexpr int ICON_SIZE_REDUCTION = 2;
    static constexpr int LEFT_MARGIN = 2;
    static constexpr int TEXT_SPACING = 4;
};

// =================================================================
// Main Plugin Class - 完全优化版本
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
    
    // 优化后的函数
    void UpdateCpuUsage();
    void DetectCoreTypes();
    void InitNVML();
    void ShutdownNVML();
    void UpdateGpuLimitReason();

    // 成员变量 - 使用智能指针
    std::vector<std::unique_ptr<CCpuUsageItem>> m_items;
    std::unique_ptr<CNvidiaMonitorItem> m_gpu_item;
    std::unique_ptr<EventLogMonitor> m_eventLogMonitor;
    
    // 性能数据
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;
    std::vector<PDH_FMT_COUNTERVALUE> m_counterValues; // 批量查询缓存
    
    // NVML相关
    bool m_nvml_initialized = false;
    HMODULE m_nvml_dll = nullptr;
    nvmlDevice_t m_nvml_device;
    
    // GDI+
    ULONG_PTR m_gdiplusToken;
    
    // 缓冲区重用
    std::vector<char> m_procInfoBuffer;
    
    // NVML函数指针
    decltype(nvmlInit_v2)* pfn_nvmlInit;
    decltype(nvmlShutdown)* pfn_nvmlShutdown;
    decltype(nvmlDeviceGetHandleByIndex_v2)* pfn_nvmlDeviceGetHandleByIndex;
    decltype(nvmlDeviceGetCurrentClocksThrottleReasons)* pfn_nvmlDeviceGetCurrentClocksThrottleReasons;
};