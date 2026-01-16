/**
 * @file ChangeLogDialog.cpp
 * @brief 变更日志查看对话框实现
 */

#include "ChangeLogDialog.h"
#include "resource_ids.h"
#include <windowsx.h>

// 列表视图列索引
enum {
    COL_TIME = 0,
    COL_ASSET_CODE,
    COL_ASSET_NAME,
    COL_FIELD,
    COL_OLD_VALUE,
    COL_NEW_VALUE
};

ChangeLogDialog::ChangeLogDialog(Database& db)
    : m_db(db)
    , m_hDlg(nullptr)
    , m_hList(nullptr)
    , m_hSearchEdit(nullptr)
    , m_filterAssetId(-1)
{
}

ChangeLogDialog::~ChangeLogDialog() {
}

void ChangeLogDialog::Show(HWND hParent) {
    m_filterAssetId = -1;
    DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_CHANGELOG_VIEW),
        hParent,
        DialogProc,
        (LPARAM)this
    );
}

void ChangeLogDialog::ShowForAsset(HWND hParent, int assetId) {
    m_filterAssetId = assetId;
    DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_CHANGELOG_VIEW),
        hParent,
        DialogProc,
        (LPARAM)this
    );
}

INT_PTR CALLBACK ChangeLogDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ChangeLogDialog* pThis = nullptr;

    if (uMsg == WM_INITDIALOG) {
        pThis = (ChangeLogDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hDlg = hDlg;
    } else {
        pThis = (ChangeLogDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR ChangeLogDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            InitDialog();
            LoadChangeLogs();
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(m_hDlg, LOWORD(wParam));
                    return TRUE;

                case IDL_BTN_SEARCH:
                    OnSearch();
                    return TRUE;

                case IDL_BTN_CLEAR:
                    OnClearSearch();
                    return TRUE;
            }
            break;

        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            // 调整列表视图大小
            if (m_hList) {
                SetWindowPos(m_hList, nullptr, 10, 50, width - 20, height - 100, SWP_NOZORDER);
            }
            return TRUE;
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 800;
            mmi->ptMinTrackSize.y = 400;
            return TRUE;
        }
    }

    return FALSE;
}

