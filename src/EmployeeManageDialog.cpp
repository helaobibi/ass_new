/**
 * @file EmployeeManageDialog.cpp
 * @brief 人员管理对话框实现
 */

#include "EmployeeManageDialog.h"
#include "Logger.h"
#include <commctrl.h>
#include <windowsx.h>
#include <sstream>

EmployeeManageDialog::EmployeeManageDialog(Database& db)
    : m_db(db)
    , m_hDlg(nullptr)
    , m_hList(nullptr)
    , m_hEditName(nullptr)
    , m_hComboDept(nullptr)
    , m_hEditPosition(nullptr)
    , m_hEditPhone(nullptr)
    , m_hEditEmail(nullptr)
    , m_hEditRemark(nullptr)
    , m_hDeptList(nullptr)
    , m_hEditDeptName(nullptr)
    , m_hSearchEdit(nullptr)
    , m_hFilterDeptCombo(nullptr)
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

                case IDE_COMBO_FILTER_DEPT:
                    if (wmEvent == CBN_SELCHANGE) {
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

                            MultiByteToWideChar(65001, 0, m_employees[idx].position.c_str(),
                                              -1, buf, 256);
                            SetWindowTextW(m_hEditPosition, buf);

                            MultiByteToWideChar(65001, 0, m_employees[idx].phone.c_str(),
                                              -1, buf, 256);
                            SetWindowTextW(m_hEditPhone, buf);

                            MultiByteToWideChar(65001, 0, m_employees[idx].email.c_str(),
                                              -1, buf, 256);
                            SetWindowTextW(m_hEditEmail, buf);

                            MultiByteToWideChar(65001, 0, m_employees[idx].remark.c_str(),
                                              -1, buf, 256);
                            SetWindowTextW(m_hEditRemark, buf);
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
                        }
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
    LOG_INFO("========== 初始化对话框 ==========");

    // 枚举所有控件，查找ListView
    LOG_INFO("枚举对话框中的所有控件...");
    HWND hChild = GetWindow(m_hDlg, GW_CHILD);
    int childCount = 0;
    while (hChild != NULL) {
        childCount++;
        wchar_t className[256];
        GetClassNameW(hChild, className, 256);
        int ctrlId = GetDlgCtrlID(hChild);

        char classNameMb[512];
        WideCharToMultiByte(65001, 0, className, -1, classNameMb, 512, nullptr, nullptr);

        LOG_DEBUG("控件#" + std::to_string(childCount) +
                 ": ID=" + std::to_string(ctrlId) +
                 ", 类名=" + std::string(classNameMb) +
                 ", 句柄=" + std::to_string((long long)hChild));

        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }
    LOG_INFO("共找到 " + std::to_string(childCount) + " 个子控件");

    // 获取控件
    m_hList = GetDlgItem(m_hDlg, IDC_LIST_EMPLOYEES);
    m_hEditName = GetDlgItem(m_hDlg, IDE_EDIT_NAME);
    m_hComboDept = GetDlgItem(m_hDlg, IDE_COMBO_DEPT);
    m_hEditPosition = GetDlgItem(m_hDlg, IDE_EDIT_POSITION);
    m_hEditPhone = GetDlgItem(m_hDlg, IDE_EDIT_PHONE);
    m_hEditEmail = GetDlgItem(m_hDlg, IDE_EDIT_EMAIL);
    m_hEditRemark = GetDlgItem(m_hDlg, IDE_EDIT_REMARK);
    m_hDeptList = GetDlgItem(m_hDlg, IDC_LIST_DEPARTMENTS);
    m_hEditDeptName = GetDlgItem(m_hDlg, IDE_EDIT_DEPT_NAME);

    LOG_INFO("IDC_LIST_DEPARTMENTS 的值: " + std::to_string(IDC_LIST_DEPARTMENTS));

    // 强制设置部门列表为Report视图模式
    LONG style = GetWindowLong(m_hDeptList, GWL_STYLE);
    style &= ~LVS_TYPEMASK;  // 清除视图模式位
    style |= LVS_REPORT | LVS_SINGLESEL;  // 设置为Report模式
    SetWindowLong(m_hDeptList, GWL_STYLE, style);

    // 设置部门列表视图样式
    ListView_SetExtendedListViewStyle(
        m_hDeptList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );

    // 添加部门列表列
    LOG_INFO("========== 开始添加部门列表列 ==========");
    LOG_DEBUG("部门列表控件句柄: " + std::to_string((long long)m_hDeptList));

    // 检查控件是否有效
    if (!IsWindow(m_hDeptList)) {
        LOG_ERROR("部门列表控件句柄无效！");
        return;
    }
    LOG_DEBUG("控件句柄有效");

    // 检查控件类名
    wchar_t className[256];
    GetClassNameW(m_hDeptList, className, 256);
    char classNameMb[512];
    WideCharToMultiByte(65001, 0, className, -1, classNameMb, 512, nullptr, nullptr);
    LOG_DEBUG("控件类名: " + std::string(classNameMb));

    // 获取Header控件
    HWND hHeader = ListView_GetHeader(m_hDeptList);
    LOG_DEBUG("Header控件句柄: " + std::to_string((long long)hHeader));
    if (hHeader == NULL) {
        LOG_ERROR("无法获取Header控件！");
    }

    // 添加列
    LVCOLUMNW lvcDept = {};
    lvcDept.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvcDept.pszText = (wchar_t*)L"部门名称";
    lvcDept.cx = 100;
    lvcDept.iSubItem = 0;

    LOG_DEBUG("调用 ListView_InsertColumn...");
    int colIdx = ListView_InsertColumn(m_hDeptList, 0, &lvcDept);
    LOG_INFO("ListView_InsertColumn 返回值: " + std::to_string(colIdx));

    // 获取错误码
    DWORD err = GetLastError();
    LOG_DEBUG("GetLastError: " + std::to_string(err));

    // 验证列数（方法1：通过Header）
    int colCount1 = Header_GetItemCount(hHeader);
    LOG_INFO("方法1 - Header_GetItemCount: " + std::to_string(colCount1));

    // 验证列数（方法2：通过SendMessage）
    int colCount2 = (int)SendMessageW(hHeader, HDM_GETITEMCOUNT, 0, 0);
    LOG_INFO("方法2 - HDM_GETITEMCOUNT: " + std::to_string(colCount2));

    // 强制刷新
    InvalidateRect(m_hDeptList, NULL, TRUE);
    UpdateWindow(m_hDeptList);

    LOG_INFO("========== 部门列表列添加完成 ==========");

    // 设置员工列表视图样式
    ListView_SetExtendedListViewStyle(
        m_hList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );

    // 添加员工列表列
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    const wchar_t* colNames[] = {L"ID", L"姓名", L"部门", L"职位", L"电话"};
    int colWidths[] = {50, 80, 80, 70, 100};

    for (int i = 0; i < 5; i++) {
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
    m_employees = m_db.GetAllEmployees();
    RefreshList();
}

