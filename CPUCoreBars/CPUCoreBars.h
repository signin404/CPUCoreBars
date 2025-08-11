// CPUCoreBars/CPUCoreBars.h (更新后)
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
#include "PluginInterface.h"

class CCpuUsageItem : public IPluginItem
{
public:
    CCpuUsageItem(int core_index);
    virtual ~CCpuUsageItem() = default;

    // --- 现有接口 ---
    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

    // --- 新增颜色接口的实现声明 ---
    COLORREF GetItemColor() const override;
    void SetItemColor(COLORREF color) override;

    void SetUsage(double usage);

private:
    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    COLORREF m_color; // <--- 新增：用于存储当前核心的颜色
};

// CCPUCoreBarsPlugin 类保持不变
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
    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
};