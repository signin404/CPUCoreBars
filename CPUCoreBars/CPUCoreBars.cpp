// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>
#include <vector>
#include <map>
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


// CCPUCoreBarsPlugin implementation (重写为健壮版本)
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin() : m_num_cores(0)
{
    if (PdhOpenQuery(nullptr, 0, &m_query) != ERROR_SUCCESS)
    {
        // 如果连查询都无法打开，就彻底放弃
        return;
    }

    bool enumeration_successful = false;
    
    // --- 尝试使用高级、准确的枚举方法 ---
    DWORD dwInstanceListSize = 0;
    PDH_STATUS status = PdhEnumObjectItemsW(nullptr, nullptr, L"Processor", nullptr, &dwInstanceListSize, nullptr, &dwInstanceListSize, PERF_DETAIL_WIZARD, 0);
    
    if (status == PDH_MORE_DATA)
    {
        std::vector<wchar_t> instanceList(dwInstanceListSize);
        // 使用循环来防止在两次调用之间缓冲区大小变化的罕见情况
        while ((status = PdhEnumObjectItemsW(nullptr, nullptr, L"Processor", nullptr, &dwInstanceListSize, instanceList.data(), &dwInstanceListSize, PERF_DETAIL_WIZARD, 0)) == PDH_MORE_DATA)
        {
            instanceList.resize(dwInstanceListSize);
        }

        if (status == ERROR_SUCCESS)
        {
            std::map<int, std::wstring> core_map;
            for (const wchar_t* pInstance = instanceList.data(); *pInstance; pInstance += wcslen(pInstance) + 1)
            {
                if (wcscmp(pInstance, L"_Total") == 0) continue;
                int core_index = _wtoi(pInstance);
                core_map[core_index] = pInstance;
            }

            if (!core_map.empty())
            {
                m_num_cores = static_cast<int>(core_map.size());
                m_items.reserve(m_num_cores);
                m_counters.reserve(m_num_cores);

                for (const auto& pair : core_map)
                {
                    m_items.push_back(new CCpuUsageItem(pair.first));
                    wchar_t counter_path[128];
                    swprintf_s(counter_path, L"\\Processor(%s)\\%% Processor Time", pair.second.c_str());
                    PDH_HCOUNTER counter;
                    if (PdhAddCounterW(m_query, counter_path, 0, &counter) == ERROR_SUCCESS)
                    {
                        m_counters.push_back(counter);
                    }
                }
                enumeration_successful = true;
            }
        }
    }

    // --- 后备方案 (Fallback) ---
    // 如果高级枚举失败，则使用简单但保证能工作的方法
    if (!enumeration_successful)
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        m_num_cores = sys_info.dwNumberOfProcessors;

        m_items.clear(); // 清理可能存在的失败尝试
        m_counters.clear();
        
        m_items.reserve(m_num_cores);
        m_counters.reserve(m_num_cores);

        for (int i = 0; i < m_num_cores; ++i)
        {
            m_items.push_back(new CCpuUsageItem(i));
            wchar_t counter_path[128];
            swprintf_s(counter_path, L"\\Processor(%d)\\%% Processor Time", i);
            PDH_HCOUNTER counter;
            if (PdhAddCounterW(m_query, counter_path, 0, &counter) == ERROR_SUCCESS)
            {
                m_counters.push_back(counter);
            }
        }
    }

    // 首次收集数据以初始化计数器
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
    case TMI_VERSION: return L"1.3.0"; // 健壮性更新
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    if (!m_query || m_counters.empty() || m_counters.size() != m_items.size()) return;

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