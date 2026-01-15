/**
 * @file AssetEditDialog.cpp
 * @brief 资产编辑对话框实现
 */

#include "AssetEditDialog.h"
#include "Logger.h"
#include <commctrl.h>
#include <windowsx.h>
#include <sstream>
#include <regex>
#include <cmath>

// 对话框模板（动态创建）
static const wchar_t g_szAssetEditTemplate[] = L"AssetEditDialog";

AssetEditDialog::AssetEditDialog(Database& db)
    : m_db(db)
    , m_hDlg(nullptr)
    , m_assetId(-1)
{
}

AssetEditDialog::~AssetEditDialog() {
}

bool AssetEditDialog::ShowAdd(HWND hParent, int copyFromId) {
    m_assetId = -1;
    m_asset = Asset();

    // 如果指定了复制源，加载源资产数据
    if (copyFromId >= 0) {
        Asset sourceAsset;
        if (m_db.GetAssetById(copyFromId, sourceAsset)) {
            // 复制除ID和编号外的所有信息
            m_asset.name = sourceAsset.name;
            m_asset.categoryId = sourceAsset.categoryId;
            m_asset.userId = sourceAsset.userId;
            m_asset.userName = sourceAsset.userName;
            m_asset.departmentName = sourceAsset.departmentName;
            m_asset.purchaseDate = sourceAsset.purchaseDate;
            m_asset.price = sourceAsset.price;
            m_asset.location = sourceAsset.location;
            m_asset.status = sourceAsset.status;
            m_asset.remark = sourceAsset.remark;
        }
    }

    // 生成新编号
    m_asset.assetCode = GetNextAssetCode();

    INT_PTR result = DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_ASSET_EDIT),
        hParent,
        DialogProc,
        (LPARAM)this
    );

    return (result == IDOK);
}

bool AssetEditDialog::ShowEdit(HWND hParent, int assetId) {
    m_assetId = assetId;
    m_asset = Asset();

    if (!m_db.GetAssetById(assetId, m_asset)) {
        MessageBoxW(hParent, L"加载资产数据失败", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    INT_PTR result = DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_ASSET_EDIT),
        hParent,
        DialogProc,
        (LPARAM)this
    );

    return (result == IDOK);
}