void ChangeLogDialog::InitDialog() {
    // 设置窗口标题
    if (m_filterAssetId >= 0) {
        SetWindowTextW(m_hDlg, L"资产变更日志");
    } else {
        SetWindowTextW(m_hDlg, L"所有变更日志");
    }

    // 获取控件句柄
    m_hList = GetDlgItem(m_hDlg, IDL_LIST_CHANGELOG);
    m_hSearchEdit = GetDlgItem(m_hDlg, IDL_EDIT_SEARCH);

    // 设置列表视图扩展样式
    ListView_SetExtendedListViewStyle(m_hList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    // 添加列
    LVCOLUMNW lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = COL_TIME;
    lvc.pszText = (LPWSTR)L"变更时间";
    lvc.cx = 150;
    ListView_InsertColumn(m_hList, COL_TIME, &lvc);

    lvc.iSubItem = COL_ASSET_CODE;
    lvc.pszText = (LPWSTR)L"资产编号";
    lvc.cx = 100;
    ListView_InsertColumn(m_hList, COL_ASSET_CODE, &lvc);

    lvc.iSubItem = COL_ASSET_NAME;
    lvc.pszText = (LPWSTR)L"资产名称";
    lvc.cx = 150;
    ListView_InsertColumn(m_hList, COL_ASSET_NAME, &lvc);

    lvc.iSubItem = COL_FIELD;
    lvc.pszText = (LPWSTR)L"变更字段";
    lvc.cx = 100;
    ListView_InsertColumn(m_hList, COL_FIELD, &lvc);

    lvc.iSubItem = COL_OLD_VALUE;
    lvc.pszText = (LPWSTR)L"原值";
    lvc.cx = 150;
    ListView_InsertColumn(m_hList, COL_OLD_VALUE, &lvc);

    lvc.iSubItem = COL_NEW_VALUE;
    lvc.pszText = (LPWSTR)L"新值";
    lvc.cx = 150;
    ListView_InsertColumn(m_hList, COL_NEW_VALUE, &lvc);
}

void ChangeLogDialog::LoadChangeLogs() {
    if (m_filterAssetId >= 0) {
        m_logs = m_db.GetChangeLogsByAssetId(m_filterAssetId);
    } else {
        m_logs = m_db.GetAllChangeLogs();
    }
    RefreshListView();
}

void ChangeLogDialog::RefreshListView() {
    ListView_DeleteAllItems(m_hList);

    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;

    for (size_t i = 0; i < m_logs.size(); i++) {
        const auto& log = m_logs[i];

        // 变更时间
        lvi.iItem = (int)i;
        lvi.iSubItem = COL_TIME;
        std::wstring wTime(log.changeTime.begin(), log.changeTime.end());
        lvi.pszText = (LPWSTR)wTime.c_str();
        ListView_InsertItem(m_hList, &lvi);

        // 资产编号
        std::wstring wCode(log.assetCode.begin(), log.assetCode.end());
        ListView_SetItemText(m_hList, (int)i, COL_ASSET_CODE, (LPWSTR)wCode.c_str());

        // 资产名称
        int len = MultiByteToWideChar(CP_UTF8, 0, log.assetName.c_str(), -1, nullptr, 0);
        std::wstring wName(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, log.assetName.c_str(), -1, &wName[0], len);
        ListView_SetItemText(m_hList, (int)i, COL_ASSET_NAME, (LPWSTR)wName.c_str());

        // 变更字段
        len = MultiByteToWideChar(CP_UTF8, 0, log.fieldName.c_str(), -1, nullptr, 0);
        std::wstring wField(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, log.fieldName.c_str(), -1, &wField[0], len);
        ListView_SetItemText(m_hList, (int)i, COL_FIELD, (LPWSTR)wField.c_str());

        // 原值
        len = MultiByteToWideChar(CP_UTF8, 0, log.oldValue.c_str(), -1, nullptr, 0);
        std::wstring wOldVal(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, log.oldValue.c_str(), -1, &wOldVal[0], len);
        if (wOldVal.empty() || (wOldVal.size() == 1 && wOldVal[0] == 0)) {
            wOldVal = L"(空)";
        }
        ListView_SetItemText(m_hList, (int)i, COL_OLD_VALUE, (LPWSTR)wOldVal.c_str());

        // 新值
        len = MultiByteToWideChar(CP_UTF8, 0, log.newValue.c_str(), -1, nullptr, 0);
        std::wstring wNewVal(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, log.newValue.c_str(), -1, &wNewVal[0], len);
        if (wNewVal.empty() || (wNewVal.size() == 1 && wNewVal[0] == 0)) {
            wNewVal = L"(空)";
        }
        ListView_SetItemText(m_hList, (int)i, COL_NEW_VALUE, (LPWSTR)wNewVal.c_str());
    }

    // 更新状态栏或标题显示记录数
    wchar_t title[256];
    if (m_filterAssetId >= 0) {
        swprintf_s(title, L"资产变更日志 - 共 %zu 条记录", m_logs.size());
    } else {
        swprintf_s(title, L"所有变更日志 - 共 %zu 条记录", m_logs.size());
    }
    SetWindowTextW(m_hDlg, title);
}

void ChangeLogDialog::OnSearch() {
    wchar_t wbuf[256] = {0};
    GetWindowTextW(m_hSearchEdit, wbuf, 256);

    // 转换为 UTF-8
    char searchBuf[256] = {0};
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, searchBuf, 256, nullptr, nullptr);
    std::string searchText = searchBuf;

    if (searchText.empty()) {
        LoadChangeLogs();
    } else {
        m_logs = m_db.SearchChangeLogs(searchText);
        RefreshListView();
    }
}

void ChangeLogDialog::OnClearSearch() {
    SetWindowTextW(m_hSearchEdit, L"");
    LoadChangeLogs();
}
