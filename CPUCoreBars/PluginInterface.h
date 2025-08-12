// CPUCoreBars/PluginInterface.h (FIXED)
#pragma once

// IPluginItem: 每个显示项目的接口
class __declspec(novtable) IPluginItem
{
public:
    // FIX: Add __stdcall to all virtual functions
    virtual const wchar_t* __stdcall GetItemName() const = 0;
    virtual const wchar_t* __stdcall GetItemId() const = 0;
    virtual const wchar_t* __stdcall GetItemLableText() const = 0;
    virtual const wchar_t* __stdcall GetItemValueText() const = 0;
    virtual const wchar_t* __stdcall GetItemValueSampleText() const = 0;
    virtual bool __stdcall IsCustomDraw() const = 0;
    virtual int __stdcall GetItemWidth() const = 0;
    virtual void __stdcall DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) = 0;
};

// 插件信息索引
enum PluginInfoIndex
{
    TMI_NAME, TMI_DESCRIPTION, TMI_AUTHOR, TMI_COPYRIGHT, TMI_URL, TMI_VERSION,
};

// ITMPlugin: 插件主接口
class __declspec(novtable) ITMPlugin
{
public:
    // FIX: Add __stdcall to all virtual functions
    virtual IPluginItem* __stdcall GetItem(int index) = 0;
    virtual void __stdcall DataRequired() = 0;
    virtual const wchar_t* __stdcall GetInfo(PluginInfoIndex index) = 0;
};

// 插件必须导出的函数
// Note: The extern "C" functions will be defined in our .cpp file