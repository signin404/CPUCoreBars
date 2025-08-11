// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>
#include <vector>
#include <map>          // <--- 引入 map 用于排序
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// CCpuUsageItem implementation (无需修改)
CCpuUsageItem::CCpuUsageItem(int core_index) : m_core_index(core_index)
{
    swprintf_s(m_item_name, L"CPU Core %d", m_core_index);
    swprintf_s(m_item_id, L"cpu_core_%d", m_core_index);
}
const wchar_t* CCpuUsageItem::GetItemName() const { return m_item_name; }
const wchar_t* CCpuUsageItem::GetItemId() const { return m_item_id; }
const wchar_t* CCpuUsageItem::GetItemLableText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueSampleText() const { return L""; }
bool CCpuUsageItem::IsCustomDraw() const { return true; }
int CCpuUsageItem::GetItemWidth() const { return 8; }
void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);
    int bar_height = static_cast<int>(h * m_usage);
    RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
    HBRUSH bar_brush = CreateSolidBrush(dark_mode ? RGB(0, 200, 0) : RGB(0, 128, 0));
    FillRect(dc, &bar_rect, bar_brush);
    DeleteObject(bar_brush);
}
void CCpuUsageItem::SetUsage(double usage) { m_usage = max(0.0, min(1.0, usage)); }


// CCPUCoreBarsPlugin implementation (大幅修改构造函数)
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin() : m_num_cores(0)
{
    // --- 1. 初始化 PDH 查询 ---
    if (PdhOpenQuery(nullptr, 0, &m_query) != ERROR_SUCCESS)
    {
        return; // 初始化失败，直接返回
    }

    // --- 2. 枚举 Processor 对象的所有实例来获取真实的核心编号 ---
    DWORD dwCounterListSize = 0;
    DWORD dwInstanceListSize = 0;
    // 首次调用获取所需缓冲区大小
    PdhEnumObjectItemsW(nullptr, nullptr, L"Processor", nullptr, &dwCounterListSize, nullptr, &dwInstanceListSize, PERF_DETAIL_WIZARD, 0);

    std::vector<wchar_t> instanceList(dwInstanceListSize);
    // 再次调用获取实例列表
    if (PdhEnumObjectItemsW(nullptr, nullptr, L"Processor", nullptr, &dwCounterListSize, instanceList.data(), &dwInstanceListSize, PERF_DETAIL_WIZARD, 0) == ERROR_SUCCESS)
    {
        // 使用 map 来自动排序核心，确保 UI 显示顺序正确
        std::map<int, std::wstring> core_map;

        // 遍历多字符串缓冲区
        for (const wchar_t* pInstance = instanceList.data(); *pInstance; pInstance += wcslen(pInstance) + 1)
        {
            // 过滤掉代表总使用率的 "_Total" 实例
            if (wcscmp(pInstance, L"_Total") == 0)
            {
                continue;
            }

            // 将实例名（核心编号字符串）转换为整数
            int core_index = _wtoi(pInstance);
            core_map[core_index] = pInstance;
        }

        // --- 3. 根据排序后的核心列表创建计数器和插件项 ---
        m_num_cores = static_cast<int>(core_map.size());
        m_items.reserve(m_num_cores);
        m_counters.reserve(m_num_cores);

        for (const auto& pair : core_map)
        {
            int core_index = pair.first;
            const std::wstring& instance_name = pair.second;

            // 创建插件项
            m_items.push_back(new CCpuUsageItem(core_index));

            // 构建正确的计数器路径，例如 "\Processor(5)\% Processor Time"
            wchar_t counter_path[128];
            swprintf_s(counter_path, L"\\Processor(%s)\\%% Processor Time", instance_name.c_str());
            
            PDH_HCOUNTER counter;
            if (PdhAddCounterW(m_query, counter_path, 0, &counter) == ERROR_SUCCESS)
            {
                m_counters.push_back(counter);
            }
        }
    }

    // --- 4. 首次收集数据以初始化计数器 ---
    if (!m_counters.empty())
    {
        PdhCollectQueryData(m_query);
    }
}

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    if (m_query)
    {
        PdhCloseQuery(m_query);
    }
    for (auto item : m_items)
    {
        delete item;
    }
}

IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < m_items.size())
    {
        return m_items[index];
    }
    return nullptr;
}

void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
}

const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME: return L"CPU Core Usage Bars";
    case TMI_DESCRIPTION: return L"Displays each CPU core usage as a vertical bar.";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"1.2.0"; // 修正了核心匹配问题，更新版本号
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    if (!m_query || m_counters.empty()) return;

    if (PdhCollectQueryData(m_query) == ERROR_SUCCESS)
    {
        for (size_t i = 0; i < m_counters.size(); ++i)
        {
            PDH_FMT_COUNTERVALUE value;
            if (PdhGetFormattedCounterValue(m_counters[i], PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS)
            {
                m_items[i]->SetUsage(value.doubleValue / 100.0);
            }
            else
            {
                m_items[i]->SetUsage(0.0);
            }
        }
    }
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}