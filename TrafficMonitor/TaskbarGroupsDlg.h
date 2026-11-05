#pragma once
#include "BaseDialog.h"
#include "TaskbarGroups.h"

//Dialog to manage the groups of the taskbar window: add/remove groups, set group name and SVG icon,
//choose which items are shown in each group and set the width of the taskbar window.
//An item can be in several groups at once; then it is shown once in each of them.
//The default group (index 0) shows the items checked in the "display items" dialog,
//so this dialog edits those sets too.
class CTaskbarGroupsDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CTaskbarGroupsDlg)

public:
    CTaskbarGroupsDlg(const CTaskbarGroupSet& groups,
                      const DisplayItemSet& display_item,
                      const StringSet& plugin_item,
                      const std::vector<CommonDisplayItem>& items,
                      CWnd* pParent = nullptr);
    virtual ~CTaskbarGroupsDlg();

    const CTaskbarGroupSet& GetGroups() const { return m_groups; }
    const DisplayItemSet& GetDisplayItem() const { return m_display_item; }
    const StringSet& GetPluginDisplayItem() const { return m_plugin_item; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    void FillGroupList(int select_index);
    void ShowCurrentGroup();
    int CurrentGroup() const;
    std::wstring GroupDisplayName(int index) const;
    bool IsMember(int group, const CommonDisplayItem& item) const;
    void SetMember(int group, const CommonDisplayItem& item, bool member);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnLbnSelchangeGroupList();
    afx_msg void OnBnClickedAdd();
    afx_msg void OnBnClickedRemove();
    afx_msg void OnEnChangeName();
    afx_msg void OnEnChangeIcon();
    afx_msg void OnBnClickedBrowseIcon();
    afx_msg void OnItemCheckChanged();

private:
    CTaskbarGroupSet m_groups;
    DisplayItemSet m_display_item;              //items of the default group (built-in items)
    StringSet m_plugin_item;                    //items of the default group (plugin items)
    std::vector<CommonDisplayItem> m_items;     //all items that can be put into groups
    CListBox m_group_list;
    CCheckListBox m_items_list;
    bool m_updating{ false };                   //true while the controls are being filled by code
};
