#pragma once

#include <windows.h>
#include <pdh.h>
#include <vector>

// 假设的插件接口定义（根据常见的插件架构）
class IPluginItem
{
public:
    virtual ~IPluginItem() = default;
    virtual const wchar_t* GetItemName() const = 0;
    virtual const wchar_t* GetItemId() const = 0;
    virtual const wchar_t* GetItemLableText() const = 0;
    virtual const wchar_t* GetItemValueText() const = 0;
    virtual const wchar_t* GetItemValueSampleText() const = 0;
    virtual bool IsCustomDraw() const = 0;
    virtual int GetItemWidth() const = 0;
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) = 0;
};

enum PluginInfoIndex
{
    TMI_NAME,
    TMI_DESCRIPTION,
    TMI_AUTHOR,
    TMI_COPYRIGHT,
    TMI_URL,
    TMI_VERSION
};

class ITMPlugin
{
public:
    virtual ~ITMPlugin() = default;
    virtual int GetItemCount() = 0;
    virtual IPluginItem* GetItem(int index) = 0;
    virtual void DataRequired() = 0;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) = 0;
};

class CCpuUsageItem : public IPluginItem
{
private:
    int m_core_index;
    bool m_is_e_core;
    double m_usage = 0.0;
    wchar_t m_item_name[64];
    wchar_t m_item_id[64];

public:
    CCpuUsageItem(int core_index, bool is_e_core);
    
    // IPluginItem interface
    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;
    virtual bool IsCustomDraw() const override;
    virtual int GetItemWidth() const override;
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;
    
    // 特有方法
    void SetUsage(double usage);
    
    // UPDATED: 添加 DrawLeafIcon 方法声明
    void DrawLeafIcon(HDC hDC, const RECT& rect, bool dark_mode);
};

class CCPUCoreBarsPlugin : public ITMPlugin
{
private:
    std::vector<CCpuUsageItem*> m_items;
    std::vector<BYTE> m_core_efficiency;
    int m_num_cores = 0;
    HQUERY m_query = nullptr;
    std::vector<HCOUNTER> m_counters;
    
    void DetectCoreTypes();
    void UpdateCpuUsage();

public:
    CCPUCoreBarsPlugin();
    virtual ~CCPUCoreBarsPlugin();
    
    static CCPUCoreBarsPlugin& Instance();
    
    // ITMPlugin interface
    virtual int GetItemCount() override { return static_cast<int>(m_items.size()); }
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
};