INT_PTR CALLBACK AssetEditDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    AssetEditDialog* pThis = nullptr;

    if (uMsg == WM_INITDIALOG) {
        pThis = (AssetEditDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hDlg = hDlg;
    } else {
        pThis = (AssetEditDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR AssetEditDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            InitDialog();
            LoadAssetData();
            return TRUE;

        case WM_COMMAND: {
            WORD wmId = LOWORD(wParam);
            switch (wmId) {
                case IDOK:
                    if (SaveAsset()) {
                        EndDialog(m_hDlg, IDOK);
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog(m_hDlg, IDCANCEL);
                    return TRUE;

                case IDA_BTN_BROWSE_USER: {
                    // 选择员工对话框（简化版 - 输入姓名）
                    MessageBoxW(m_hDlg, L"请直接输入员工姓名", L"提示", MB_OK);
                    SetFocus(GetDlgItem(m_hDlg, IDA_EDIT_USER));
                    return TRUE;
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

void AssetEditDialog::InitDialog() {
    // 设置字体
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // 加载分类列表
    m_categories = m_db.GetAllCategories();
    HWND hComboCat = GetDlgItem(m_hDlg, IDA_COMBO_CATEGORY);
    ComboBox_ResetContent(hComboCat);
    for (const auto& cat : m_categories) {
        int len = MultiByteToWideChar(65001, 0, cat.name.c_str(), -1, nullptr, 0);
        wchar_t* wname = new wchar_t[len];
        MultiByteToWideChar(65001, 0, cat.name.c_str(), -1, wname, len);
        ComboBox_AddString(hComboCat, wname);
        delete[] wname;
    }

    // 加载部门列表
    m_departments = m_db.GetAllDepartments();
    HWND hComboDept = GetDlgItem(m_hDlg, IDA_COMBO_DEPT);
    ComboBox_ResetContent(hComboDept);
    ComboBox_AddString(hComboDept, L"");
    for (const auto& dept : m_departments) {
        int len = MultiByteToWideChar(65001, 0, dept.name.c_str(), -1, nullptr, 0);
        wchar_t* wname = new wchar_t[len];
        MultiByteToWideChar(65001, 0, dept.name.c_str(), -1, wname, len);
        ComboBox_AddString(hComboDept, wname);
        delete[] wname;
    }

    // 加载状态列表
    HWND hComboStatus = GetDlgItem(m_hDlg, IDA_COMBO_STATUS);
    ComboBox_ResetContent(hComboStatus);
    const wchar_t* statuses[] = {L"在用", L"闲置", L"维修中", L"已报废"};
    for (int i = 0; i < 4; i++) {
        ComboBox_AddString(hComboStatus, statuses[i]);
    }

    // 设置默认日期
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t dateBuf[32];
    swprintf_s(dateBuf, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    SetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_DATE), dateBuf);

    // 设置默认状态
    ComboBox_SetCurSel(hComboStatus, 0);  // 在用
}

void AssetEditDialog::LoadAssetData() {
    wchar_t buf[256];

    // 编号
    MultiByteToWideChar(65001, 0, m_asset.assetCode.c_str(), -1, buf, 256);
    SetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_CODE), buf);

    if (m_assetId < 0 && m_asset.name.empty()) {
        // 纯新增模式（无复制），只设置编号
        return;
    }

    // 新增复制模式或编辑模式 - 加载所有数据

    // 名称
    MultiByteToWideChar(65001, 0, m_asset.name.c_str(), -1, buf, 256);
    SetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_NAME), buf);

    // 分类
    HWND hComboCat = GetDlgItem(m_hDlg, IDA_COMBO_CATEGORY);
    for (size_t i = 0; i < m_categories.size(); i++) {
        if (m_categories[i].id == m_asset.categoryId) {
            ComboBox_SetCurSel(hComboCat, (int)i);
            break;
        }
    }

    // 使用人
    HWND hUser = GetDlgItem(m_hDlg, IDA_EDIT_USER);
    MultiByteToWideChar(65001, 0, m_asset.userName.c_str(), -1, buf, 256);
    SetWindowTextW(hUser, buf);

    // 部门（根据员工ID查询部门）
    HWND hComboDept = GetDlgItem(m_hDlg, IDA_COMBO_DEPT);
    if (m_asset.userId > 0 && !m_asset.departmentName.empty()) {
        // 查找员工所属部门
        for (size_t i = 0; i < m_departments.size(); i++) {
            if (m_departments[i].name == m_asset.departmentName) {
                ComboBox_SetCurSel(hComboDept, (int)i + 1);  // +1 因为有空选项
                break;
            }
        }
    }

    // 购入日期
    if (!m_asset.purchaseDate.empty()) {
        MultiByteToWideChar(65001, 0, m_asset.purchaseDate.c_str(), -1, buf, 256);
        SetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_DATE), buf);
    }

    // 价格
    if (m_asset.price > 0) {
        swprintf_s(buf, L"%.2f", m_asset.price);
        SetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_PRICE), buf);
    }

    // 存放位置
    MultiByteToWideChar(65001, 0, m_asset.location.c_str(), -1, buf, 256);
    SetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_LOCATION), buf);

    // 状态
    HWND hComboStatus = GetDlgItem(m_hDlg, IDA_COMBO_STATUS);
    int statusIdx = 0;
    if (m_asset.status == "在用") statusIdx = 0;
    else if (m_asset.status == "闲置") statusIdx = 1;
    else if (m_asset.status == "维修中") statusIdx = 2;
    else if (m_asset.status == "已报废") statusIdx = 3;
    ComboBox_SetCurSel(hComboStatus, statusIdx);

    // 备注
    MultiByteToWideChar(65001, 0, m_asset.remark.c_str(), -1, buf, 256);
    SetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_REMARK), buf);
}

