// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include "PluginInterface.h"

class CCpuUsageItem; // 前向声明

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

    std::thread m_worker_thread;
    std::atomic<bool> m_is_running;
    std::atomic<bool> m_is_initialized;
    std::mutex m_data_mutex; // 这个互斥锁现在也保护 m_items 的创建
    std::vector<double> m_cpu_usage_buffer;
};