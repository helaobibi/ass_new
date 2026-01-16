/**
 * @file MainWindow.h
 * @brief 固定资产管理系统 - 主窗口
 *
 * Win32 API 实现的主窗口，包含：
 * - 菜单栏
 * - 工具栏
 * - 搜索和筛选面板
 * - 资产列表视图
 * - 状态栏
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>

#include "models.h"
#include "database.h"

// 前向声明
class AssetEditDialog;
class CategoryManageDialog;
class EmployeeManageDialog;
class ChangeLogDialog;

// 控件ID定义
#define ID_LISTVIEW_ASSETS     1001
#define ID_EDIT_SEARCH         1002
#define ID_COMBO_CATEGORY      1003
#define ID_COMBO_STATUS        1004
#define ID_STATUSBAR           1005

// 菜单ID定义
#define IDM_FILE_EXIT          2000
#define IDM_ASSET_ADD          2100
#define IDM_ASSET_EDIT         2101
#define IDM_ASSET_DELETE       2102
#define IDM_CATEGORY_MANAGE    2200
#define IDM_EMPLOYEE_MANAGE    2201
#define IDM_IMPORT_CSV         2300
#define IDM_EXPORT_CSV         2301
#define IDM_DOWNLOAD_TEMPLATE  2302
#define IDM_REFRESH            2400
#define IDM_CLEAR_FILTERS      2401
#define IDM_CHANGELOG          2402
#define IDM_ABOUT              2500

// 列表视图列索引
enum {
    COL_ID = 0,
    COL_CODE,
    COL_NAME,
    COL_CATEGORY,
    COL_USER,
    COL_PURCHASE_DATE,
    COL_PRICE,
    COL_LOCATION,
    COL_STATUS,
    COL_REMARK
};

/**
 * @brief 主窗口类
 *
 * 使用纯 Win32 API 实现，遵循 Windows 编码规范：
 * - 类名大写开头
 * - 成员函数大写开头（PascalCase）
 * - 成员变量 m_ 前缀，驼峰命名
 */
class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    // 禁止拷贝
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    /**
     * @brief 创建并显示主窗口
     */
    bool Create(HINSTANCE hInstance);

    /**
     * @brief 进入消息循环
     */
    int MessageLoop();

private:
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    HWND m_hListView;
    HWND m_hSearchEdit;
    HWND m_hCategoryCombo;
    HWND m_hStatusCombo;
    HWND m_hStatusBar;

    Database m_db;
    std::vector<Asset> m_assets;
    std::vector<Category> m_categories;
    int m_selectedAssetId;

    // 排序状态
    int m_sortColumn;      // 当前排序列 (-1 表示无排序)
    bool m_sortAscending;  // 是否升序
    int m_sortState;       // 排序状态: 0=默认, 1=升序, 2=降序

    /**
     * @brief 注册窗口类
     */
    bool RegisterWindowClass();

    /**
     * @brief 创建窗口和控件
     */
    bool CreateControls();

    /**
     * @brief 创建菜单栏
     */
    void CreateMenus();

    /**
     * @brief 创建工具栏
     */
    void CreateToolBar();

    /**
     * @brief 初始化列表视图列
     */
    void InitListViewColumns();

    /**
     * @brief 加载数据
     */
    void LoadData();

    /**
     * @brief 刷新列表视图
     */
    void RefreshListView();

    /**
     * @brief 刷新分类下拉菜单
     */
    void RefreshCategoryCombo();

    /**
     * @brief 更新状态栏
     */
    void UpdateStatusBar();

    /**
     * @brief 获取当前搜索条件
     */
    void GetSearchConditions(std::string& searchText, int& categoryId, std::string& status);

    /**
     * @brief 添加资产
     */
    void OnAddAsset();

    /**
     * @brief 编辑资产
     */
    void OnEditAsset();

    /**
     * @brief 删除资产
     */
    void OnDeleteAsset();

    /**
     * @brief 分类管理
     */
    void OnManageCategories();

    /**
     * @brief 人员管理
     */
    void OnManageEmployees();

    /**
     * @brief 导入 CSV
     */
    void OnImportCSV();

    /**
     * @brief 导出 CSV
     */
    void OnExportCSV();

    /**
     * @brief 下载导入模板
     */
    void OnDownloadTemplate();

    /**
     * @brief 清除筛选条件
     */
    void OnClearFilters();

    /**
     * @brief 查看变更日志
     */
    void OnViewChangeLog();

    /**
     * @brief 处理列排序
     */
    void OnColumnClick(int column);

    /**
     * @brief 排序资产列表
     */
    void SortAssets();

    /**
     * @brief 重置列头文本
     */
    void ResetColumnHeaders();

    /**
     * @brief 更新列头文本（显示排序箭头）
     */
    void UpdateColumnHeader(int column, int sortState);

    /**
     * @brief 窗口过程
     */
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief 成员函数窗口过程
     */
    LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

// 全局窗口指针（用于 WindowProc）
extern MainWindow* g_mainWindow;

#endif  // MAINWINDOW_H
