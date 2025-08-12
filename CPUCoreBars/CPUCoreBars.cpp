// CPUCoreBars/CPUCoreBars.cpp
#include "CPUCoreBars.h"
#include "resource.h"
#include <string>
#include <fstream>
#include <vector>
#include <CommDlg.h>
#include <Pdh.h>
#include <PdhMsg.h>

#pragma comment(lib, "pdh.lib")
#pragma execution_character_set("utf-8") // FIX: 强制执行字符集为UTF-8

// --- CCpuUsageItem implementation (No changes) ---
CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core)
    : m_core_index(core_index), m_is_e_core(is_e_core), m_color(RGB(0, 128, 0)) {}
const wchar_t* CCpuUsageItem::GetItemName() const { return m_item_name; }
const wchar_t* CCpuUsageItem::GetItemId() const { return m_item_id; }
const wchar_t* CCpuUsageItem::GetItemLableText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueSampleText() const { return L""; }
bool CCpuUsageItem::IsCustomDraw() const { return true; }
int CCpuUsageItem::GetItemWidth() const { return 10; }
void CCpuUsageItem::SetUsage(double usage) { m_usage = max(0.0, min(1.0, usage)); }
void CCpuUsageItem::SetColor(COLORREF color) { m_color = color; }
void CCpuUsageItem::DrawECoreSymbol(HDC hDC, const RECT& rect, bool dark_mode) {
    COLORREF icon_color = dark_mode ? RGB(80, 80, 80) : RGB(220, 220, 220);
    SetTextColor(hDC, icon_color);
    SetBkMode(hDC, TRANSPARENT);
    const wchar_t* symbol = L"\u26B2";
    HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
    HGDIOBJ hOldFont = SelectObject(hDC, hFont);
    DrawTextW(hDC, symbol, -1, (LPRECT)&rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hDC, hOldFont);
    DeleteObject(hFont);
}
void CCpuUsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) {
    HDC dc = (HDC)hDC;
    RECT rect = { x, y, x + w, y + h };
    HBRUSH bg_brush = CreateSolidBrush(dark_mode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    FillRect(dc, &rect, bg_brush);
    DeleteObject(bg_brush);
    if (m_is_e_core) {
        DrawECoreSymbol(dc, rect, dark_mode);
    }
    COLORREF bar_color = m_color;
    int bar_height = static_cast<int>(h * m_usage);
    if (bar_height > 0) {
        RECT bar_rect = { x, y + (h - bar_height), x + w, y + h };
        HBRUSH bar_brush = CreateSolidBrush(bar_color);
        FillRect(dc, &bar_rect, bar_brush);
        DeleteObject(bar_brush);
    }
}

// --- SettingsDialogProc (No changes) ---
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static CCPUCoreBarsPlugin* p_plugin = nullptr;
    static std::vector<COLORREF> temp_colors;
    switch (message) {
    case WM_INITDIALOG: {
        p_plugin = (CCPUCoreBarsPlugin*)lParam;
        temp_colors = p_plugin->m_core_colors;
        HWND h_list = GetDlgItem(hDlg, IDC_CORE_LIST);
        for (size_t i = 0; i < temp_colors.size(); ++i) {
            wchar_t buffer[32];
            swprintf_s(buffer, L"Core %zu", i);
            SendMessage(h_list, LB_ADDSTRING, 0, (LPARAM)buffer);
        }
        SendMessage(h_list, LB_SETCURSEL, 0, 0);
        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_CORE_LIST, LBN_SELCHANGE), (LPARAM)h_list);
        return (INT_PTR)TRUE;
    }
    case WM_DRAWITEM: {
        if (wParam == IDC_COLOR_PREVIEW) {
            LPDRAWITEMSTRUCT p_dis = (LPDRAWITEMSTRUCT)lParam;
            LRESULT selected_index = SendMessage(GetDlgItem(hDlg, IDC_CORE_LIST), LB_GETCURSEL, 0, 0);
            if (selected_index != LB_ERR) {
                HBRUSH brush = CreateSolidBrush(temp_colors[selected_index]);
                FillRect(p_dis->hDC, &p_dis->rcItem, brush);
                DeleteObject(brush);
            }
            return (INT_PTR)TRUE;
        }
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDC_CORE_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                InvalidateRect(GetDlgItem(hDlg, IDC_COLOR_PREVIEW), NULL, TRUE);
            }
            break;
        case IDC_CHANGE_COLOR_BUTTON: {
            LRESULT selected_index = SendMessage(GetDlgItem(hDlg, IDC_CORE_LIST), LB_GETCURSEL, 0, 0);
            if (selected_index != LB_ERR) {
                CHOOSECOLOR cc;
                static COLORREF acr_cust_clr[16];
                ZeroMemory(&cc, sizeof(cc));
                cc.lStructSize = sizeof(cc);
                cc.hwndOwner = hDlg;
                cc.lpCustColors = (LPDWORD)acr_cust_clr;
                cc.rgbResult = temp_colors[selected_index];
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                if (ChooseColor(&cc) == TRUE) {
                    temp_colors[selected_index] = cc.rgbResult;
                    InvalidateRect(GetDlgItem(hDlg, IDC_COLOR_PREVIEW), NULL, TRUE);
                }
            }
            break;
        }
        case IDOK:
            p_plugin->m_core_colors = temp_colors;
            p_plugin->SaveSettings();
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    }
    return (INT_PTR)FALSE;
}

