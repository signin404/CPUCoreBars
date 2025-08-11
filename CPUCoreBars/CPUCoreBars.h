// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include "PluginInterface.h"

// --- 定义 NtQuerySystemInformation 所需的结构和类型 ---

// 这个结构包含了单个处理器的性能时间信息
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

// 定义我们要查询的信息类别
#define SystemProcessorPerformanceInformation 8

// 定义函数指针类型
typedef LONG(WINAPI* pNtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

// 修复：添加NTSTATUS类型定义
typedef LONG NTSTATUS;

class CCpuUsageItem : public IPluginItem
{
public:
    CCpuUsageItem(int core_index);
    virtual ~CCpuUsageItem() = default;

    // IPluginItem接口实现
    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

    // 修复：添加资源使用图相关函数
    int IsDrawResourceUsageGraph() const override { return 1; } // 启用资源占用图
    float GetResourceUsageGraphValue() const override { return static_cast<float>(m_usage); }

    // 设置CPU使用率
    void SetUsage(double usage);

    // 修复：添加获取核心索引的方法，用于调试
    int GetCoreIndex() const { return m_core_index; }

private:
    int m_core_index;           // 实际核心索引（从0开始）
    double m_usage = 0.0;       // CPU使用率（0.0-1.0）
    wchar_t m_item_name[32];    // 显示名称（如"CPU Core 1"）
    wchar_t m_item_id[32];      // 唯一ID（如"cpu_core_0"）
};

class CCPUCoreBarsPlugin : public ITMPlugin
{
public:
    static CCPUCoreBarsPlugin& Instance();

    // ITMPlugin接口实现
    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;

    // 修复：添加工具提示信息
    const wchar_t* GetTooltipInfo() override;

    // 修复：添加获取项目数量的方法
    int GetItemCount() const { return static_cast<int>(m_items.size()); }

private:
    CCPUCoreBarsPlugin();
    ~CCPUCoreBarsPlugin();
    
    // 禁用拷贝构造和赋值
    CCPUCoreBarsPlugin(const CCPUCoreBarsPlugin&) = delete;
    CCPUCoreBarsPlugin& operator=(const CCPUCoreBarsPlugin&) = delete;

    // 更新CPU使用率数据
    void UpdateCpuUsage();

    // 成员变量
    std::vector<CCpuUsageItem*> m_items;                // CPU核心项目列表
    
    // --- NtQuerySystemInformation相关成员变量 ---
    pNtQuerySystemInformation m_pNtQuerySystemInformation = nullptr;
    int m_num_cores = 0;                                // CPU核心数量
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> m_last_perf_info;  // 上次性能信息
    bool m_is_first_run = true;                         // 是否首次运行
};

// 修复：添加调试用的内联函数
inline int GetCPUCoreCount()
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return static_cast<int>(sys_info.dwNumberOfProcessors);
}