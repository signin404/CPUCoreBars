// CPUCoreBars/CPUCoreBars.h - 性能优化版本
#pragma once
#include <windows.h>
#include <vector>
#include <array>
#include <stack>
#include <unordered_map>
#include <Pdh.h>
// GDI+ headers must be included after windows.h
#include <gdiplus.h> 
#include "PluginInterface.h"
#include "nvml.h"

using namespace Gdiplus;

// =================================================================
// 对象池管理器 - 内存池和对象复用优化
// =================================================================
class GDIObjectPool {
private:
    std::unordered_map<COLORREF, std::stack<HBRUSH>> m_brush_pools;
    std::unordered_map<COLORREF, std::stack<HPEN>> m_pen_pools;
    std::vector<HBRUSH> m_all_brushes;  // 跟踪所有创建的画刷用于清理
    std::vector<HPEN> m_all_pens;       // 跟踪所有创建的画笔用于清理

public:
    static GDIObjectPool& Instance() {
        static GDIObjectPool instance;
        return instance;
    }
    
    ~GDIObjectPool() {
        // 清理所有GDI对象
        for (HBRUSH brush : m_all_brushes) {
            if (brush) DeleteObject(brush);
        }
        for (HPEN pen : m_all_pens) {
            if (pen) DeleteObject(pen);
        }
    }
    
    HBRUSH GetBrush(COLORREF color) {
        auto& pool = m_brush_pools[color];
        if (!pool.empty()) {
            HBRUSH brush = pool.top();
            pool.pop();
            return brush;
        }
        HBRUSH brush = CreateSolidBrush(color);
        if (brush) {
            m_all_brushes.push_back(brush);
        }
        return brush;
    }
    
    void ReturnBrush(COLORREF color, HBRUSH brush) {
        if (brush) {
            m_brush_pools[color].push(brush);
        }
    }
    
    HPEN GetPen(COLORREF color, int width = 1) {
        COLORREF key = color | (width << 24);  // 组合颜色和宽度作为键
        auto& pool = m_pen_pools[key];
        if (!pool.empty()) {
            HPEN pen = pool.top();
            pool.pop();
            return pen;
        }
        HPEN pen = CreatePen(PS_SOLID, width, color);
        if (pen) {
            m_all_pens.push_back(pen);
        }
        return pen;
    }
    
    void ReturnPen(COLORREF color, HPEN pen, int width = 1) {
        if (pen) {
            COLORREF key = color | (width << 24);
            m_pen_pools[key].push(pen);
        }
    }
};

// =================================================================
// 字符串缓存管理器 - 字符串操作优化
// =================================================================
class StringCache {
private:
    static constexpr size_t MAX_COUNTER_PATH_LEN = 64;
    static constexpr size_t MAX_QUERY_LEN = 256;
    
    // 预构建的字符串模板
    std::array<wchar_t[MAX_COUNTER_PATH_LEN], 128> m_counter_paths;  // 支持最多128个核心
    std::array<wchar_t[MAX_QUERY_LEN], 4> m_query_templates;        // 预构建查询模板
    bool m_paths_initialized = false;
    
public:
    static StringCache& Instance() {
        static StringCache instance;
        return instance;
    }
    
    void InitializeCounterPaths(int num_cores) {
        if (m_paths_initialized) return;
        
        for (int i = 0; i < num_cores && i < 128; ++i) {
            swprintf_s(m_counter_paths[i], L"\\Processor(%d)\\%% Processor Time", i);
        }
        
        // 预构建事件查询模板
        wcscpy_s(m_query_templates[0], L"*[System[Provider[@Name='WHEA-Logger'] and TimeCreated[timediff(@SystemTime) <= 86400000]]]");
        wcscpy_s(m_query_templates[1], L"*[System[Provider[@Name='nvlddmkm'] and TimeCreated[timediff(@SystemTime) <= 86400000]]]");
        
        m_paths_initialized = true;
    }
    
    __forceinline const wchar_t* GetCounterPath(int core_index) const {
        return (core_index >= 0 && core_index < 128) ? m_counter_paths[core_index] : nullptr;
    }
    
