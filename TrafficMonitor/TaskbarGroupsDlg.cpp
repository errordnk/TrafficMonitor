#include "stdafx.h"
#include "TrafficMonitor.h"
#include "TaskbarGroupsDlg.h"

IMPLEMENT_DYNAMIC(CTaskbarGroupsDlg, CBaseDialog)

CTaskbarGroupsDlg::CTaskbarGroupsDlg(const CTaskbarGroupSet& groups,
                                     const DisplayItemSet& display_item,
                                     const StringSet& plugin_item,
                                     const std::vector<CommonDisplayItem>& items,
                                     CWnd* pParent /*=nullptr*/)
    : CBaseDialog(IDD_TASKBAR_GROUPS_DIALOG, pParent)
    , m_groups(groups)
    , m_display_item(display_item)
    , m_plugin_item(plugin_item)
    , m_items(items)
{
}

CTaskbarGroupsDlg::~CTaskbarGroupsDlg()
{
}

void CTaskbarGroupsDlg::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_GROUP_LIST, m_group_list);
    DDX_Control(pDX, IDC_GROUP_ITEMS_LIST, m_items_list);
}

BEGIN_MESSAGE_MAP(CTaskbarGroupsDlg, CBaseDialog)
    ON_LBN_SELCHANGE(IDC_GROUP_LIST, &CTaskbarGroupsDlg::OnLbnSelchangeGroupList)
    ON_BN_CLICKED(IDC_GROUP_ADD_BUTTON, &CTaskbarGroupsDlg::OnBnClickedAdd)
    ON_BN_CLICKED(IDC_GROUP_REMOVE_BUTTON, &CTaskbarGroupsDlg::OnBnClickedRemove)
    ON_EN_CHANGE(IDC_GROUP_NAME_EDIT, &CTaskbarGroupsDlg::OnEnChangeName)
    ON_EN_CHANGE(IDC_GROUP_ICON_EDIT, &CTaskbarGroupsDlg::OnEnChangeIcon)
    ON_BN_CLICKED(IDC_GROUP_ICON_BROWSE_BUTTON, &CTaskbarGroupsDlg::OnBnClickedBrowseIcon)
    ON_CLBN_CHKCHANGE(IDC_GROUP_ITEMS_LIST, &CTaskbarGroupsDlg::OnItemCheckChanged)
END_MESSAGE_MAP()

BOOL CTaskbarGroupsDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    SetIcon(theApp.GetMenuIcon(IDI_ITEM), FALSE);
    m_items_list.SetItemHeight(0, DPI(20));
    SetDlgItemInt(IDC_GROUP_WIDTH_EDIT, m_groups.window_width, FALSE);
    FillGroupList(0);
    return TRUE;
}

std::wstring CTaskbarGroupsDlg::GroupDisplayName(int index) const
{
    const std::wstring& name = m_groups.GetGroup(index).name;
    return name.empty() ? (L"Group " + std::to_wstring(index + 1)) : name;
}

int CTaskbarGroupsDlg::CurrentGroup() const
{
    int index = m_group_list.GetCurSel();
    if (index < 0 || index >= m_groups.GroupCount())
        return 0;
    return index;
}

bool CTaskbarGroupsDlg::IsMember(int group, const CommonDisplayItem& item) const
{
    if (group != 0)
        return m_groups.Contains(group, item);
    if (item.IsPlugin())
        return item.PluginItem() != nullptr && m_plugin_item.Contains(item.PluginItem()->GetItemId());
    return m_display_item.Contains(item.ItemType());
}

void CTaskbarGroupsDlg::SetMember(int group, const CommonDisplayItem& item, bool member)
{
    if (group != 0)
    {
        m_groups.SetMember(group, item, member);
    }
    else if (item.IsPlugin())
    {
        if (item.PluginItem() != nullptr)
            m_plugin_item.SetStrContained(item.PluginItem()->GetItemId(), member);
    }
    else
    {
        if (member)
            m_display_item.Add(item.ItemType());
        else
            m_display_item.Remove(item.ItemType());
    }
}

