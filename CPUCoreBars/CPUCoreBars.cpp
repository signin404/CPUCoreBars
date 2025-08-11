// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// =================================================================
// CCpuUsageItem implementation
// =================================================================

// 构造函数更新，接收 is_e_core
CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core) 
    : m_core_index(core_index), m_is_e_core(is_e_core)
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
int CCpuUsageItem::GetItemWidth() const { return 10; } // 可以适当加宽以容纳图标

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = max(0.0, min(1.0, usage));
}

// 新增：手绘树叶图标的函数
void CCpuUsageItem::DrawLeafIcon(HDC hDC, const RECT& rect, bool dark_mode)
{
    // 选择一个不显眼的颜色
    COLORREF icon_color = dark_mode ? RGB(80, 80, 80) : RGB(220, 220, 220);
    HPEN hPen = CreatePen(PS_SOLID, 1, icon_color);
    HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);

    // 将图标绘制在条形图中央
    int center_x = rect.left + (rect.right - rect.left) / 2;
    int center_y = rect.top + (rect.bottom - rect.top) / 2;

    // 定义叶子形状 (一个简单的弧线加一根茎)
    MoveToEx(hDC, center_x, center_y + 4, NULL);
    LineTo(hDC, center_x, center_y - 4); // 茎
    MoveToEx(hDC, center_x, center_y - 4, NULL);
    // 使用 ArcTo 创建一个简单的弧形作为叶子
    ArcTo(hDC, center_x - 5, center_y - 5, center_x + 1, center_y + 1, 
        center_x, center_y - 4, center_x, center_y - 4);

    SelectObject(hDC, hOldPen);
    DeleteObject(hPen);
}

// DrawItem 函数大改，实现颜色选择和图标绘制
void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };

    // 1. 绘制背景
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    // 2. 如果是 E-Core，绘制树叶图标
    if (m_is_e_core)
    {
        DrawLeafIcon(dc, rect, dark_mode);
    }

    // 3. 根据核心索引选择条形图颜色
    COLORREF bar_color;
    if (m_core_index >= 12 && m_core_index <= 19)
    {
        bar_color = RGB(0xD9, 0x42, 0x35); // #D94235
    }
    else if (m_core_index % 2 == 1) // 奇数核心 1, 3, 5...
    {
        bar_color = RGB(0x26, 0xA0, 0xDA); // #26A0DA
    }
    else // 偶数核心 0, 2, 4...
    {
        bar_color = RGB(0x76, 0xCA, 0x53); // #76CA53
    }

    // 4. 绘制使用率条形图
    int bar_height = static_cast<int>(h * m_usage);
    if (bar_height > 0)
    {
        RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
        HBRUSH bar_brush = CreateSolidBrush(bar_color);
        FillRect(dc, &bar_rect, bar_brush);
        DeleteObject(bar_brush);
    }
}


// =================================================================
// CCPUCoreBarsPlugin implementation
// =================================================================

CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

// 新增：检测核心类型的函数实现
void CCPUCoreBarsPlugin::DetectCoreTypes()
{
    // 默认所有核心都不是 E-Core
    m_core_efficiency.assign(m_num_cores, 1); 

    DWORD length = 0;
    // 首先调用以获取所需缓冲区的大小
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return; // 不支持此功能或出错
    }

    std::vector<char> buffer(length);
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc_info = 
        (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data();

    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, proc_info, &length))
    {
        return; // 调用失败
    }

    char* ptr = buffer.data();
    while (ptr < buffer.data() + length)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current_info =
            (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

        if (current_info->Relationship == RelationProcessorCore)
        {
            BYTE efficiency = current_info->Processor.EfficiencyClass;
            // 遍历此核心关联的所有逻辑处理器
            for (int i = 0; i < current_info->Processor.GroupCount; ++i)
            {
                KAFFINITY mask = current_info->Processor.GroupMask[i].Mask;
                for (int j = 0; j < sizeof(KAFFINITY) * 8; ++j)
                {
                    if ((mask >> j) & 1)
                    {
                        // 计算逻辑处理器的全局索引
                        // 注意：此简化版假设只有一个处理器组
                        int logical_proc_index = j; 
                        if (logical_proc_index < m_num_cores)
                        {
                            m_core_efficiency[logical_proc_index] = efficiency;
                        }
                    }
                }
            }
        }
        ptr += current_info->Size;
    }
}

// 构造函数更新
CCPUCoreBarsPlugin::CCPUCoreBarsPlugin()
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_num_cores = sys_info.dwNumberOfProcessors;

    // 1. 首先检测核心类型
    DetectCoreTypes();

    // 2. 创建显示项，并传入核心类型
    for (int i = 0; i < m_num_cores; ++i)
    {
        // EfficiencyClass 为 0 通常表示 E-Core
        bool is_e_core = (m_core_efficiency[i] == 0);
        m_items.push_back(new CCpuUsageItem(i, is_e_core));
    }

    // 3. 初始化 PDH 查询
    if (PdhOpenQuery(nullptr, 0, &m_query) == ERROR_SUCCESS)
    {
        m_counters.resize(m_num_cores);
        for (int i = 0; i < m_num_cores; ++i)
        {
            wchar_t counter_path[128];
            swprintf_s(counter_path, L"\\Processor(%d)\\%% Processor Time", i);
            PdhAddCounterW(m_query, counter_path, 0, &m_counters[i]);
        }
        PdhCollectQueryData(m_query);
    }
}

// 析构函数、GetItem、DataRequired、GetInfo、UpdateCpuUsage 保持不变...
// (此处省略未修改的代码，与上一版相同)

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
    case TMI_DESCRIPTION: return L"Displays each CPU core usage as a vertical bar with P/E core detection.";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"1.2.0"; // 版本号+1
    default: return L"";
    }
}

void CCPUCoreBarsPlugin::UpdateCpuUsage()
{
    if (!m_query) return;
    if (PdhCollectQueryData(m_query) == ERROR_SUCCESS)
    {
        for (int i = 0; i < m_num_cores; ++i)
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