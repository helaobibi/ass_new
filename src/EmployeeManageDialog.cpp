/**
 * @file EmployeeManageDialog.cpp
 * @brief 人员管理对话框实现
 */

#include "EmployeeManageDialog.h"
#include <commctrl.h>
#include <windowsx.h>
#include <unordered_map>

EmployeeManageDialog::EmployeeManageDialog(Database& db)
    : m_db(db)
    , m_hDlg(nullptr)
    , m_hList(nullptr)
    , m_hEditName(nullptr)
    , m_hComboDept(nullptr)
    , m_hDeptList(nullptr)
    , m_hEditDeptName(nullptr)
    , m_hSearchEdit(nullptr)
    , m_selectedId(-1)
    , m_selectedDeptId(-1)
{
}

EmployeeManageDialog::~EmployeeManageDialog() {
}

void EmployeeManageDialog::Show(HWND hParent) {
    DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_EMPLOYEE_MANAGE),
        hParent,
        DialogProc,
        (LPARAM)this
    );
}

INT_PTR CALLBACK EmployeeManageDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    EmployeeManageDialog* pThis = nullptr;

    if (uMsg == WM_INITDIALOG) {
        pThis = (EmployeeManageDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hDlg = hDlg;
    } else {
        pThis = (EmployeeManageDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR EmployeeManageDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            InitDialog();
            return TRUE;

        case WM_COMMAND: {
            WORD wmId = LOWORD(wParam);
            WORD wmEvent = HIWORD(wParam);
            switch (wmId) {
                case IDE_BTN_ADD:
                    OnAdd();
                    return TRUE;

                case IDE_BTN_EDIT:
                    OnEdit();
                    return TRUE;

                case IDE_BTN_DELETE:
                    OnDelete();
                    return TRUE;

                case IDE_BTN_ADD_DEPT:
                    OnAddDepartment();
                    return TRUE;

                case IDE_BTN_DEL_DEPT:
                    OnDeleteDepartment();
                    return TRUE;

                case IDE_EDIT_SEARCH:
                    if (wmEvent == EN_CHANGE) {
                        LoadEmployees();
                    }
                    return TRUE;

                case IDOK:
                case IDCANCEL:
                    EndDialog(m_hDlg, LOWORD(wParam));
                    return TRUE;
            }
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            if (pnmhdr->idFrom == IDC_LIST_EMPLOYEES) {
                if (pnmhdr->code == LVN_ITEMCHANGED) {
                    LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
                    if (pnmlv->uNewState & LVIS_SELECTED) {
                        int idx = pnmlv->iItem;
                        if (idx >= 0 && idx < (int)m_employees.size()) {
                            m_selectedId = m_employees[idx].id;
                            wchar_t buf[256];

                            MultiByteToWideChar(65001, 0, m_employees[idx].name.c_str(),
                                              -1, buf, 256);
                            SetWindowTextW(m_hEditName, buf);

                            // 设置部门
                            int deptIdx = -1;
                            for (size_t i = 0; i < m_departments.size(); i++) {
                                if (m_departments[i].id == m_employees[idx].departmentId) {
                                    deptIdx = (int)i;
                                    break;
                                }
                            }
                            ComboBox_SetCurSel(m_hComboDept, deptIdx >= 0 ? deptIdx : 0);
                        }
                    }
                } else if (pnmhdr->code == NM_DBLCLK) {
                    OnEdit();
                }
            } else if (pnmhdr->idFrom == IDC_LIST_DEPARTMENTS) {
                if (pnmhdr->code == LVN_ITEMCHANGED) {
                    LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
                    if (pnmlv->uNewState & LVIS_SELECTED) {
                        int idx = pnmlv->iItem;
                        if (idx >= 0 && idx < (int)m_departments.size()) {
                            m_selectedDeptId = m_departments[idx].id;
                            LoadEmployees();  // 选中部门时刷新员工列表
                        }
                    } else if (!(pnmlv->uNewState & LVIS_SELECTED) && (pnmlv->uOldState & LVIS_SELECTED)) {
                        // 取消选中时显示全部员工
                        m_selectedDeptId = -1;
                        LoadEmployees();
                    }
                }
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(m_hDlg, IDCANCEL);
            return TRUE;
    }

    return FALSE;
}

void EmployeeManageDialog::InitDialog() {
    // 获取控件
    m_hList = GetDlgItem(m_hDlg, IDC_LIST_EMPLOYEES);
    m_hEditName = GetDlgItem(m_hDlg, IDE_EDIT_NAME);
    m_hComboDept = GetDlgItem(m_hDlg, IDE_COMBO_DEPT);
    m_hDeptList = GetDlgItem(m_hDlg, IDC_LIST_DEPARTMENTS);
    m_hEditDeptName = GetDlgItem(m_hDlg, IDE_EDIT_DEPT_NAME);

    // 强制设置部门列表为Report视图模式
    LONG style = GetWindowLong(m_hDeptList, GWL_STYLE);
    style &= ~LVS_TYPEMASK;
    style |= LVS_REPORT | LVS_SINGLESEL;
    SetWindowLong(m_hDeptList, GWL_STYLE, style);

    // 设置部门列表视图样式
    ListView_SetExtendedListViewStyle(
        m_hDeptList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );

    // 检查控件是否有效
    if (!IsWindow(m_hDeptList)) {
        return;
    }

    // 添加部门列表列
    LVCOLUMNW lvcDept = {};
    lvcDept.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvcDept.pszText = (wchar_t*)L"部门名称";
    lvcDept.cx = 100;
    lvcDept.iSubItem = 0;
    ListView_InsertColumn(m_hDeptList, 0, &lvcDept);

    // 设置员工列表视图样式
    ListView_SetExtendedListViewStyle(
        m_hList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );

    // 添加员工列表列
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    const wchar_t* colNames[] = {L"姓名", L"部门", L"设备数量"};
    int colWidths[] = {100, 100, 80};

    for (int i = 0; i < 3; i++) {
        lvc.pszText = (wchar_t*)colNames[i];
        lvc.cx = colWidths[i];
        lvc.iSubItem = i;
        ListView_InsertColumn(m_hList, i, &lvc);
    }

    // 加载部门列表
    LoadDepartments();
    LoadEmployees();
}

void EmployeeManageDialog::LoadEmployees() {
    std::vector<Employee> allEmployees = m_db.GetAllEmployees();

    // 根据选中的部门筛选
    m_employees.clear();
    if (m_selectedDeptId >= 0) {
        m_employees.reserve(allEmployees.size());
        for (auto& emp : allEmployees) {
            if (emp.departmentId == m_selectedDeptId) {
                m_employees.push_back(std::move(emp));
            }
        }
    } else {
        m_employees = std::move(allEmployees);
    }

    RefreshList();
}

void EmployeeManageDialog::RefreshList() {
    ListView_DeleteAllItems(m_hList);

    // 建立部门ID到名称的映射，避免重复查找
    std::unordered_map<int, const std::string*> deptMap;
    for (const auto& dept : m_departments) {
        deptMap[dept.id] = &dept.name;
    }

    // 批量获取所有员工的资产数量，避免 N+1 查询
    std::unordered_map<int, int> assetCounts = m_db.GetAllEmployeeAssetCounts();

    wchar_t buf[256];
    for (size_t i = 0; i < m_employees.size(); i++) {
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;

        // 姓名
        MultiByteToWideChar(65001, 0, m_employees[i].name.c_str(), -1, buf, 256);
        lvi.pszText = buf;
        ListView_InsertItem(m_hList, &lvi);

        // 部门名称 - O(1) 查找
        auto it = deptMap.find(m_employees[i].departmentId);
        if (it != deptMap.end()) {
            MultiByteToWideChar(65001, 0, it->second->c_str(), -1, buf, 256);
        } else {
            buf[0] = L'\0';
        }
        ListView_SetItemText(m_hList, i, 1, buf);

        // 设备数量 - O(1) 查找
        auto countIt = assetCounts.find(m_employees[i].id);
        int assetCount = (countIt != assetCounts.end()) ? countIt->second : 0;
        _itow_s(assetCount, buf, 10);
        ListView_SetItemText(m_hList, i, 2, buf);
    }

    ClearEditFields();
}

void EmployeeManageDialog::ClearEditFields() {
    m_selectedId = -1;
    SetWindowText(m_hEditName, L"");
    ComboBox_SetCurSel(m_hComboDept, 0);
}

void EmployeeManageDialog::OnAdd() {
    wchar_t buf[256];
    GetWindowTextW(m_hEditName, buf, 256);

    if (wcslen(buf) == 0) {
        MessageBoxW(m_hDlg, L"请输入员工姓名", L"提示", MB_OK | MB_ICONWARNING);
        SetFocus(m_hEditName);
        return;
    }

    char mbBuf[512];
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string name = mbBuf;

    // 获取部门
    int deptIdx = ComboBox_GetCurSel(m_hComboDept);
    int deptId = -1;
    if (deptIdx > 0 && deptIdx - 1 < (int)m_departments.size()) {
        deptId = m_departments[deptIdx - 1].id;
    }

    Employee emp{0, name, deptId};

    if (m_db.AddEmployee(emp)) {
        LoadEmployees();
    } else {
        MessageBoxW(m_hDlg, L"添加失败", L"错误", MB_OK | MB_ICONERROR);
    }
}

void EmployeeManageDialog::OnEdit() {
    if (m_selectedId < 0) {
        MessageBoxW(m_hDlg, L"请先选择要编辑的员工", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    wchar_t buf[256];
    GetWindowTextW(m_hEditName, buf, 256);

    if (wcslen(buf) == 0) {
        MessageBoxW(m_hDlg, L"请输入员工姓名", L"提示", MB_OK | MB_ICONWARNING);
        SetFocus(m_hEditName);
        return;
    }

    char mbBuf[512];
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string name = mbBuf;

    // 获取部门
    int deptIdx = ComboBox_GetCurSel(m_hComboDept);
    int deptId = -1;
    if (deptIdx > 0 && deptIdx - 1 < (int)m_departments.size()) {
        deptId = m_departments[deptIdx - 1].id;
    }

    Employee emp{m_selectedId, name, deptId};

    if (m_db.UpdateEmployee(emp)) {
        LoadEmployees();
    } else {
        MessageBoxW(m_hDlg, L"更新失败", L"错误", MB_OK | MB_ICONERROR);
    }
}

void EmployeeManageDialog::OnDelete() {
    if (m_selectedId < 0) {
        MessageBoxW(m_hDlg, L"请先选择要删除的员工", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    int result = MessageBoxW(m_hDlg, L"确定要删除选中的员工吗？\n其名下资产将被清空。", L"确认",
        MB_YESNO | MB_ICONQUESTION);

    if (result == IDYES) {
        if (m_db.DeleteEmployee(m_selectedId)) {
            LoadEmployees();
        } else {
            MessageBoxW(m_hDlg, L"删除失败", L"错误",
                MB_OK | MB_ICONERROR);
        }
    }
}

// 加载部门列表
void EmployeeManageDialog::LoadDepartments() {
    m_departments = m_db.GetAllDepartments();
    RefreshDepartmentList();

    // 更新部门下拉框
    ComboBox_ResetContent(m_hComboDept);
    ComboBox_AddString(m_hComboDept, L"");
    for (const auto& dept : m_departments) {
        wchar_t wname[256];
        MultiByteToWideChar(65001, 0, dept.name.c_str(), -1, wname, 256);
        ComboBox_AddString(m_hComboDept, wname);
    }
    ComboBox_SetCurSel(m_hComboDept, 0);
}

// 刷新部门列表
void EmployeeManageDialog::RefreshDepartmentList() {
    if (m_hDeptList == NULL) {
        return;
    }

    ListView_DeleteAllItems(m_hDeptList);

    for (size_t i = 0; i < m_departments.size(); i++) {
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;
        lvi.pszText = (wchar_t*)L"";

        int idx = ListView_InsertItem(m_hDeptList, &lvi);

        if (idx >= 0) {
            wchar_t buf[256];
            MultiByteToWideChar(65001, 0, m_departments[i].name.c_str(), -1, buf, 256);
            ListView_SetItemText(m_hDeptList, idx, 0, buf);
        }
    }
}

// 添加部门
void EmployeeManageDialog::OnAddDepartment() {
    wchar_t buf[256];
    GetWindowTextW(m_hEditDeptName, buf, 256);

    if (wcslen(buf) == 0) {
        MessageBoxW(m_hDlg, L"请输入部门名称", L"提示", MB_OK | MB_ICONWARNING);
        SetFocus(m_hEditDeptName);
        return;
    }

    char mbBuf[512];
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string name = mbBuf;

    Department dept{0, name};

    if (m_db.AddDepartment(dept)) {
        LoadDepartments();
        SetWindowTextW(m_hEditDeptName, L"");
    } else {
        MessageBoxW(m_hDlg, L"添加失败（可能部门名称已存在）", L"错误", MB_OK | MB_ICONERROR);
    }
}

// 删除部门
void EmployeeManageDialog::OnDeleteDepartment() {
    if (m_selectedDeptId < 0) {
        MessageBoxW(m_hDlg, L"请先选择要删除的部门", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    int result = MessageBoxW(m_hDlg, L"确定要删除选中的部门吗？\n该部门下的员工将变为无部门。", L"确认",
        MB_YESNO | MB_ICONQUESTION);

    if (result == IDYES) {
        if (m_db.DeleteDepartment(m_selectedDeptId)) {
            m_selectedDeptId = -1;
            LoadDepartments();
            LoadEmployees();  // 刷新员工列表以更新部门显示
        } else {
            MessageBoxW(m_hDlg, L"删除失败", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}
