// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include "PluginInterface.h"

class CCpuUsageItem : public IPluginItem
{
public:
    CCpuUsageItem(int core_index, bool is_e_core);
    virtual ~CCpuUsageItem() = default;

    // ... (GetItemName等函数保持不变)
    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

    void SetUsage(double usage);
    void SetIsECore(bool is_e_core); // <--- 新增: 用于后台更新核心类型

private:
    void DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode);
    int m_core_index;
    double m_usage = 0.0;
    wchar_t m_item_name[32];
    wchar_t m_item_id[32];
    bool m_is_e_core;
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

    void WorkerThread();

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;

    std::thread m_worker_thread;
    std::atomic<bool> m_is_running;
    std::atomic<bool> m_is_initialized;
    std::mutex m_data_mutex;
    std::vector<double> m_cpu_usage_buffer;
};