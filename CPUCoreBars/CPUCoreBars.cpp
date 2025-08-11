// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h" // <--- 已修正笔误 .hh -> .h
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


// CCPUCoreBarsPlugin implementation (最终正确且健壮版)
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin() : m_num_cores(0)
{
    // 1. 打开查询，如果失败，则构造函数结束，插件不会加载
    if (PdhOpenQuery(nullptr, 0, &m_query) != ERROR_SUCCESS)
    {
        return;
    }

    // 2. 健壮地枚举所有 "Processor" 实例
    DWORD dwInstanceListSize = 0;
    PDH_STATUS status = PdhEnumObjectItemsW(nullptr, nullptr, L"Processor", nullptr, &dwInstanceListSize, nullptr, &dwInstanceListSize, PERF_DETAIL_WIZARD, 0);
    
    // 必须从 PDH_MORE_DATA 开始
    if (status != PDH_MORE_DATA)
    {
        PdhCloseQuery(m_query); // 清理资源
        m_query = nullptr;
        return; // 如果无法获取缓冲区大小，则放弃
    }

    std::vector<wchar_t> instanceList(dwInstanceListSize);
    // 使用循环来防止在两次调用之间缓冲区大小变化的罕见情况
    while ((status = PdhEnumObjectItemsW(nullptr, nullptr, L"Processor", nullptr, &dwInstanceListSize, instanceList.data(), &dwInstanceListSize, PERF_DETAIL_WIZARD, 0)) == PDH_MORE_DATA)
    {
        instanceList.resize(dwInstanceListSize);
    }

    // 只有在最终成功时才继续
    if (status == ERROR_SUCCESS)
    {
        std::map<int, std::wstring> core_map;
        for (const wchar_t* pInstance = instanceList.data(); *pInstance; pInstance += wcslen(pInstance) + 1)
        {
            if (wcscmp(pInstance, L"_Total") == 0) continue;
            int core_index = _wtoi(pInstance);
            core_map[core_index] = pInstance;
        }

        // 确保我们真的找到了核心
        if (!core_map.empty())
        {
            m_num_cores = static_cast<int>(core_map.size());
            m_items.reserve(m_num_cores);
            m_counters.reserve(m_num_cores);

            // 遍历排序后的 map，确保顺序和编号完全正确
            for (const auto& pair : core_map)
            {
                // 使用 map 的 key (已排序的真实核心编号) 创建显示项
                m_items.push_back(new CCpuUsageItem(pair.first));
                
                // 使用 map 的 value (对应的实例名) 创建计数器
                wchar_t counter_path[128];
                swprintf_s(counter_path, L"\\Processor(%s)\\%% Processor Time", pair.second.c_str());
                PDH_HCOUNTER counter;
                if (PdhAddCounterW(m_query, counter_path, 0, &counter) == ERROR_SUCCESS)
                {
                    m_counters.push_back(counter);
                }
            }
        }
    }
    
    // 如果 m_items 或 m_counters 在此之后仍然为空 (因为枚举失败或没有找到核心),
    // 插件将不会加载，这是正确的行为。

    // 3. 首次收集数据以初始化
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
    case TMI_VERSION: return L"2.0.1"; // 修正编译错误
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    // 增加一个额外的安全检查
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