void CTaskbarGroupsDlg::FillGroupList(int select_index)
{
    m_updating = true;
    m_group_list.ResetContent();
    for (int i = 0; i < m_groups.GroupCount(); i++)
        m_group_list.AddString(GroupDisplayName(i).c_str());
    if (select_index < 0 || select_index >= m_groups.GroupCount())
        select_index = 0;
    m_group_list.SetCurSel(select_index);
    m_updating = false;
    ShowCurrentGroup();
}

void CTaskbarGroupsDlg::ShowCurrentGroup()
{
    m_updating = true;
    int group = CurrentGroup();
    const TaskbarGroup& g = m_groups.GetGroup(group);
    SetDlgItemText(IDC_GROUP_NAME_EDIT, g.name.c_str());
    SetDlgItemText(IDC_GROUP_ICON_EDIT, g.icon_path.c_str());
    EnableDlgCtrl(IDC_GROUP_REMOVE_BUTTON, group > 0);      //the default group cannot be removed

    //all items; an item is checked when it is shown in the selected group
    int top = m_items_list.GetTopIndex();
    m_items_list.ResetContent();
    for (const auto& item : m_items)
    {
        int index = m_items_list.AddString(item.GetItemName());
        m_items_list.SetCheck(index, IsMember(group, item) ? TRUE : FALSE);
    }
    if (top >= 0 && top < m_items_list.GetCount())
        m_items_list.SetTopIndex(top);
    m_updating = false;
}

void CTaskbarGroupsDlg::OnLbnSelchangeGroupList()
{
    if (!m_updating)
        ShowCurrentGroup();
}

void CTaskbarGroupsDlg::OnBnClickedAdd()
{
    int index = m_groups.AddGroup(L"Group " + std::to_wstring(m_groups.GroupCount() + 1));
    FillGroupList(index);
    GetDlgItem(IDC_GROUP_NAME_EDIT)->SetFocus();
    static_cast<CEdit*>(GetDlgItem(IDC_GROUP_NAME_EDIT))->SetSel(0, -1);
}

void CTaskbarGroupsDlg::OnBnClickedRemove()
{
    int index = CurrentGroup();
    if (m_groups.RemoveGroup(index))
        FillGroupList(min(index, m_groups.GroupCount() - 1));
}

void CTaskbarGroupsDlg::OnEnChangeName()
{
    if (m_updating)
        return;
    CString text;
    GetDlgItemText(IDC_GROUP_NAME_EDIT, text);
    int group = CurrentGroup();
    m_groups.GetGroup(group).name = text.GetString();

    //refresh the entry in the group list
    m_updating = true;
    m_group_list.DeleteString(group);
    m_group_list.InsertString(group, GroupDisplayName(group).c_str());
    m_group_list.SetCurSel(group);
    m_updating = false;
}

void CTaskbarGroupsDlg::OnEnChangeIcon()
{
    if (m_updating)
        return;
    CString text;
    GetDlgItemText(IDC_GROUP_ICON_EDIT, text);
    m_groups.GetGroup(CurrentGroup()).icon_path = text.GetString();
}

void CTaskbarGroupsDlg::OnBnClickedBrowseIcon()
{
    CFileDialog dlg(TRUE, _T("svg"), NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, _T("SVG files (*.svg)|*.svg||"), this);
    if (dlg.DoModal() == IDOK)
        SetDlgItemText(IDC_GROUP_ICON_EDIT, dlg.GetPathName());      //EN_CHANGE saves the path
}

void CTaskbarGroupsDlg::OnItemCheckChanged()
{
    if (m_updating)
        return;
    int row = m_items_list.GetCurSel();
    if (row < 0 || row >= static_cast<int>(m_items.size()))
        return;
    SetMember(CurrentGroup(), m_items[row], m_items_list.GetCheck(row) != 0);
}

void CTaskbarGroupsDlg::OnOK()
{
    for (int i = 0; i < m_groups.GroupCount(); i++)
    {
        if (m_groups.GetGroup(i).name.empty())
            m_groups.GetGroup(i).name = GroupDisplayName(i);
    }
    BOOL translated = FALSE;
    UINT width = GetDlgItemInt(IDC_GROUP_WIDTH_EDIT, &translated, FALSE);
    m_groups.window_width = translated ? static_cast<int>(min(width, 10000u)) : 0;
    CBaseDialog::OnOK();
}
