/**
 * @file AssetEditDialog.h
 * @brief 资产编辑对话框
 *
 * 用于新增和编辑固定资产的模态对话框
 */

#ifndef ASSETEDITDIALOG_H
#define ASSETEDITDIALOG_H

#include "models.h"
#include "database.h"
#include <windows.h>
#include <string>

// 控件ID定义
#include "resource_ids.h"
#define IDA_EDIT_NAME          3002
#define IDA_COMBO_CATEGORY     3003
#define IDA_EDIT_USER          3004
#define IDA_COMBO_DEPT         3005
#define IDA_EDIT_DATE          3006
#define IDA_EDIT_PRICE         3007
#define IDA_EDIT_LOCATION      3008
#define IDA_COMBO_STATUS       3009
#define IDA_EDIT_REMARK        3010
#define IDA_BTN_BROWSE_USER    3011
#define IDA_STATIC_DEPT        3012

/**
 * @brief 资产编辑对话框类
 */
class AssetEditDialog {
public:
    AssetEditDialog(Database& db);
    ~AssetEditDialog();

    /**
     * @brief 显示对话框（新增模式）
     * @param copyFromId 可选，复制此资产的信息（-1表示不复制）
     * @return 成功返回 true
     */
    bool ShowAdd(HWND hParent, int copyFromId = -1);

    /**
     * @brief 显示对话框（编辑模式）
     * @param assetId 要编辑的资产ID
     * @return 成功返回 true
     */
    bool ShowEdit(HWND hParent, int assetId);

private:
    Database& m_db;
    HWND m_hDlg;
    int m_assetId;            // -1 表示新增模式
    Asset m_asset;

    std::vector<Category> m_categories;
    std::vector<Department> m_departments;

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
     * @brief 加载资产数据（编辑模式）
     */
    void LoadAssetData();

    /**
     * @brief 保存资产数据
     */
    bool SaveAsset();

    /**
     * @brief 验证输入
     */
    bool ValidateInput();

    /**
     * @brief 生成下一个资产编号
     */
    std::string GetNextAssetCode();

    /**
     * @brief 获取用户输入的使用人姓名
     */
    std::string GetUserName();

    /**
     * @brief 根据姓名查找员工ID
     */
    int FindEmployeeId(const std::string& name, int deptId);
};

#endif  // ASSETEDITDIALOG_H
