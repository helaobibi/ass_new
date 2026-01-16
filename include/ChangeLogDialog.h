/**
 * @file ChangeLogDialog.h
 * @brief 变更日志查看对话框
 *
 * 用于查看资产变更历史记录
 */

#ifndef CHANGELOGDIALOG_H
#define CHANGELOGDIALOG_H

#include "models.h"
#include "database.h"
#include <windows.h>
#include <commctrl.h>
#include <vector>

/**
 * @brief 变更日志查看对话框类
 */
class ChangeLogDialog {
public:
    ChangeLogDialog(Database& db);
    ~ChangeLogDialog();

    /**
     * @brief 显示对话框（查看所有变更日志）
     */
    void Show(HWND hParent);

    /**
     * @brief 显示对话框（查看指定资产的变更日志）
     */
    void ShowForAsset(HWND hParent, int assetId);

private:
    Database& m_db;
    HWND m_hDlg;
    HWND m_hList;
    HWND m_hSearchEdit;

    std::vector<AssetChangeLog> m_logs;
    int m_filterAssetId;  // -1 表示显示所有

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
     * @brief 加载变更日志
     */
    void LoadChangeLogs();

    /**
     * @brief 刷新列表视图
     */
    void RefreshListView();

    /**
     * @brief 搜索变更日志
     */
    void OnSearch();

    /**
     * @brief 清除搜索
     */
    void OnClearSearch();
};

#endif  // CHANGELOGDIALOG_H
