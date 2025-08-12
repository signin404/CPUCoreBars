// CPUCoreBars/CPUCoreBars.h
#pragma once
#include <windows.h>
#include <vector>
#include <Pdh.h>
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

    void WorkerThread(); // 后台工作线程现在也负责初始化

    std::vector<CCpuUsageItem*> m_items;
    int m_num_cores;

    // --- 线程管理与状态 ---
    std::thread m_worker_thread;
    std::atomic<bool> m_is_running;
    std::atomic<bool> m_is_initialized; // <--- 新增：标记初始化是否完成
    std::mutex m_data_mutex;
    std::vector<double> m_cpu_usage_buffer;
};