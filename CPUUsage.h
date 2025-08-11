#pragma once
#include <windows.h>
#include <vector>
#include "PluginInterface.h"

class CCpuUsageItem : public IPluginItem
{
public:
    CCpuUsageItem(int core_index);
    ~CCpuUsageItem();

    // IPluginItem
    virtual const wchar_t* GetItemName() const;
    virtual const wchar_t* GetItemId() const;
    virtual const wchar_t* GetItemLableText() const;
    virtual const wchar_t* GetItemValueText() const;
    virtual const wchar_t* GetItemValueSampleText() const;
    virtual bool IsCustomDraw() const;
    virtual int GetItemWidth() const;
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode);

    void SetUsage(double usage);

private:
    int m_core_index;
    double m_usage;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
};

class CCPUUsagePlugin : public ITMPlugin
{
public:
    static CCPUUsagePlugin& Instance();

    // ITMPlugin
    virtual IPluginItem* GetItem(int index);
    virtual void DataRequired();
    virtual const wchar_t* GetInfo(PluginInfoIndex index);

private:
    CCPUUsagePlugin();
    ~CCPUUsagePlugin();

    CCPUUsagePlugin(const CCPUUsagePlugin&) = delete;
    CCPUUsagePlugin& operator=(const CCPUUsagePlugin&) = delete;

    void GetCpuUsage();

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;
    ULARGE_INTEGER m_last_idle_time;
    ULARGE_INTEGER m_last_kernel_time;
    ULARGE_INTEGER m_last_user_time;
    std::vector<ULARGE_INTEGER> m_last_core_times;
};