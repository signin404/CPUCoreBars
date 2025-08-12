#pragma once
#include "PluginInterface.h"
#include <windows.h>
#include <vector>
#include <string>
#include <Pdh.h>

class CCpuUsageItem : public IPluginItem
{
public:
    CCpuUsageItem(int core_index, bool is_e_core);
    virtual ~CCpuUsageItem() = default;

    // FIX: Add __stdcall to all overrides
    const wchar_t* __stdcall GetItemName() const override;
    const wchar_t* __stdcall GetItemId() const override;
    const wchar_t* __stdcall GetItemLableText() const override;
    const wchar_t* __stdcall GetItemValueText() const override;
    const wchar_t* __stdcall GetItemValueSampleText() const override;
    bool __stdcall IsCustomDraw() const override;
    int __stdcall GetItemWidth() const override;
    void __stdcall DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

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

    // FIX: Add __stdcall to all overrides
    IPluginItem* __stdcall GetItem(int index) override;
    void __stdcall DataRequired() override;
    const wchar_t* __stdcall GetInfo(PluginInfoIndex index) override;

    void ShowSettingWindow(void* hParent);
    std::vector<COLORREF> m_core_colors;
    void SaveSettings();

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