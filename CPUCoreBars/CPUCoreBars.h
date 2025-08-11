// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
#include "PluginInterface.h"

class CCpuUsageItem : public IPluginItem
{
public:
    // 构造函数现在需要知道核心类型
    CCpuUsageItem(int core_index, bool is_e_core);
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
    // 新增：手绘树叶图标的辅助函数
    void DrawLeafIcon(HDC hDC, const RECT& rect, bool dark_mode);

    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core; // <--- 新增：标记是否为 E-Core
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
    ~CCPUCoreBarsPlugin();
    CCPUCoreBarsPlugin(const CCPUCoreBarsPlugin&) = delete;
    CCPUCoreBarsPlugin& operator=(const CCPUCoreBarsPlugin&) = delete;

    void UpdateCpuUsage();
    void DetectCoreTypes(); // <--- 新增：检测核心类型的函数

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;

    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;

    // <--- 新增：存储每个逻辑核心的效率等级 (0=E-Core, >0=P-Core)
    std::vector<BYTE> m_core_efficiency;
};