bool AssetEditDialog::SaveAsset() {
    if (!ValidateInput()) {
        return false;
    }

    wchar_t buf[256];
    char mbBuf[512];

    // 获取编号
    GetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_CODE), buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    m_asset.assetCode = mbBuf;

    // 获取名称
    GetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_NAME), buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    m_asset.name = mbBuf;

    // 获取分类
    HWND hComboCat = GetDlgItem(m_hDlg, IDA_COMBO_CATEGORY);
    int catIdx = ComboBox_GetCurSel(hComboCat);
    m_asset.categoryId = (catIdx >= 0 && catIdx < (int)m_categories.size())
        ? m_categories[catIdx].id : -1;

    // 获取使用人
    std::string userName = GetUserName();

    // 获取部门
    HWND hComboDept = GetDlgItem(m_hDlg, IDA_COMBO_DEPT);
    int deptIdx = ComboBox_GetCurSel(hComboDept);
    int deptId = -1;
    if (deptIdx > 0 && deptIdx - 1 < (int)m_departments.size()) {
        deptId = m_departments[deptIdx - 1].id;
    }

    // 查找员工ID
    m_asset.userId = FindEmployeeId(userName, deptId);

    // 获取购入日期
    GetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_DATE), buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    m_asset.purchaseDate = mbBuf;

    // 获取价格
    GetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_PRICE), buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    m_asset.price = atof(mbBuf);

    // 获取存放位置
    GetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_LOCATION), buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    m_asset.location = mbBuf;

    // 获取状态
    HWND hComboStatus = GetDlgItem(m_hDlg, IDA_COMBO_STATUS);
    int statusIdx = ComboBox_GetCurSel(hComboStatus);
    const char* statuses[] = {"在用", "闲置", "维修中", "已报废"};
    m_asset.status = (statusIdx >= 0 && statusIdx < 4) ? statuses[statusIdx] : "在用";

    // 自动调整状态：有使用人则为在用，否则为闲置
    if (m_asset.userId > 0 && m_asset.status == "闲置") {
        m_asset.status = "在用";
    } else if (m_asset.userId < 0 && m_asset.status == "在用") {
        m_asset.status = "闲置";
    }

    // 获取备注
    GetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_REMARK), buf, 256);
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    m_asset.remark = mbBuf;

    // 保存到数据库
    bool result;
    if (m_assetId < 0) {
        result = m_db.AddAsset(m_asset);
    } else {
        result = m_db.UpdateAsset(m_asset);
    }

    if (!result) {
        MessageBoxW(m_hDlg, L"保存失败！", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

bool AssetEditDialog::ValidateInput() {
    wchar_t buf[256];

    // 验证编号
    GetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_CODE), buf, 256);
    if (wcslen(buf) == 0) {
        MessageBoxW(m_hDlg, L"请输入资产编号", L"提示", MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hDlg, IDA_EDIT_CODE));
        return false;
    }

    // 验证名称
    GetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_NAME), buf, 256);
    if (wcslen(buf) == 0) {
        MessageBoxW(m_hDlg, L"请输入资产名称", L"提示", MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hDlg, IDA_EDIT_NAME));
        return false;
    }

    return true;
}

std::string AssetEditDialog::GetNextAssetCode() {
    Asset lastAsset;
    if (m_db.GetLastAsset(lastAsset)) {
        // 解析最后一个编号
        std::string code = lastAsset.assetCode;
        std::regex re(R"((\D*)(\d+))");
        std::smatch match;
        if (std::regex_match(code, match, re)) {
            std::string prefix = match[1].str();
            int number = std::stoi(match[2].str());
            number++;

            // 计算数字位数
            std::string numStr = match[2].str();
            int digits = (int)numStr.length();

            // 格式化新编号
            char fmt[32];
            sprintf_s(fmt, "%%s%%0%dd", digits);
            char newCode[64];
            sprintf_s(newCode, fmt, prefix.c_str(), number);
            return std::string(newCode);
        }
    }
    return "ZC001";
}

std::string AssetEditDialog::GetUserName() {
    wchar_t buf[256];
    GetWindowTextW(GetDlgItem(m_hDlg, IDA_EDIT_USER), buf, 256);
    char mbBuf[512];
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    return std::string(mbBuf);
}

int AssetEditDialog::FindEmployeeId(const std::string& name, int deptId) {
    if (name.empty()) {
        return -1;
    }

    // 根据姓名查找员工
    std::vector<Employee> employees = m_db.GetEmployeesByName(name);

    if (employees.empty()) {
        // 未找到员工
        wchar_t wmsg[512];
        wchar_t wname[256];
        MultiByteToWideChar(65001, 0, name.c_str(), -1, wname, 256);
        swprintf_s(wmsg, L"未找到姓名为 '%s' 的员工，请先在人员管理中添加该员工", wname);
        MessageBoxW(m_hDlg, wmsg, L"提示", MB_OK | MB_ICONWARNING);
        return -1;
    }

    if (employees.size() == 1) {
        // 唯一匹配
        return employees[0].id;
    }

    // 多个同名员工，使用部门筛选
    if (deptId >= 0) {
        for (const auto& emp : employees) {
            if (emp.departmentId == deptId) {
                return emp.id;
            }
        }
    }

    // 仍有歧义
    std::string msg = "存在多个姓名为 '" + name + "' 的员工：\n";
    for (const auto& emp : employees) {
        msg += "  - " + emp.name;
        // 获取部门名称
        for (const auto& dept : m_departments) {
            if (dept.id == emp.departmentId) {
                msg += " (" + dept.name + ")";
                break;
            }
        }
        msg += "\n";
    }
    msg += "\n请使用部门筛选来区分";

    wchar_t wmsg[1024];
    MultiByteToWideChar(65001, 0, msg.c_str(), -1, wmsg, 1024);
    MessageBoxW(m_hDlg, wmsg, L"提示", MB_OK | MB_ICONWARNING);
    return -1;
}