// --- CCPUCoreBarsPlugin implementation (No changes) ---
CCPUCoreBarsPlugin& CCPUCoreBarsPlugin::Instance() { static CCPUCoreBarsPlugin instance; return instance; }
std::wstring CCPUCoreBarsPlugin::GetSettingsPath() const {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    wchar_t* last_slash = wcsrchr(path, L'\\');
    if (last_slash) {
        *(last_slash + 1) = L'\0';
        wcscat_s(path, L"plugins\\CPUCoreBars.ini");
        return path;
    }
    return L"";
}
void CCPUCoreBarsPlugin::LoadSettings() {
    m_core_colors.assign(m_num_cores, RGB(0, 128, 0));
    std::wifstream file(GetSettingsPath());
    if (file.is_open()) {
        for (int i = 0; i < m_num_cores; ++i) {
            COLORREF color;
            if (file >> color) { m_core_colors[i] = color; }
            else { break; }
        }
        file.close();
    }
}
void CCPUCoreBarsPlugin::SaveSettings() {
    std::wofstream file(GetSettingsPath());
    if (file.is_open()) {
        for (const auto& color : m_core_colors) {
            file << color << std::endl;
        }
        file.close();
    }
    ApplySettingsToItems();
}
void CCPUCoreBarsPlugin::ApplySettingsToItems() {
    for (size_t i = 0; i < m_items.size() && i < m_core_colors.size(); ++i) {
        m_items[i]->SetColor(m_core_colors[i]);
    }
}
void CCPUCoreBarsPlugin::DetectCoreTypes() {
    m_core_efficiency.assign(m_num_cores, 1);
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;
    std::vector<char> buffer(length);
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer.data();
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, proc_info, &length)) return;
    char* ptr = buffer.data();
    while (ptr < buffer.data() + length) {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current_info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;
        if (current_info->Relationship == RelationProcessorCore) {
            BYTE efficiency = current_info->Processor.EfficiencyClass;
            for (int i = 0; i < current_info->Processor.GroupCount; ++i) {
                KAFFINITY mask = current_info->Processor.GroupMask[i].Mask;
                for (int j = 0; j < sizeof(KAFFINITY) * 8; ++j) {
                    if ((mask >> j) & 1) {
                        int logical_proc_index = j;
                        if (logical_proc_index < m_num_cores) {
                            m_core_efficiency[logical_proc_index] = efficiency;
                        }
                    }
                }
            }
        }
        ptr += current_info->Size;
    }
}
CCPUCoreBarsPlugin::CCPUCoreBarsPlugin() {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_num_cores = sys_info.dwNumberOfProcessors;
    LoadSettings();
    DetectCoreTypes();
    for (int i = 0; i < m_num_cores; ++i) {
        bool is_e_core = (m_core_efficiency[i] == 0);
        m_items.push_back(new CCpuUsageItem(i, is_e_core));
    }
    ApplySettingsToItems();
    if (PdhOpenQuery(nullptr, 0, &m_query) == ERROR_SUCCESS) {
        m_counters.resize(m_num_cores);
        for (int i = 0; i < m_num_cores; ++i) {
            wchar_t counter_path[128];
            swprintf_s(counter_path, L"\\Processor(%d)\\%% Processor Time", i);
            PdhAddCounterW(m_query, counter_path, 0, &m_counters[i]);
        }
        PdhCollectQueryData(m_query);
    }
}
CCPUCoreBarsPlugin::~CCPUCoreBarsPlugin() {
    if (m_query) PdhCloseQuery(m_query);
    for (auto item : m_items) delete item;
}
IPluginItem* CCPUCoreBarsPlugin::GetItem(int index) {
    if (index >= 0 && static_cast<size_t>(index) < m_items.size()) return m_items[index];
    return nullptr;
}
void CCPUCoreBarsPlugin::DataRequired() { UpdateCpuUsage(); }
const wchar_t* CCPUCoreBarsPlugin::GetInfo(PluginInfoIndex index) {
    switch (index) {
    case TMI_NAME: return L"CPU核心使用率条形图";
    case TMI_DESCRIPTION: return L"将每个CPU核心的使用率显示为独立的竖向条形图，并可自定义颜色。";
    case TMI_AUTHOR: return L"Your Name";
    case TMI_COPYRIGHT: return L"Copyright (C) 2025";
    case TMI_URL: return L"";
    case TMI_VERSION: L"2.3.0"; // Final working version
    default: return L"";
    }
}
void CCPUCoreBarsPlugin::UpdateCpuUsage() {
    if (!m_query) return;
    if (PdhCollectQueryData(m_query) == ERROR_SUCCESS) {
        for (int i = 0; i < m_num_cores; ++i) {
            PDH_FMT_COUNTERVALUE value;
            if (PdhGetFormattedCounterValue(m_counters[i], PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) {
                m_items[i]->SetUsage(value.doubleValue / 100.0);
            }
            else {
                m_items[i]->SetUsage(0.0);
            }
        }
    }
}
void CCPUCoreBarsPlugin::ShowSettingWindow(void* hParent) {
    HMODULE h_dll = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)&CCPUCoreBarsPlugin::Instance, &h_dll);
    DialogBoxParamW(h_dll, MAKEINTRESOURCEW(IDD_SETTINGS), (HWND)hParent, SettingsDialogProc, (LPARAM)this);
}

// =================================================================
// EXPORTED FUNCTIONS
// =================================================================

extern "C" __declspec(dllexport) ITMPlugin* __stdcall TMPluginGetInstance()
{
    return &CCPUCoreBarsPlugin::Instance();
}

extern "C" __declspec(dllexport) void __stdcall TMPluginShowSettingWindow(void* hParent)
{
    CCPUCoreBarsPlugin::Instance().ShowSettingWindow(hParent);
}