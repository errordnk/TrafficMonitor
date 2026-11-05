#pragma once
#include "DisplayItem.h"

class CTaskbarItemOrderHelper
{
public:
    CTaskbarItemOrderHelper(bool displayed_only = false);

    void Init();

    //根据设定的顺序获取任务窗口每个显示项目
    std::vector<CommonDisplayItem> GetAllDisplayItemsWithOrder() const;

    //Moves the item at display_index (an index in GetAllDisplayItemsWithOrder) up (delta = -1) or down (delta = 1).
    //Returns the new display index, or -1 if the item cannot be moved.
    int MoveDisplayedItem(int display_index, int delta);

    void FromString(const std::wstring& str);
    std::wstring ToString() const;
    void SetOrder(const vector<int>& item_order);
    const vector<int>& GetItemOrderConst() const;
    vector<int>& GetItemOrder();

    static CString GetItemDisplayName(CommonDisplayItem item);
    static bool IsItemDisplayed(CommonDisplayItem item);

private:
    //规范m_item_order里的项目，如果m_item_order里有序号超过了显示项目的个数，则将其移除，并在后面添加缺少的项目的序号
    void NormalizeItemOrder();

private:
    vector<int> m_item_order;   //保存每个项目的顺序
    vector<CommonDisplayItem> m_all_item_in_default_order;    //以默认顺序保存的所有显示项目
    bool m_displayed_only;
};
