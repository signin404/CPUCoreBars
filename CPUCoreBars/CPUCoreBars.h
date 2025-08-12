// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
#include "PluginInterface.h"

class CCpuUsageItem : public IPluginItem
{
public:
    CCpuUsageItem(int core_index, bool is_e_core);
    virtual ~CCpuUsageItem() = default;

    // ... (GetItemName等函数保持不变) ...
    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

    void SetUsage(double usage);
    void SetColor(COLORREF color); // <--- 新增：设置颜色的方法

private:
    void DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode);

    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core;
    COLORREF m_color; // <--- 新增：存储该核心的颜色
};

class CCPUCoreBarsPlugin : public ITMPlugin
{
public:
    static CCPUCoreBarsPlugin& Instance();

    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    
    // <--- 新增：显示设置窗口的接口实现
    virtual void ShowSettingWindow(void* hParent);

    // <--- 新增：公开的颜色向量，供DialogProc访问
    std::vector<COLORREF> m_core_colors;

private:
    CCPUCoreBarsPlugin();
    ~CCPUCoreBarsPlugin();
    CCPUCoreBarsPlugin(const CCPUCoreBarsPlugin&) = delete;
    CCPUCoreBarsPlugin& operator=(const CCPUCoreBarsPlugin&) = delete;

    void UpdateCpuUsage();
    void DetectCoreTypes();

    // <--- 新增：加载和保存设置的函数
    void LoadSettings();
    void SaveSettings();
    std::wstring GetSettingsPath() const;
    void ApplySettingsToItems();

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;
};