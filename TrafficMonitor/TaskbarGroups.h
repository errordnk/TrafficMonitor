#pragma once
#include "DisplayItem.h"
#include "IniHelper.h"

//A group of taskbar window items. Items of one group are laid out together,
//and every non-empty group gets an equal share of the taskbar window width.
struct TaskbarGroup
{
    std::wstring name;          //group name (shown in settings)
    std::wstring icon_path;     //optional SVG file drawn at the left side of the group
    std::wstring text;          //optional text drawn at the left side of the group instead of the icon
    bool is_default{ false };   //the default group shows the items checked in the "display items" sets
    std::set<std::wstring> items;   //keys of the items of the group (not used by the default group)

    bool operator==(const TaskbarGroup& another) const
    {
        return name == another.name && icon_path == another.icon_path && text == another.text
               && is_default == another.is_default && items == another.items;
    }
};

//Group list of the taskbar window.
//There is always at least one group and exactly one of them is the default group (it can be moved, not removed): its items are the ones that are
//checked in the "display items" sets (DisplayItemSet / plugin_display_item), so old settings keep working.
//The other groups keep their own item sets. An item may be in any number of groups and is then shown once in each.
class CTaskbarGroupSet
{
public:
    CTaskbarGroupSet();

    //Total width of the taskbar window in pixels (96 dpi). 0 means automatic: the widest group times the group count.
    int window_width{ 0 };

    //Stable key of an item that is saved to the ini file
    static std::wstring ItemKey(const CommonDisplayItem& item);

    int GroupCount() const { return static_cast<int>(m_groups.size()); }
    const std::vector<TaskbarGroup>& Groups() const { return m_groups; }
    TaskbarGroup& GetGroup(int index) { return m_groups[index]; }
    const TaskbarGroup& GetGroup(int index) const { return m_groups[index]; }

    //Membership of the non-default groups (the default group is decided by the display item sets)
    bool Contains(int group_index, const CommonDisplayItem& item) const;
    void SetMember(int group_index, const CommonDisplayItem& item, bool member);
    //true if any group except the default one contains an item
    bool HasCustomGroupItems() const;
    //true if the item is in at least one group except the default one
    bool InCustomGroup(const CommonDisplayItem& item) const;

    //Adds a new empty group and returns its index
    int AddGroup(const std::wstring& name);
    //Removes a group. The default group cannot be removed.
    bool RemoveGroup(int index);
    //Moves a group up (delta = -1) or down (delta = 1). Returns the new index, or -1 if it cannot be moved.
    int MoveGroup(int index, int delta);
    bool IsDefault(int index) const { return index >= 0 && index < GroupCount() && m_groups[index].is_default; }

    void LoadFromIni(const CIniHelper& ini);
    void SaveToIni(CIniHelper& ini) const;

    bool operator==(const CTaskbarGroupSet& another) const;

private:
    std::vector<TaskbarGroup> m_groups;
};

//Rasterizes group icons (SVG files only, through Direct2D) to HICON of the requested size.
namespace TaskbarGroupIcons
{
    //"currentColor" in the SVG is drawn with the given color (the text color of the taskbar window).
    //Returns NULL if the file cannot be loaded. The icon is owned by the cache.
    HICON Get(const std::wstring& path, int size, COLORREF color);
    //Destroys all cached icons
    void Clear();
}