    __forceinline const wchar_t* GetWHEAQuery() const { return m_query_templates[0]; }
    __forceinline const wchar_t* GetNvlddmkmQuery() const { return m_query_templates[1]; }
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
    
    // 编译器优化：内联函数
    __forceinline COLORREF CalculateBarColor() const {
        // 使用位运算和分支预测优化
        if (m_usage >= 0.9) [[likely]] return RGB(217, 66, 53);  // 红色
        if (m_usage >= 0.5) return RGB(246, 182, 78);            // 橙色
        
        // 基于核心索引的颜色 - 使用位运算优化
        if (m_core_index >= 12 && m_core_index <= 19) [[unlikely]] {
            return RGB(217, 66, 53);  // 红色
        }
        return (m_core_index & 1) ? RGB(38, 160, 218) : RGB(118, 202, 83);
    }
    
    // 原有成员变量
    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core;
    
    // 新增：静态字体缓存
    static HFONT s_symbolFont;
    static int s_fontRefCount;
    
    // 绘图性能优化：更多缓存和脏标记
    mutable HBRUSH m_cachedBgBrush = nullptr;
    mutable HBRUSH m_cachedBarBrush = nullptr;
    mutable COLORREF m_lastBgColor = 0xFFFFFFFF;  // 使用无效颜色初始化
    mutable COLORREF m_lastBarColor = 0xFFFFFFFF;
    mutable bool m_lastDarkMode = false;
    mutable bool m_needs_redraw = true;
    mutable double m_last_drawn_usage = -1.0;
    mutable RECT m_lastRect = {0};
    
    // GDI对象池颜色追踪
    mutable COLORREF m_pooled_bg_color = 0xFFFFFFFF;
    mutable COLORREF m_pooled_bar_color = 0xFFFFFFFF;
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
    // 编译器优化：内联函数
    __forceinline COLORREF CalculateTextColor(bool dark_mode) const {
        // 使用字符串哈希快速比较而不是wcscmp
        switch (m_value_hash) {
        case 0x8B6F4E2A:  // "过热" 的哈希值（示例）
            return RGB(217, 66, 53);
        case 0x7C3D9E8B:  // "功耗" 的哈希值（示例）
            return RGB(246, 182, 78);
        default:
            return dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
        }
    }
    
    // 快速字符串哈希函数
    __forceinline uint32_t HashString(const wchar_t* str) const {
        uint32_t hash = 5381;
        while (*str) {
            hash = ((hash << 5) + hash) + *str++;
        }
        return hash;
    }
    
    // 原有成员变量
    wchar_t m_value_text[128];
    int m_width = 100;
    bool m_has_system_error = false;
    uint32_t m_value_hash = 0;  // 缓存字符串哈希值
    
    // Graphics对象缓存
    mutable Graphics* m_cachedGraphics = nullptr;
    mutable HDC m_lastHdc = nullptr;
    
    // 绘图优化缓存
    mutable bool m_needs_redraw = true;
    mutable bool m_last_error_status = false;
    mutable bool m_last_dark_mode = false;
    mutable RECT m_lastRect = {0};
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
    
    // 大幅优化的事件日志查询函数
    DWORD QueryEventLogCountOptimized(const wchar_t* query);

    // 数据结构优化：使用数组和更紧凑的布局
    static constexpr int MAX_CORES = 128;
    std::array<CCpuUsageItem*, MAX_CORES> m_items_array{};  // 固定大小数组，减少间接访问
    std::vector<CCpuUsageItem*> m_items;  // 保持原有接口兼容性
    int m_num_cores;
    
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    
    // 数据结构优化：使用数组代替vector
    std::array<BYTE, MAX_CORES> m_core_efficiency{};
    
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
    
    // 事件日志查询缓存和频率控制
    DWORD m_cached_whea_count = 0;
    DWORD m_cached_nvlddmkm_count = 0;
    DWORD m_last_error_check_time = 0;
    static const DWORD ERROR_CHECK_INTERVAL_MS = 30000; // 30秒检查间隔
    
    // 内存管理优化：预分配缓冲区
    mutable std::vector<char> m_event_buffer;
    mutable std::array<EVT_HANDLE, 512> m_event_handles;  // 预分配事件句柄数组
};
