// CPUCoreBars/CPUCoreBars.h - 性能优化版本
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
// GDI+ headers must be included after windows.h
#include <gdiplus.h> 
#include "PluginInterface.h"
#include "nvml.h"

using namespace Gdiplus;

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
    
    // 新增：内联函数声明
    inline COLORREF CalculateBarColor() const;
    
    // 原有成员变量
    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core;
    
    // 新增：静态字体缓存
    static HFONT s_symbolFont;
    static int s_fontRefCount;
    
    // 新增：GDI对象缓存
    mutable HBRUSH m_cachedBgBrush;
    mutable HBRUSH m_cachedBarBrush;
    mutable COLORREF m_lastBgColor;
    mutable COLORREF m_lastBarColor;
    mutable bool m_lastDarkMode;
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
    // 新增：内联函数声明
    inline COLORREF CalculateTextColor(bool dark_mode) const;
    
    // 原有成员变量
    wchar_t m_value_text[128];
    int m_width = 100;
    bool m_has_system_error = false;
    
    // 新增：Graphics对象缓存
    mutable Graphics* m_cachedGraphics;
    mutable HDC m_lastHdc;
};

// =================================================================
// Temperature Item (for CPU and GPU)
// =================================================================
class CTempMonitorItem : public IPluginItem
{
public:
    CTempMonitorItem(const wchar_t* name, const wchar_t* id, const wchar_t* label);
    virtual ~CTempMonitorItem() = default;

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;

    void SetValue(int temp);

private:
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    wchar_t m_label[16];
    wchar_t m_value_text[32];
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
    const wchar_