void EmployeeManageDialog::RefreshList() {
    ListView_DeleteAllItems(m_hList);

    for (size_t i = 0; i < m_employees.size(); i++) {
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;

        wchar_t buf[256];
        _itow_s(m_employees[i].id, buf, 10);
        lvi.pszText = buf;
        ListView_InsertItem(m_hList, &lvi);

        MultiByteToWideChar(65001, 0, m_employees[i].name.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hList, i, 1, buf);

        // 部门名称
        std::string deptName;
        for (const auto& dept : m_departments) {
            if (dept.id == m_employees[i].departmentId) {
                deptName = dept.name;
                break;
            }
        }
        MultiByteToWideChar(65001, 0, deptName.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hList, i, 2, buf);

        MultiByteToWideChar(65001, 0, m_employees[i].position.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hList, i, 3, buf);

        MultiByteToWideChar(65001, 0, m_employees[i].phone.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hList, i, 4, buf);
    }

    ClearEditFields();
}

void EmployeeManageDialog::ClearEditFields() {
    m_selectedId = -1;
    SetWindowText(m_hEditName, L"");
    ComboBox_SetCurSel(m_hComboDept, 0);
    SetWindowText(m_hEditPosition, L"");
    SetWindowText(m_hEditPhone, L"");
    SetWindowText(m_hEditEmail, L"");
    SetWindowText(m_hEditRemark, L"");
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

    // 获取其他字段
    GetWindowTextW(m_hEditPosition, buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string position = mbBuf;

    GetWindowTextW(m_hEditPhone, buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string phone = mbBuf;

    GetWindowTextW(m_hEditEmail, buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string email = mbBuf;

    GetWindowTextW(m_hEditRemark, buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string remark = mbBuf;

    Employee emp{0, name, deptId, position, phone, email, remark};

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

    // 获取其他字段
    GetWindowTextW(m_hEditPosition, buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string position = mbBuf;

    GetWindowTextW(m_hEditPhone, buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string phone = mbBuf;

    GetWindowTextW(m_hEditEmail, buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string email = mbBuf;

    GetWindowTextW(m_hEditRemark, buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string remark = mbBuf;

    Employee emp{m_selectedId, name, deptId, position, phone, email, remark};

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
    LOG_INFO("========== 开始加载部门列表 ==========");

    m_departments = m_db.GetAllDepartments();
    LOG_INFO("从数据库获取到 " + std::to_string(m_departments.size()) + " 个部门");

    for (size_t i = 0; i < m_departments.size(); i++) {
        LOG_DEBUG("部门[" + std::to_string(i) + "]: ID=" + std::to_string(m_departments[i].id) +
                 ", 名称=" + m_departments[i].name);
    }

    RefreshDepartmentList();

    // 更新部门下拉框
    LOG_DEBUG("更新部门下拉框...");
    ComboBox_ResetContent(m_hComboDept);
    ComboBox_AddString(m_hComboDept, L"");  // 空选项表示无部门
    for (const auto& dept : m_departments) {
        int len = MultiByteToWideChar(65001, 0, dept.name.c_str(), -1, nullptr, 0);
        wchar_t* wname = new wchar_t[len];
        MultiByteToWideChar(65001, 0, dept.name.c_str(), -1, wname, len);
        ComboBox_AddString(m_hComboDept, wname);
        delete[] wname;
    }
    ComboBox_SetCurSel(m_hComboDept, 0);
    LOG_INFO("部门列表加载完成");
}

// 刷新部门列表
void EmployeeManageDialog::RefreshDepartmentList() {
    LOG_INFO("========== 开始刷新部门列表显示 ==========");
    LOG_DEBUG("部门列表控件句柄: " + std::to_string((long long)m_hDeptList));

    if (m_hDeptList == NULL) {
        LOG_ERROR("部门列表控件句柄为 NULL！");
        return;
    }

    // 检查列数
    HWND hHeader = ListView_GetHeader(m_hDeptList);
    int colCount = Header_GetItemCount(hHeader);
    LOG_DEBUG("ListView 列数: " + std::to_string(colCount));

    // 检查 ListView 样式
    LONG style = GetWindowLong(m_hDeptList, GWL_STYLE);
    LOG_DEBUG("ListView 样式: 0x" + std::to_string(style));
    LOG_DEBUG("是否有 LVS_REPORT: " + std::to_string((style & LVS_REPORT) != 0));

    ListView_DeleteAllItems(m_hDeptList);
    LOG_DEBUG("已清空列表，准备插入 " + std::to_string(m_departments.size()) + " 个部门");

    for (size_t i = 0; i < m_departments.size(); i++) {
        LOG_DEBUG("插入部门[" + std::to_string(i) + "]: ID=" + std::to_string(m_departments[i].id) +
                 ", 名称=" + m_departments[i].name);

        // 先插入空项目
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;
        lvi.pszText = (wchar_t*)L"";

        int idx = ListView_InsertItem(m_hDeptList, &lvi);
        LOG_DEBUG("ListView_InsertItem 返回索引: " + std::to_string(idx));

        // 检查插入后的项目数
        int count = ListView_GetItemCount(m_hDeptList);
        LOG_DEBUG("插入后项目数: " + std::to_string(count));

        // 然后设置文本
        if (idx >= 0) {
            wchar_t buf[256];
            int len = MultiByteToWideChar(65001, 0, m_departments[i].name.c_str(), -1, buf, 256);
            LOG_DEBUG("转换后的宽字符长度: " + std::to_string(len));

            ListView_SetItemText(m_hDeptList, idx, 0, buf);
            LOG_DEBUG("ListView_SetItemText 已调用");

            // 验证文本是否设置成功
            wchar_t verify[256] = {0};
            ListView_GetItemText(m_hDeptList, idx, 0, verify, 256);
            char verifyMb[512];
            WideCharToMultiByte(65001, 0, verify, -1, verifyMb, 512, nullptr, nullptr);
            LOG_DEBUG("验证读取的文本: " + std::string(verifyMb));
        } else {
            LOG_ERROR("ListView_InsertItem 失败，返回 -1");
        }
    }

    int itemCount = ListView_GetItemCount(m_hDeptList);
    LOG_INFO("部门列表刷新完成，当前项目数: " + std::to_string(itemCount));
}

// 添加部门
void EmployeeManageDialog::OnAddDepartment() {
    LOG_INFO("========== 开始添加部门 ==========");

    wchar_t buf[256];
    GetWindowTextW(m_hEditDeptName, buf, 256);

    if (wcslen(buf) == 0) {
        LOG_WARNING("部门名称为空，取消添加");
        MessageBoxW(m_hDlg, L"请输入部门名称", L"提示", MB_OK | MB_ICONWARNING);
        SetFocus(m_hEditDeptName);
        return;
    }

    char mbBuf[512];
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string name = mbBuf;
    LOG_INFO("准备添加部门: " + name);

    Department dept{0, name};

    LOG_DEBUG("调用 Database::AddDepartment()...");
    if (m_db.AddDepartment(dept)) {
        LOG_INFO("✓ 部门添加成功，新ID=" + std::to_string(dept.id));
        LoadDepartments();
        SetWindowTextW(m_hEditDeptName, L"");
    } else {
        std::string dbError = m_db.GetLastError();
        LOG_ERROR("✗ 部门添加失败: " + dbError);
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
