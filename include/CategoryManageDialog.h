/**
 * @file CategoryManageDialog.h
 * @brief 分类管理对话框
 *
 * 用于管理资产分类的对话框
 */

#ifndef CATEGORYMANAGEDIALOG_H
#define CATEGORYMANAGEDIALOG_H

#include "models.h"
#include "database.h"
#include <windows.h>
#include <vector>

// 控件ID定义
#include "resource_ids.h"
#define IDC_EDIT_CATEGORY       3102
#define IDC_BTN_ADD             3103
#define IDC_BTN_EDIT            3104
#define IDC_BTN_DELETE          3105

/**
 * @brief 分类管理对话框类
 */
class CategoryManageDialog {
public:
    CategoryManageDialog(Database& db);
    ~CategoryManageDialog();

    /**
     * @brief 显示对话框
     */
    void Show(HWND hParent);

private:
    Database& m_db;
    HWND m_hDlg;
    HWND m_hList;
    HWND m_hEdit;

    std::vector<Category> m_categories;
    int m_selectedId;

    /**
     * @brief 对话框过程
     */
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief 成员函数对话框过程
     */
    INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief 初始化对话框
     */
    void InitDialog();

    /**
     * @brief 加载分类列表
     */
    void LoadCategories();

    /**
     * @brief 刷新列表
     */
    void RefreshList();

    /**
     * @brief 添加分类
     */
    void OnAdd();

    /**
     * @brief 编辑分类
     */
    void OnEdit();

    /**
     * @brief 删除分类
     */
    void OnDelete();
};

#endif  // CATEGORYMANAGEDIALOG_H
