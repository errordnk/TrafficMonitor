#pragma once
#include "BaseDialog.h"
#include "TaskbarGroups.h"
#include "TaskbarItemOrderHelper.h"

struct TaskBarSettingData;

//The only dialog that configures which items the taskbar window shows and how they are arranged.
//It manages the groups (add/remove/reorder, name, SVG icon or text drawn instead of the icon), the items
//of every group, the order of the items and the width of the taskbar window.
//An item can be in several groups at once; then it is shown once in each of them.
//The default group shows the items checked in the display item sets of the taskbar settings,
//so this dialog edits those sets too.
class CTaskbarGroupsDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CTaskbarGroupsDlg)

public:
    //Shows the dialog for the taskbar settings and stores the result in data when the user clicks OK.
    //Returns true if data was changed.
    static bool Edit(TaskBarSettingData& data, CWnd* pParent = nullptr);

    CTaskbarGroupsDlg(const TaskBarSettingData& data, CWnd* pParent = nullptr);
    virtual ~CTaskbarGroupsDlg();

    const CTaskbarGroupSet& GetGroups() const { return m_groups; }
    const DisplayItemSet& GetDisplayItem() const { return m_display_item; }
    const StringSet& GetPluginDisplayItem() const { return m_plugin_item; }
    const std::vector<int>& GetItemOrder() const { return m_item_order.GetItemOrderConst(); }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    void FillGroupList(int select_index);
    void ShowCurrentGroup();
    void UpdateButtons();
    int CurrentGroup() const;
    std::wstring GroupDisplayName(int index) const;
    bool IsMember(int group, const CommonDisplayItem& item) const;
    void SetMember(int group, const CommonDisplayItem& item, bool member);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnLbnSelchangeGroupList();
    afx_msg void OnLbnSelchangeItemsList();
    afx_msg void OnBnClickedAdd();
    afx_msg void OnBnClickedRemove();
    afx_msg void OnBnClickedGroupUp();
    afx_msg void OnBnClickedGroupDown();
    afx_msg void OnBnClickedItemUp();
    afx_msg void OnBnClickedItemDown();
    afx_msg void OnEnChangeName();
    afx_msg void OnEnChangeIcon();
    afx_msg void OnEnChangeText();
    afx_msg void OnBnClickedBrowseIcon();
    afx_msg void OnItemCheckChanged();

private:
    CTaskbarGroupSet m_groups;
    DisplayItemSet m_display_item;              //items of the default group (built-in items)
    StringSet m_plugin_item;                    //items of the default group (plugin items)
    CTaskbarItemOrderHelper m_item_order;       //order of the items (shared by all groups)
    std::vector<CommonDisplayItem> m_items;     //all items that can be put into groups, in display order
    CListBox m_group_list;
    CCheckListBox m_items_list;
    bool m_updating{ false };                   //true while the controls are being filled by code
};
