// CPUCoreBars/CPUCoreBars.cpp - 完全优化版本
#include "CPUCoreBars.h"
#include <string>
#include <algorithm>
#include <chrono>
#include <PdhMsg.h>
#include <winevt.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// =================================================================
// GraphicsPool implementation
// =================================================================
GraphicsPool& GraphicsPool::Instance() {
    static GraphicsPool instance;
    return instance;
}

Graphics* GraphicsPool::Acquire(HDC hdc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 查找可用的Graphics对象
    for (auto& wrapper : m_pool) {
        if (!wrapper.inUse && wrapper.hdc == hdc) {
            wrapper.inUse = true;
            wrapper.graphics->ResetTransform();
            return wrapper.graphics.get();
        }
    }
    
    // 创建新的Graphics对象
    m_pool.push_back({std::make_unique<Graphics>(hdc), true, hdc});
    m_pool.back().graphics->SetSmoothingMode(SmoothingModeAntiAlias);
    return m_pool.back().graphics.get();
}

void GraphicsPool::Release(Graphics* g) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& wrapper : m_pool) {
        if (wrapper.graphics.get() == g) {
            wrapper.inUse = false;
            break;
        }
    }
}

// =================================================================
// EventLogMonitor implementation
// =================================================================
EventLogMonitor::EventLogMonitor() {
    StartMonitoring();
}

EventLogMonitor::~EventLogMonitor() {
    StopMonitoring();
}

void EventLogMonitor::StartMonitoring() {
    m_workerThread = std::thread(&EventLogMonitor::MonitoringThread, this);
}

void EventLogMonitor::StopMonitoring() {
    m_running = false;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void EventLogMonitor::MonitoringThread() {
    while (m_running) {
        DWORD wheaCount = QueryEventLogCount(L"WHEA-Logger");
        DWORD nvCount = QueryEventLogCount(L"nvlddmkm");
        
        m_wheaCount.store(wheaCount);
        m_nvlddmkmCount.store(nvCount);
        
        // 休眠30秒
        for (int i = 0; i < 300 && m_running; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

DWORD EventLogMonitor::QueryEventLogCount(LPCWSTR provider_name) {
    wchar_t query[256];
    swprintf_s(query, 
        L"*[System[Provider[@Name='%s'] and TimeCreated[timediff(@SystemTime) <= 86400000]]]",
        provider_name);
        
    EVT_HANDLE hResults = EvtQuery(NULL, L"System", query, 
        EvtQueryChannelPath | EvtQueryReverseDirection);
    if (hResults == NULL) return 0;

    DWORD total_count = 0;
    EVT_HANDLE hEvents[256];
    DWORD returned = 0;
    
    while (EvtNext(hResults, ARRAYSIZE(hEvents), hEvents, 1000, 0, &returned)) {
        total_count += returned;
        for (DWORD i = 0; i < returned; i++) {
            EvtClose(hEvents[i]);
        }
    }
    
    EvtClose(hResults);
    return total_count;
}

// =================================================================
// CCpuUsageItem implementation - 完全优化版本
// =================================================================

// 静态成员定义
HFONT CCpuUsageItem::s_symbolFont = nullptr;
int CCpuUsageItem::s_fontRefCount = 0;

constexpr decltype(CCpuUsageItem::s_colorMap) CCpuUsageItem::s_colorMap;

CCpuUsageItem::CCpuUsageItem(int core_index, bool is_e_core) 
    : m_core_index(core_index), m_is_e_core(is_e_core),
      m_cachedBgBrush(nullptr), m_cachedBarBrush(nullptr),
      m_lastBgColor(0), m_lastBarColor(0), m_lastDarkMode(false)
{
    if (s_symbolFont == nullptr) {
        s_symbolFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
    }
    s_fontRefCount++;
    
    swprintf_s(m_item_name, L"CPU Core %d", m_core_index);
    swprintf_s(m_item_id, L"cpu_core_%d", m_core_index);
}

CCpuUsageItem::~CCpuUsageItem() {
    if (m_cachedBgBrush) DeleteObject(m_cachedBgBrush);
    if (m_cachedBarBrush) DeleteObject(m_cachedBarBrush);
    if (m_memDC) DeleteDC(m_memDC);
    if (m_memBitmap) DeleteObject(m_memBitmap);
    
    s_fontRefCount--;
    if (s_fontRefCount == 0 && s_symbolFont) {
        DeleteObject(s_symbolFont);
        s_symbolFont = nullptr;
    }
}

const wchar_t* CCpuUsageItem::GetItemName() const { return m_item_name; }
const wchar_t* CCpuUsageItem::GetItemId() const { return m_item_id; }
const wchar_t* CCpuUsageItem::GetItemLableText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueText() const { return L""; }
const wchar_t* CCpuUsageItem::GetItemValueSampleText() const { return L""; }
bool CCpuUsageItem::IsCustomDraw() const { return true; }
int CCpuUsageItem::GetItemWidth() const {