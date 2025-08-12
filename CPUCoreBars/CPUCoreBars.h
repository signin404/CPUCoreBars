// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include <string>       // <--- FIX: 包含 <string> 以使用 std::wstring
#include <Pdh.h>
#include "PluginInterface.h"

class CCpuUsageItem : public IPluginItem
{
public:
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
    void SetColor(COLORREF color);

private:
    void DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode);

    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core;
    COLORREF m_color;
};

class CCPUCoreBarsPlugin : public ITMPlugin
{
public:
    static CCPUCoreBarsPlugin& Instance();

    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    void ShowSettingWindow(void* hParent) override;

    // --- 公开成员，供对话框访问 ---
    std::vector<COLORREF> m_core_colors;
    void SaveSettings(); // <--- FIX: 移到 public，以便对话框可以调用

private:
    CCPUCoreBarsPlugin();
    ~CCPUCoreBarsPlugin();
    CCPUCoreBarsPlugin(const CCPUCoreBarsPlugin&) = delete;
    CCPUCoreBarsPlugin& operator=(const CCPUCoreBarsPlugin&) = delete;

    void UpdateCpuUsage();
    void DetectCoreTypes();
    void LoadSettings();
    std::wstring GetSettingsPath() const;
    void ApplySettingsToItems();

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    PDH_HQUERY m_query = nullptr;
    std::vector<PDH_HCOUNTER> m_counters;
    std::vector<BYTE> m_core_efficiency;
};