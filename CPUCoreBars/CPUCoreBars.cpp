// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")

// =================================================================
// CCpuUsageItem implementation
// =================================================================

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
int CCpuUsageItem::GetItemWidth() const { return 10; }

void CCpuUsageItem::SetUsage(double usage)
{
    m_usage = max(0.0, min(1.0, usage));
}

// --- NEW ---
// 美化后的树叶图标绘制函数
void CCpuUsageItem::DrawLeafIcon(HDC hDC, const RECT& rect, bool dark_mode)
{
    // 选择一个不显眼的颜色作为填充色
    COLORREF icon_color = dark_mode ? RGB(80, 80, 80) : RGB(220, 220, 220);
    HBRUSH hBrush = CreateSolidBrush(icon_color);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);

    // 为了避免描边，我们使用一个空画笔
    HPEN hPen = (HPEN)GetStockObject(NULL_PEN);
    HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);

    // 将图标绘制在条形图中央
    int center_x = rect.left + (rect.right - rect.left) / 2;
    int center_y = rect.top + (rect.bottom - rect.top) / 2;
    
    // 定义叶子形状的控制点，使其更像任务管理器的风格
    // 我们将使用两条三阶贝塞尔曲线来构成叶子的左右两半
    const int leaf_h = 4; // 叶子半高
    const int leaf_w = 4; // 叶子半宽

    POINT bottom_tip = { center_x, center_y + leaf_h };
    POINT top_tip    = { center_x, center_y - leaf_h };

    // 左半边叶子的控制点
    POINT left_curve[3] = {
        { center_x - leaf_w, center_y + leaf_h / 2 },
        { center_x - leaf_w, center_y - leaf_h / 2 },
        top_tip
    };

    // 右半边叶子的控制点
    POINT right_curve[3] = {
        { center_x + leaf_w, center_y - leaf_h / 2 },
        { center_x + leaf_w, center_y + leaf_h / 2 },
        bottom_tip
    };

    // 使用 GDI 路径来绘制并填充形状
    BeginPath(hDC);
    MoveToEx(hDC, bottom_tip.x, bottom_tip.y, NULL);
    PolyBezierTo(hDC, left_curve, 3);  // 绘制左半边
    PolyBezierTo(hDC, right_curve, 3); // 绘制右半边，闭合路径
    EndPath(hDC);

    FillPath(hDC); // 填充路径

    // 恢复并删除 GDI 对象
    SelectObject(hDC, hOldPen);
    SelectObject(hDC, hOldBrush);
    DeleteObject(hBrush);
}

// DrawItem 函数更新了颜色定义
void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };

    // 1. 绘制背景
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);

    // 2. 如果是 E-Core，绘制美化后的树叶图标
    if (m_is_e_core)
    {
        DrawLeafIcon(dc, rect, dark_mode);
    }

    // --- UPDATED ---
    // 3. 根据核心索引选择新的条形图颜色
    COLORREF bar_color;
    if (m_core_index >= 12 && m_core_index <= 19)
    {
        bar_color = RGB(217, 66, 53);
    }
    else if (m_core_index % 2 == 1) // 奇数核心 1, 3, 5...
    {
        bar_color = RGB(38, 160, 218);
    }
    else // 偶数核心 0, 2, 4...
    {
        bar_color = RGB(118, 202, 83);
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
// (这部分代码无需修改，保持原样即可)
// =================================================================

CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance()
{
    static CCPUCoreBarsPlugin instance;
    return instance;
}

void CCPUCoreBarsPlugin::DetectCoreTypes()
{
    m_core_efficiency.assign(m_num_cores, 1); 
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;
    std::vector<char> buffer(length);
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data();
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, proc_info, &length)) return;
    char* ptr = buffer.data();
    while (ptr < buffer.data() + length)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;
        if (current_info->Relationship == RelationProcessorCore)
        {
            BYTE efficiency = current_info->Processor.EfficiencyClass;
            for (int i = 0; i < current_info->Processor.GroupCount; ++i)
            {
                KAFFINITY mask = current_info->Processor.GroupMask[i].Mask;
                for (int j = 0; j < sizeof(KAFFINITY) * 8; ++j)
                {
                    if ((mask >> j) & 1)
                    {
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

CCPUCoreBarsPlugin::CCPUCoreBarsPlugin()
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_num_cores = sys_info.dwNumberOfProcessors;
    DetectCoreTypes();
    for (int i = 0; i < m_num_cores; ++i)
    {
        bool is_e_core = (m_core_efficiency[i] == 0);
        m_items.push_back(new CCpuUsageItem(i, is_e_core));
    }
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

CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin()
{
    if (m_query) PdhCloseQuery(m_query);
    for (auto item : m_items) delete item;
}

IPluginItem* CCPUCoreBarsPlugin::GetItem(int index)
{
    if (index >= 0 && static_cast<size_t>(index) < m_items.size()) return m_items[index];
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
    case TMI_VERSION: return L"1.3.0"; // 版本号+1
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