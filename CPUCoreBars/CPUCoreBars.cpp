// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include <string>
#include <PdhMsg.h>
#include <winevt.h> // <--- Include for Windows Event Log

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wevtapi.lib") // <--- Link the Event Log API library

// =================================================================
// CCpuUsageItem implementation (no changes)
// =================================================================
// ... (The entire implementation of CCpuUsageItem is here and unchanged)
CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core) : m_core_index(core_index), m_is_e_core(is_e_core) { swprintf_s(m_item_name, L"CPU Core %d", m_core_index); swprintf_s(m_item_id, L"cpu_core_%d", m_core_index); }
const wchar_t* CCpuUsageItem::GetItemName() const { return m_item_name; }
const wchar_t* CCpuUsageItem::GetItemId() const { return m_item_id; }
const wchar_t* CCpuUsageItem::GetItemLableText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueSampleText() const { return L""; }
bool CCpuUsageItem::IsCustomDraw() const { return true; }
int CCpuUsageItem::GetItemWidth() const { return 10; }
void CCpuUsageItem::SetUsage(double usage) { m_usage = max(0.0, min(1.0, usage)); }
void CCpuUsageItem::DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode) { COLORREF icon_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0); SetTextColor(hDC, icon_color); SetBkMode(hDC, TRANSPARENT); const wchar_t* symbol = L"\u26B2"; HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol"); HGDIOBJ hOldFont = SelectObject(hDC, hFont); DrawTextW(hDC, symbol, -1, (LPRECT)&rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE); SelectObject(hDC, hOldFont); DeleteObject(hFont); }
void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) { HDC dc = (HDC)hDC; RECT rect = { x, y, x + w, y + h }; HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255)); FillRect(dc, &rect, bg_brush); DeleteObject(bg_brush); COLORREF bar_color; if (m_core_index >= 12 && m_core_index <= 19) { bar_color = RGB(217, 66, 53); } else if (m_core_index % 2 == 1) { bar_color = RGB(38, 160, 218); } else { bar_color = RGB(118, 202, 83); } if (m_usage >= 0.8) { bar_color = RGB(217, 66, 53); } else if (m_usage >= 0.5) { bar_color = RGB(246, 182, 78); } int bar_height = static_cast<int>(h * m_usage); if (bar_height > 0) { RECT bar_rect = { x, y + (h - bar_height), x + w, y + h }; HBRUSH bar_brush = CreateSolidBrush(bar_color); FillRect(dc, &bar_rect, bar_brush); DeleteObject(bar_brush); } if (m_is_e_core) { DrawECoreSymbol(dc, rect, dark_mode); } }


// =================================================================
// UPDATED: CNvidiaLimitReasonItem implementation (with WHEA logic)
// =================================================================
CNvidiaLimitReasonItem::CNvidiaLimitReasonItem()
{
    wcscpy_s(m_value_text, L"N/A");

    HDC hdc = GetDC(NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    // Measure the longest value text
    const wchar_t* sample_value = GetItemValueSampleText();
    SIZE value_size;
    GetTextExtentPoint32W(hdc, sample_value, wcslen(sample_value), &value_size);

    // Measure the icon part (use "99+" as the widest case)
    const wchar_t* sample_icon = L"99+";
    SIZE icon_size;
    GetTextExtentPoint32W(hdc, sample_icon, wcslen(sample_icon), &icon_size);
    
    // Width = icon width + padding + value width
    m_width = icon_size.cx + 4 + value_size.cx;

    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);
}

const wchar_t* CNvidiaLimitReasonItem::GetItemName() const { return L"GPU/WHEA 状态"; }
const wchar_t* CNvidiaLimitReasonItem::GetItemId() const { return L"gpu_whea_status"; }
const wchar_t* CNvidiaLimitReasonItem::GetItemLableText() const { return L"☁"; }
const wchar_t* CNvidiaLimitReasonItem::GetItemValueText() const { return m_value_text; }
const wchar_t* CNvidiaLimitReasonItem::GetItemValueSampleText() const { return L"硬过热"; }
bool CNvidiaLimitReasonItem::IsCustomDraw() const { return true; }
int CNvidiaLimitReasonItem::GetItemWidth() const { return m_width; }

void CNvidiaLimitReasonItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = (HDC)hDC;
    
    // Determine icon width (use "99+" as reference for spacing)
    SIZE icon_size;
    GetTextExtentPoint32W(dc, L"99+", 3, &icon_size);
    int icon_width = icon_size.cx;

    RECT icon_rect = { x, y, x + icon_width, y + h };
    RECT text_rect = { x + icon_width + 4, y, x + w, y + h };

    COLORREF default_text_color = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    SetBkMode(dc, TRANSPARENT);

    // --- 1. Draw Icon or WHEA Count ---
    if (m_whea_count > 0)
    {
        wchar_t whea_text[10];
        if (m_whea_count > 99) {
            wcscpy_s(whea_text, L"99+");
        } else {
            swprintf_s(whea_text, L"%d", m_whea_count);
        }
        
        // Draw WHEA count in RED
        SetTextColor(dc, RGB(255, 0, 0));
        DrawTextW(dc, whea_text, -1, &icon_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    else
    {
        // Draw normal kaomoji icon
        HFONT hIconFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
        HGDIOBJ hOldFont = SelectObject(dc, hIconFont);
        SetTextColor(dc, default_text_color);
        DrawTextW(dc, GetItemLableText(), -1, &icon_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, hOldFont);
        DeleteObject(hIconFont);
    }

    // --- 2. Draw the Value Text ---
    SetTextColor(dc, default_text_color);
    DrawTextW(dc, GetItemValueText(), -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void CNvidiaLimitReasonItem::SetValue(const wchar_t* value) { wcscpy_s(m_value_text, value); }
void CNvidiaLimitReasonItem::SetWheaCount(int count) { m_whea_count = count; }


// =================================================================
// UPDATED: CCPUCoreBarsPlugin implementation
// =================================================================
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance() { static CCPUCoreBarsPlugin instance; return instance; }
CCPUCoreBarsPlugin::CCPUCoreBarsPlugin() { /* ... unchanged ... */ InitNVML(); }
CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin() { /* ... unchanged ... */ }
IPluginItem* CCPUCoreBarsPlugin::GetItem(int index) { /* ... unchanged ... */ }

// UPDATED: DataRequired now calls WHEA update
void CCPUCoreBarsPlugin::DataRequired()
{
    UpdateCpuUsage();
    UpdateGpuLimitReason();
    UpdateWheaErrorCount();

    // Pass WHEA count to the display item
    if (m_gpu_item) {
        m_gpu_item->SetWheaCount(m_whea_error_count);
    }
}

const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index) { /* ... version bump to 2.5.0 ... */ }
void CCPUCoreBarsPlugin::InitNVML() { /* ... unchanged ... */ }
void CCPUCoreBarsPlugin::ShutdownNVML() { /* ... unchanged ... */ }
void CCPUCoreBarsPlugin::UpdateGpuLimitReason() { /* ... unchanged ... */ }
void CCPUCoreBarsPlugin::UpdateCpuUsage() { /* ... unchanged ... */ }
void CCPUCoreBarsPlugin::DetectCoreTypes() { /* ... unchanged ... */ }

// NEW: WHEA Error Count Logic
void CCPUCoreBarsPlugin::UpdateWheaErrorCount()
{
    // Query for WHEA-Logger events in the System log from the last 24 hours
    LPCWSTR query = L"*[System[Provider[@Name='WHEA-Logger'] and TimeCreated[timediff(@SystemTime) <= 86400000]]]";
    EVT_HANDLE hResults = EvtQuery(NULL, L"System", query, EvtQueryChannelPath | EvtQueryReverseDirection);
    
    if (hResults == NULL) {
        m_whea_error_count = 0; // Or some error indicator if you prefer
        return;
    }

    DWORD dwEventCount = 0;
    EVT_HANDLE hEvents[128];
    DWORD dwReturned = 0;

    while (EvtNext(hResults, ARRAYSIZE(hEvents), hEvents, INFINITE, 0, &dwReturned))
    {
        dwEventCount += dwReturned;
        for (DWORD i = 0; i < dwReturned; i++) {
            EvtClose(hEvents[i]);
        }
    }

    m_whea_error_count = dwEventCount;
    EvtClose(hResults);
}

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance() { return &CCPUCoreBarsPlugin::Instance(); }

// NOTE: The unchanged function bodies are omitted here for brevity, 
// but you should use the full code from the previous step for them.