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

// 美化后的树叶图标绘制函数 - 现在只有一个版本，包含了所有美化效果
void CCpuUsageItem::DrawLeafIcon(HDC hDC, const RECT& rect, bool dark_mode)
{
    // 计算图标大小和位置
    int center_x = rect.left + (rect.right - rect.left) / 2;
    int center_y = rect.top + (rect.bottom - rect.top) / 2;
    int leaf_width = min(10, (rect.right - rect.left) / 2);
    int leaf_height = min(12, (rect.bottom - rect.top) / 2);
    
    // 使用更丰富的颜色
    COLORREF base_color = dark_mode ? RGB(85, 170, 85) : RGB(76, 175, 80);
    COLORREF dark_color = dark_mode ? RGB(56, 112, 56) : RGB(46, 125, 50);
    COLORREF light_color = dark_mode ? RGB(129, 199, 132) : RGB(165, 214, 167);
    COLORREF stem_color = dark_mode ? RGB(101, 67, 33) : RGB(139, 95, 51);
    COLORREF vein_color = dark_mode ? RGB(70, 140, 70) : RGB(56, 142, 60);
    
    // 创建画笔和画刷
    HBRUSH dark_brush = CreateSolidBrush(dark_color);
    HBRUSH base_brush = CreateSolidBrush(base_color);
    HBRUSH light_brush = CreateSolidBrush(light_color);
    HPEN stem_pen = CreatePen(PS_SOLID, 2, stem_color);
    HPEN leaf_pen = CreatePen(PS_SOLID, 1, base_color);
    HPEN vein_pen = CreatePen(PS_SOLID, 1, vein_color);
    
    HPEN old_pen = (HPEN)SelectObject(hDC, stem_pen);
    HBRUSH old_brush = (HBRUSH)SelectObject(hDC, base_brush);
    
    // 1. 绘制茎部
    SelectObject(hDC, stem_pen);
    MoveToEx(hDC, center_x, center_y + leaf_height/2 + 3, NULL);
    LineTo(hDC, center_x, center_y - leaf_height/2);
    
    // 2. 绘制叶子的阴影层 (稍微偏移创建深度效果)
    SelectObject(hDC, dark_brush);
    POINT shadow_points[6];
    shadow_points[0] = {center_x + 1, center_y - leaf_height/2 + 1};
    shadow_points[1] = {center_x + leaf_width/2 + 1, center_y - leaf_height/4 + 1};
    shadow_points[2] = {center_x + leaf_width/2 + 1, center_y + leaf_height/4 + 1};
    shadow_points[3] = {center_x + 1, center_y + leaf_height/2 + 1};
    shadow_points[4] = {center_x - leaf_width/2 + 1, center_y + leaf_height/4 + 1};
    shadow_points[5] = {center_x - leaf_width/2 + 1, center_y - leaf_height/4 + 1};
    Polygon(hDC, shadow_points, 6);
    
    // 3. 绘制主叶子层
    SelectObject(hDC, base_brush);
    POINT leaf_points[6];
    leaf_points[0] = {center_x, center_y - leaf_height/2};
    leaf_points[1] = {center_x + leaf_width/2, center_y - leaf_height/4};
    leaf_points[2] = {center_x + leaf_width/2, center_y + leaf_height/4};
    leaf_points[3] = {center_x, center_y + leaf_height/2};
    leaf_points[4] = {center_x - leaf_width/2, center_y + leaf_height/4};
    leaf_points[5] = {center_x - leaf_width/2, center_y - leaf_height/4};
    Polygon(hDC, leaf_points, 6);
    
    // 4. 绘制高光层
    SelectObject(hDC, light_brush);
    POINT highlight_points[4];
    highlight_points[0] = {center_x, center_y - leaf_height/2};
    highlight_points[1] = {center_x + leaf_width/3, center_y - leaf_height/6};
    highlight_points[2] = {center_x, center_y + leaf_height/6};
    highlight_points[3] = {center_x - leaf_width/3, center_y - leaf_height/6};
    Polygon(hDC, highlight_points, 4);
    
    // 5. 绘制详细叶脉
    SelectObject(hDC, vein_pen);
    // 主脉
    MoveToEx(hDC, center_x, center_y - leaf_height/2, NULL);
    LineTo(hDC, center_x, center_y + leaf_height/2);
    
    // 侧脉 - 上半部分
    MoveToEx(hDC, center_x, center_y - leaf_height/4, NULL);
    LineTo(hDC, center_x - leaf_width/3, center_y);
    MoveToEx(hDC, center_x, center_y - leaf_height/4, NULL);
    LineTo(hDC, center_x + leaf_width/3, center_y);
    
    // 侧脉 - 下半部分
    MoveToEx(hDC, center_x, center_y + leaf_height/6, NULL);
    LineTo(hDC, center_x - leaf_width/4, center_y + leaf_height/3);
    MoveToEx(hDC, center_x, center_y + leaf_height/6, NULL);
    LineTo(hDC, center_x + leaf_width/4, center_y + leaf_height/3);
    
    // 6. 添加额外的高光效果 (浅色模式)
    if (!dark_mode) {
        COLORREF highlight_color = RGB(200, 255, 200); // 非常浅的绿色高光
        HPEN highlight_pen = CreatePen(PS_SOLID, 1, highlight_color);
        SelectObject(hDC, highlight_pen);
        
        // 在叶子边缘添加小高光线
        MoveToEx(hDC, center_x - 1, center_y - leaf_height/3, NULL);
        LineTo(hDC, center_x - 2, center_y - leaf_height/3 + 2);
        MoveToEx(hDC, center_x + 1, center_y - leaf_height/4, NULL);
        LineTo(hDC, center_x + 2, center_y - leaf_height/4 + 1);
        
        DeleteObject(highlight_pen);
    }
    
    // 恢复对象并清理
    SelectObject(hDC, old_pen);
    SelectObject(hDC, old_brush);
    DeleteObject(dark_brush);
    DeleteObject(base_brush);
    DeleteObject(light_brush);
    DeleteObject(stem_pen);
    DeleteObject(leaf_pen);
    DeleteObject(vein_pen);
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

    // 2. 如果是 E-Core，绘制美化的树叶图标
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

// 检测核心类型的函数实现
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

// 构造函数
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
    case TMI_DESCRIPTION: return L"Displays each CPU core usage as a vertical bar with P/E core detection and beautiful leaf icons.";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: return L"1.3.0"; // 版本号更新
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