// 美化后的树叶图标绘制函数
void CCpuUsageItem::DrawLeafIcon(HDC hDC, const RECT& rect, bool dark_mode)
{
    // 计算图标大小和位置
    int center_x = rect.left + (rect.right - rect.left) / 2;
    int center_y = rect.top + (rect.bottom - rect.top) / 2;
    int leaf_width = min(8, (rect.right - rect.left) / 2);
    int leaf_height = min(10, (rect.bottom - rect.top) / 2);
    
    // 颜色设置 - 更加精美的绿色系
    COLORREF leaf_color = dark_mode ? RGB(85, 170, 85) : RGB(76, 175, 80);      // 叶子主色
    COLORREF stem_color = dark_mode ? RGB(101, 67, 33) : RGB(139, 95, 51);      // 茎的颜色
    COLORREF vein_color = dark_mode ? RGB(70, 140, 70) : RGB(56, 142, 60);      // 叶脉颜色
    
    // 创建画笔和画刷
    HPEN stem_pen = CreatePen(PS_SOLID, 2, stem_color);
    HPEN leaf_pen = CreatePen(PS_SOLID, 1, leaf_color);
    HPEN vein_pen = CreatePen(PS_SOLID, 1, vein_color);
    HBRUSH leaf_brush = CreateSolidBrush(leaf_color);
    
    HPEN old_pen = (HPEN)SelectObject(hDC, stem_pen);
    HBRUSH old_brush = (HBRUSH)SelectObject(hDC, leaf_brush);
    
    // 1. 绘制茎部 - 稍微粗一些，更立体
    SelectObject(hDC, stem_pen);
    MoveToEx(hDC, center_x, center_y + leaf_height/2 + 3, NULL);
    LineTo(hDC, center_x, center_y - leaf_height/2 - 1);
    
    // 2. 绘制叶子形状 - 使用贝塞尔曲线创建更自然的叶子
    SelectObject(hDC, leaf_pen);
    
    // 定义叶子的关键点
    POINT leaf_points[7];
    leaf_points[0] = {center_x, center_y - leaf_height/2};           // 顶点
    leaf_points[1] = {center_x + leaf_width/2, center_y - leaf_height/4}; // 右上
    leaf_points[2] = {center_x + leaf_width/2, center_y + leaf_height/4}; // 右下
    leaf_points[3] = {center_x, center_y + leaf_height/2};           // 底点
    leaf_points[4] = {center_x - leaf_width/2, center_y + leaf_height/4}; // 左下
    leaf_points[5] = {center_x - leaf_width/2, center_y - leaf_height/4}; // 左上
    leaf_points[6] = {center_x, center_y - leaf_height/2};           // 回到顶点
    
    // 绘制并填充叶子
    Polygon(hDC, leaf_points, 6);
    
    // 3. 绘制叶脉 - 增加细节
    SelectObject(hDC, vein_pen);
    
    // 主叶脉 (中央)
    MoveToEx(hDC, center_x, center_y - leaf_height/2, NULL);
    LineTo(hDC, center_x, center_y + leaf_height/2);
    
    // 侧叶脉 (左侧)
    MoveToEx(hDC, center_x, center_y - leaf_height/4, NULL);
    LineTo(hDC, center_x - leaf_width/3, center_y);
    MoveToEx(hDC, center_x, center_y, NULL);
    LineTo(hDC, center_x - leaf_width/3, center_y + leaf_height/4);
    
    // 侧叶脉 (右侧)
    MoveToEx(hDC, center_x, center_y - leaf_height/4, NULL);
    LineTo(hDC, center_x + leaf_width/3, center_y);
    MoveToEx(hDC, center_x, center_y, NULL);
    LineTo(hDC, center_x + leaf_width/3, center_y + leaf_height/4);
    
    // 4. 添加高光效果 (可选)
    if (!dark_mode) {
        COLORREF highlight_color = RGB(144, 238, 144); // 浅绿色高光
        HPEN highlight_pen = CreatePen(PS_SOLID, 1, highlight_color);
        SelectObject(hDC, highlight_pen);
        
        // 在叶子左上角添加小高光
        MoveToEx(hDC, center_x - 1, center_y - leaf_height/3, NULL);
        LineTo(hDC, center_x - 2, center_y - leaf_height/3 + 2);
        
        DeleteObject(highlight_pen);
    }
    
    // 恢复原始对象并清理
    SelectObject(hDC, old_pen);
    SelectObject(hDC, old_brush);
    DeleteObject(stem_pen);
    DeleteObject(leaf_pen);
    DeleteObject(vein_pen);
    DeleteObject(leaf_brush);
}

// 进一步优化版本 - 使用更复杂的形状和渐变效果
void CCpuUsageItem::DrawAdvancedLeafIcon(HDC hDC, const RECT& rect, bool dark_mode)
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
    
    // 创建渐变效果的画刷 (简化版本，使用分层绘制)
    HBRUSH dark_brush = CreateSolidBrush(dark_color);
    HBRUSH base_brush = CreateSolidBrush(base_color);
    HBRUSH light_brush = CreateSolidBrush(light_color);
    HPEN stem_pen = CreatePen(PS_SOLID, 2, stem_color);
    HPEN leaf_pen = CreatePen(PS_SOLID, 1, base_color);
    
    HPEN old_pen = (HPEN)SelectObject(hDC, stem_pen);
    HBRUSH old_brush = (HBRUSH)SelectObject(hDC, base_brush);
    
    // 1. 绘制茎部
    MoveToEx(hDC, center_x, center_y + leaf_height/2 + 3, NULL);
    LineTo(hDC, center_x, center_y - leaf_height/2);
    
    // 2. 绘制叶子的阴影层 (稍微偏移)
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
    
    // 5. 绘制叶脉
    SelectObject(hDC, leaf_pen);
    // 主脉
    MoveToEx(hDC, center_x, center_y - leaf_height/2, NULL);
    LineTo(hDC, center_x, center_y + leaf_height/2);
    
    // 侧脉
    MoveToEx(hDC, center_x, center_y - leaf_height/4, NULL);
    LineTo(hDC, center_x - leaf_width/3, center_y);
    MoveToEx(hDC, center_x, center_y - leaf_height/4, NULL);
    LineTo(hDC, center_x + leaf_width/3, center_y);
    MoveToEx(hDC, center_x, center_y + leaf_height/6, NULL);
    LineTo(hDC, center_x - leaf_width/4, center_y + leaf_height/3);
    MoveToEx(hDC, center_x, center_y + leaf_height/6, NULL);
    LineTo(hDC, center_x + leaf_width/4, center_y + leaf_height/3);
    
    // 恢复对象并清理
    SelectObject(hDC, old_pen);
    SelectObject(hDC, old_brush);
    DeleteObject(dark_brush);
    DeleteObject(base_brush);
    DeleteObject(light_brush);
    DeleteObject(stem_pen);
    DeleteObject(leaf_pen);
}