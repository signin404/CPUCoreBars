// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h> // <--- 添加 PDH 头文件
#include "PluginInterface.h"

class CCpuUsageItem : public IPluginItem
{
public:
    CCpuUsageItem(int core_index);
    virtual ~CCpuUsageItem() = default;

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
    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
};

class CCPUCoreBarsPlugin : public ITMPlugin
{
public:
    static CCPUCoreBarsPlugin& Instance();

    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;

private:
    CCPUCoreBarsPlugin();
    ~CCPUCoreBarsPlugin(); // <--- 需要实现析构函数来清理资源
    CCPUCoreBarsPlugin(const CCPUCoreBarsPlugin&) = delete;
    CCPUCoreBarsPlugin& operator=(const CCPUCoreBarsPlugin&) = delete;

    void UpdateCpuUsage();

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;

    // --- 新增的 PDH 成员变量 ---
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
};