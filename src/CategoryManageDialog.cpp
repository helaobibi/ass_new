/**
 * @file CategoryManageDialog.cpp
 * @brief 分类管理对话框实现
 */

#include "CategoryManageDialog.h"
#include <commctrl.h>
#include <windowsx.h>

CategoryManageDialog::CategoryManageDialog(Database& db)
    : m_db(db)
    , m_hDlg(nullptr)
    , m_hList(nullptr)
    , m_hEdit(nullptr)
    , m_selectedId(-1)
{
}

CategoryManageDialog::~CategoryManageDialog() {
}

void CategoryManageDialog::Show(HWND hParent) {
    DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_CATEGORY_MANAGE),
        hParent,
        DialogProc,
        (LPARAM)this
    );
}

INT_PTR CALLBACK CategoryManageDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    CategoryManageDialog* pThis = nullptr;

    if (uMsg == WM_INITDIALOG) {
        pThis = (CategoryManageDialog*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hDlg = hDlg;
    } else {
        pThis = (CategoryManageDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR CategoryManageDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            InitDialog();
            return TRUE;

        case WM_COMMAND: {
            WORD wmId = LOWORD(wParam);
            switch (wmId) {
                case IDC_BTN_ADD:
                    OnAdd();
                    return TRUE;

                case IDC_BTN_EDIT:
                    OnEdit();
                    return TRUE;

                case IDC_BTN_DELETE:
                    OnDelete();
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
            if (pnmhdr->idFrom == IDC_LIST_CATEGORIES) {
                if (pnmhdr->code == LVN_ITEMCHANGED) {
                    LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
                    if (pnmlv->uNewState & LVIS_SELECTED) {
                        int idx = pnmlv->iItem;
                        if (idx >= 0 && idx < (int)m_categories.size()) {
                            m_selectedId = m_categories[idx].id;
                            wchar_t buf[256];
                            MultiByteToWideChar(65001, 0, m_categories[idx].name.c_str(),
                                              -1, buf, 256);
                            SetWindowTextW(m_hEdit, buf);
                        }
                    }
                } else if (pnmhdr->code == NM_DBLCLK) {
                    OnEdit();
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

void CategoryManageDialog::InitDialog() {
    // 获取控件
    m_hList = GetDlgItem(m_hDlg, IDC_LIST_CATEGORIES);
    m_hEdit = GetDlgItem(m_hDlg, IDC_EDIT_CATEGORY);

    // 设置列表视图样式
    ListView_SetExtendedListViewStyle(
        m_hList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );

    // 添加列
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.pszText = (wchar_t*)L"ID";
    lvc.cx = 60;
    lvc.iSubItem = 0;
    ListView_InsertColumn(m_hList, 0, &lvc);

    lvc.pszText = (wchar_t*)L"分类名称";
    lvc.cx = 200;
    lvc.iSubItem = 1;
    ListView_InsertColumn(m_hList, 1, &lvc);

    LoadCategories();
}

void CategoryManageDialog::LoadCategories() {
    m_categories = m_db.GetAllCategories();
    RefreshList();
}

void CategoryManageDialog::RefreshList() {
    ListView_DeleteAllItems(m_hList);

    for (size_t i = 0; i < m_categories.size(); i++) {
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;

        wchar_t buf[256];
        _itow_s(m_categories[i].id, buf, 10);
        lvi.pszText = buf;
        ListView_InsertItem(m_hList, &lvi);

        MultiByteToWideChar(65001, 0, m_categories[i].name.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hList, i, 1, buf);
    }

    m_selectedId = -1;
    SetWindowText(m_hEdit, L"");
}

void CategoryManageDialog::OnAdd() {
    wchar_t buf[256];
    GetWindowTextW(m_hEdit, buf, 256);

    if (wcslen(buf) == 0) {
        MessageBoxW(m_hDlg, L"请输入分类名称", L"提示", MB_OK | MB_ICONWARNING);
        SetFocus(m_hEdit);
        return;
    }

    char mbBuf[512];
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string name = mbBuf;

    // 检查是否已存在
    Category existing;
    if (m_db.GetCategoryByName(name, existing)) {
        MessageBoxW(m_hDlg, L"该分类已存在", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    Category cat{0, name};
    if (m_db.AddCategory(cat)) {
        LoadCategories();
        SetWindowText(m_hEdit, L"");
    } else {
        MessageBoxW(m_hDlg, L"添加失败", L"错误", MB_OK | MB_ICONERROR);
    }
}

void CategoryManageDialog::OnEdit() {
    if (m_selectedId < 0) {
        MessageBoxW(m_hDlg, L"请先选择要编辑的分类", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    wchar_t buf[256];
    GetWindowTextW(m_hEdit, buf, 256);

    if (wcslen(buf) == 0) {
        MessageBoxW(m_hDlg, L"请输入分类名称", L"提示", MB_OK | MB_ICONWARNING);
        SetFocus(m_hEdit);
        return;
    }

    char mbBuf[512];
    WideCharToMultiByte(65001, 0, buf, -1, mbBuf, 512, nullptr, nullptr);
    std::string name = mbBuf;

    // 检查是否与其他分类重名
    Category existing;
    if (m_db.GetCategoryByName(name, existing) && existing.id != m_selectedId) {
        MessageBoxW(m_hDlg, L"该分类名称已存在", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    Category cat{m_selectedId, name};
    if (m_db.UpdateCategory(cat)) {
        LoadCategories();
        SetWindowText(m_hEdit, L"");
        m_selectedId = -1;
    } else {
        MessageBoxW(m_hDlg, L"更新失败", L"错误", MB_OK | MB_ICONERROR);
    }
}

void CategoryManageDialog::OnDelete() {
    if (m_selectedId < 0) {
        MessageBoxW(m_hDlg, L"请先选择要删除的分类", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    int result = MessageBoxW(m_hDlg, L"确定要删除选中的分类吗？", L"确认",
        MB_YESNO | MB_ICONQUESTION);

    if (result == IDYES) {
        if (m_db.DeleteCategory(m_selectedId)) {
            LoadCategories();
            SetWindowText(m_hEdit, L"");
            m_selectedId = -1;
        } else {
            MessageBoxW(m_hDlg, L"删除失败（可能有关联的资产）", L"错误",
                MB_OK | MB_ICONERROR);
        }
    }
}
