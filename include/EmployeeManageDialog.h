/**
 * @file EmployeeManageDialog.h
 * @brief 人员管理对话框
 *
 * 用于管理员工的对话框
 */

#ifndef EMPLOYEEMANAGEDIALOG_H
#define EMPLOYEEMANAGEDIALOG_H

#include "models.h"
#include "database.h"
#include <windows.h>
#include <vector>

// 控件ID定义
#include "resource_ids.h"

/**
 * @brief 人员管理对话框类
 */
class EmployeeManageDialog {
public:
    EmployeeManageDialog(Database& db);
    ~EmployeeManageDialog();

    /**
     * @brief 显示对话框
     */
    void Show(HWND hParent);

private:
    Database& m_db;
    HWND m_hDlg;
    HWND m_hList;
    HWND m_hEditName;
    HWND m_hComboDept;
    HWND m_hDeptList;
    HWND m_hEditDeptName;
    HWND m_hSearchEdit;           // 搜索框

    std::vector<Employee> m_employees;
    std::vector<Department> m_departments;
    int m_selectedId;
    int m_selectedDeptId;

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
     * @brief 加载员工列表
     */
    void LoadEmployees();

    /**
     * @brief 刷新列表
     */
    void RefreshList();

    /**
     * @brief 添加员工
     */
    void OnAdd();

    /**
     * @brief 编辑员工
     */
    void OnEdit();

    /**
     * @brief 删除员工
     */
    void OnDelete();

    /**
     * @brief 加载部门列表
     */
    void LoadDepartments();

    /**
     * @brief 刷新部门列表
     */
    void RefreshDepartmentList();

    /**
     * @brief 添加部门
     */
    void OnAddDepartment();

    /**
     * @brief 删除部门
     */
    void OnDeleteDepartment();

    /**
     * @brief 清空编辑区
     */
    void ClearEditFields();
};

#endif  // EMPLOYEEMANAGEDIALOG_H
