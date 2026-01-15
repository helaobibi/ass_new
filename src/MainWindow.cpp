/**
 * @file MainWindow.cpp
 * @brief 主窗口实现
 */

#include "MainWindow.h"
#include "AssetEditDialog.h"
#include "CategoryManageDialog.h"
#include "EmployeeManageDialog.h"
#include "CSVHelper.h"
#include <commdlg.h>
#include <windowsx.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

// 全局指针
MainWindow* g_mainWindow = nullptr;

// 窗口类名
static const wchar_t WC_MAINWINDOW[] = L"AssetManagerMainWindow";

MainWindow::MainWindow()
    : m_hInstance(nullptr)
    , m_hWnd(nullptr)
    , m_hListView(nullptr)
    , m_hSearchEdit(nullptr)
    , m_hCategoryCombo(nullptr)
    , m_hStatusCombo(nullptr)
    , m_hStatusBar(nullptr)
    , m_selectedAssetId(-1)
    , m_sortColumn(-1)
    , m_sortAscending(true)
    , m_sortState(0)
{
}

MainWindow::~MainWindow() {
}

bool MainWindow::RegisterWindowClass() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WC_MAINWINDOW;
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    return RegisterClassExW(&wc) != 0;
}

bool MainWindow::Create(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    g_mainWindow = this;

    if (!RegisterWindowClass()) {
        MessageBoxW(nullptr, L"窗口类注册失败！", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!m_db.Initialize()) {
        MessageBoxW(nullptr, L"数据库初始化失败！", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
    DWORD dwExStyle = WS_EX_APPWINDOW;

    m_hWnd = CreateWindowExW(
        dwExStyle,
        WC_MAINWINDOW,
        L"固定资产管理系统",
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1100, 650,
        nullptr,
        nullptr,
        m_hInstance,
        this
    );

    if (!m_hWnd) {
        MessageBoxW(nullptr, L"创建窗口失败！", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    CreateMenus();
    CreateToolBar();

    if (!CreateControls()) {
        MessageBoxW(nullptr, L"创建控件失败！", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    LoadData();

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    return true;
}

void MainWindow::CreateMenus() {
    HMENU hMenu = CreateMenu();

    // 文件菜单
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, IDM_IMPORT_CSV, L"导入 CSV...");
    AppendMenuW(hFileMenu, MF_STRING, IDM_EXPORT_CSV, L"导出 CSV...");
    AppendMenuW(hFileMenu, MF_STRING, IDM_DOWNLOAD_TEMPLATE, L"下载导入模板...");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"退出");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"文件(&F)");

    // 编辑菜单
    HMENU hEditMenu = CreatePopupMenu();
    AppendMenuW(hEditMenu, MF_STRING, IDM_ASSET_ADD, L"新增资产\tIns");
    AppendMenuW(hEditMenu, MF_STRING, IDM_ASSET_EDIT, L"编辑资产\tEnter");
    AppendMenuW(hEditMenu, MF_STRING, IDM_ASSET_DELETE, L"删除资产\tDel");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hEditMenu, L"编辑(&E)");

    // 管理菜单
    HMENU hManageMenu = CreatePopupMenu();
    AppendMenuW(hManageMenu, MF_STRING, IDM_CATEGORY_MANAGE, L"分类管理...");
    AppendMenuW(hManageMenu, MF_STRING, IDM_EMPLOYEE_MANAGE, L"人员管理...");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hManageMenu, L"管理(&M)");

    // 视图菜单
    HMENU hViewMenu = CreatePopupMenu();
    AppendMenuW(hViewMenu, MF_STRING, IDM_REFRESH, L"刷新\tF5");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"视图(&V)");

    // 帮助菜单
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, IDM_ABOUT, L"关于");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"帮助(&H)");

    SetMenu(m_hWnd, hMenu);
}

void MainWindow::CreateToolBar() {
    // 简单工具栏实现
}

bool MainWindow::CreateControls() {
    // 获取客户区大小
    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);

    int y = 10;

    // 搜索和筛选面板
    HWND hSearchLabel = CreateWindowExW(
        0, L"STATIC", L"搜索：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, y, 50, 20,
        m_hWnd, nullptr, m_hInstance, nullptr
    );

    m_hSearchEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        60, y, 200, 22,
        m_hWnd, (HMENU)(UINT_PTR)ID_EDIT_SEARCH, m_hInstance, nullptr
    );

    HWND hCatLabel = CreateWindowExW(
        0, L"STATIC", L"分类：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        270, y, 50, 20,
        m_hWnd, nullptr, m_hInstance, nullptr
    );

    m_hCategoryCombo = CreateWindowExW(
        0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        320, y, 120, 200,
        m_hWnd, (HMENU)(UINT_PTR)ID_COMBO_CATEGORY, m_hInstance, nullptr
    );

    HWND hStatusLabel = CreateWindowExW(
        0, L"STATIC", L"状态：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        450, y, 50, 20,
        m_hWnd, nullptr, m_hInstance, nullptr
    );

    m_hStatusCombo = CreateWindowExW(
        0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        500, y, 100, 200,
        m_hWnd, (HMENU)(UINT_PTR)ID_COMBO_STATUS, m_hInstance, nullptr
    );

    HWND hClearBtn = CreateWindowExW(
        0, L"BUTTON", L"清除筛选",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        610, y, 80, 24,
        m_hWnd, (HMENU)IDM_CLEAR_FILTERS, m_hInstance, nullptr
    );

    y += 35;

    // 列表视图
    m_hListView = CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
        10, y, rcClient.right - 20, rcClient.bottom - y - 30,
        m_hWnd, (HMENU)(UINT_PTR)ID_LISTVIEW_ASSETS, m_hInstance, nullptr
    );

    // 设置扩展样式
    ListView_SetExtendedListViewStyle(
        m_hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );

    InitListViewColumns();

    // 状态栏
    m_hStatusBar = CreateWindowExW(
        0, STATUSCLASSNAMEW, L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        m_hWnd, (HMENU)(UINT_PTR)ID_STATUSBAR, m_hInstance, nullptr
    );

    // 设置字体
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (hFont) {
        SendMessage(m_hSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(m_hCategoryCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(m_hStatusCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(m_hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    // 加载分类和状态下拉列表
    m_categories = m_db.GetAllCategories();

    ComboBox_AddString(m_hCategoryCombo, L"全部");
    for (const auto& cat : m_categories) {
        int len = MultiByteToWideChar(65001, 0, cat.name.c_str(), -1, nullptr, 0);
        wchar_t* wname = new wchar_t[len];
        MultiByteToWideChar(65001, 0, cat.name.c_str(), -1, wname, len);
        ComboBox_AddString(m_hCategoryCombo, wname);
        delete[] wname;
    }
    ComboBox_SetCurSel(m_hCategoryCombo, 0);

    const wchar_t* statuses[] = {L"全部", L"在用", L"闲置", L"维修中", L"已报废"};
    for (int i = 0; i < 5; i++) {
        ComboBox_AddString(m_hStatusCombo, statuses[i]);
    }
    ComboBox_SetCurSel(m_hStatusCombo, 0);

    return true;
}

void MainWindow::InitListViewColumns() {
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    const wchar_t* colNames[] = {
        L"ID", L"资产编号", L"资产名称", L"分类",
        L"使用人", L"购入日期", L"金额", L"存放位置",
        L"状态", L"备注"
    };
    int colWidths[] = {50, 100, 150, 80, 80, 90, 80, 100, 60, 200};

    for (int i = 0; i < 10; i++) {
        lvc.pszText = (wchar_t*)colNames[i];
        lvc.cx = colWidths[i];
        lvc.iSubItem = i;
        ListView_InsertColumn(m_hListView, i, &lvc);
    }
}

void MainWindow::LoadData() {
    std::string searchText;
    int categoryId = -1;
    std::string status;

    GetSearchConditions(searchText, categoryId, status);
    m_assets = m_db.SearchAssets(searchText, categoryId, status);
    RefreshListView();
    UpdateStatusBar();
}

void MainWindow::RefreshListView() {
    ListView_DeleteAllItems(m_hListView);

    for (size_t i = 0; i < m_assets.size(); i++) {
        const Asset& asset = m_assets[i];

        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;

        // ID
        wchar_t buf[256];
        _itow_s(asset.id, buf, 10);
        lvi.pszText = buf;
        ListView_InsertItem(m_hListView, &lvi);

        // 资产编号
        MultiByteToWideChar(65001, 0, asset.assetCode.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hListView, i, COL_CODE, buf);

        // 资产名称
        MultiByteToWideChar(65001, 0, asset.name.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hListView, i, COL_NAME, buf);

        // 分类
        MultiByteToWideChar(65001, 0, asset.categoryName.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hListView, i, COL_CATEGORY, buf);

        // 使用人
        MultiByteToWideChar(65001, 0, asset.userName.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hListView, i, COL_USER, buf);

        // 购入日期
        MultiByteToWideChar(65001, 0, asset.purchaseDate.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hListView, i, COL_PURCHASE_DATE, buf);

        // 金额
        swprintf_s(buf, L"%.2f", asset.price);
        ListView_SetItemText(m_hListView, i, COL_PRICE, buf);

        // 存放位置
        MultiByteToWideChar(65001, 0, asset.location.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hListView, i, COL_LOCATION, buf);

        // 状态
        MultiByteToWideChar(65001, 0, asset.status.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hListView, i, COL_STATUS, buf);

        // 备注
        MultiByteToWideChar(65001, 0, asset.remark.c_str(), -1, buf, 256);
        ListView_SetItemText(m_hListView, i, COL_REMARK, buf);
    }
}

void MainWindow::RefreshCategoryCombo() {
    // 保存当前选中的索引
    int currentSel = ComboBox_GetCurSel(m_hCategoryCombo);

    // 清空下拉菜单
    ComboBox_ResetContent(m_hCategoryCombo);

    // 重新加载分类列表
    m_categories = m_db.GetAllCategories();

    // 添加"全部"选项
    ComboBox_AddString(m_hCategoryCombo, L"全部");

    // 添加所有分类
    for (const auto& cat : m_categories) {
        int len = MultiByteToWideChar(65001, 0, cat.name.c_str(), -1, nullptr, 0);
        wchar_t* wname = new wchar_t[len];
        MultiByteToWideChar(65001, 0, cat.name.c_str(), -1, wname, len);
        ComboBox_AddString(m_hCategoryCombo, wname);
        delete[] wname;
    }

    // 恢复选择（如果索引仍然有效）
    int newCount = ComboBox_GetCount(m_hCategoryCombo);
    if (currentSel >= 0 && currentSel < newCount) {
        ComboBox_SetCurSel(m_hCategoryCombo, currentSel);
    } else {
        ComboBox_SetCurSel(m_hCategoryCombo, 0);  // 默认选择"全部"
    }
}

void MainWindow::UpdateStatusBar() {
    int count = 0;
    double total = 0.0;
    m_db.GetAssetStats(count, total);

    wchar_t buf[256];
    swprintf_s(buf, L"共 %d 条记录，总价值: ¥%.2f", count, total);
    SendMessage(m_hStatusBar, WM_SETTEXT, 0, (LPARAM)buf);
}

void MainWindow::GetSearchConditions(std::string& searchText, int& categoryId, std::string& status) {
    // 获取搜索文本
    wchar_t wbuf[256];
    GetWindowTextW(m_hSearchEdit, wbuf, 256);
    char searchBuf[256];
    WideCharToMultiByte(65001, 0, wbuf, -1, searchBuf, 256, nullptr, nullptr);
    searchText = searchBuf;

    // 获取分类
    int catIdx = ComboBox_GetCurSel(m_hCategoryCombo);
    if (catIdx > 0 && catIdx - 1 < (int)m_categories.size()) {
        categoryId = m_categories[catIdx - 1].id;
    } else {
        categoryId = -1;
    }

    // 获取状态
    int statusIdx = ComboBox_GetCurSel(m_hStatusCombo);
    const char* statuses[] = {"", "在用", "闲置", "维修中", "已报废"};
    if (statusIdx >= 0 && statusIdx < 5) {
        status = statuses[statusIdx];
    }
}

void MainWindow::OnAddAsset() {
    AssetEditDialog dialog(m_db);
    // 如果有选中的资产，复制其信息
    int copyFromId = (m_selectedAssetId >= 0) ? m_selectedAssetId : -1;
    if (dialog.ShowAdd(m_hWnd, copyFromId)) {
        LoadData();
    }
}

void MainWindow::OnEditAsset() {
    if (m_selectedAssetId < 0) {
        MessageBoxW(m_hWnd, L"请先选择要编辑的资产", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }
    AssetEditDialog dialog(m_db);
    if (dialog.ShowEdit(m_hWnd, m_selectedAssetId)) {
        LoadData();
    }
}

void MainWindow::OnDeleteAsset() {
    if (m_selectedAssetId < 0) {
        MessageBoxW(m_hWnd, L"请先选择要删除的资产", L"提示", MB_OK | MB_ICONWARNING);
        return;
    }

    int result = MessageBoxW(m_hWnd, L"确定要删除选中的资产吗？", L"确认",
        MB_YESNO | MB_ICONQUESTION);

    if (result == IDYES) {
        if (m_db.DeleteAsset(m_selectedAssetId)) {
            LoadData();
            MessageBoxW(m_hWnd, L"删除成功", L"成功", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(m_hWnd, L"删除失败", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::OnManageCategories() {
    CategoryManageDialog dialog(m_db);
    dialog.Show(m_hWnd);
    // 对话框关闭后刷新分类下拉菜单和数据
    RefreshCategoryCombo();
    LoadData();
}

void MainWindow::OnManageEmployees() {
    EmployeeManageDialog dialog(m_db);
    dialog.Show(m_hWnd);
    // 对话框关闭后刷新数据
    LoadData();
}

void MainWindow::OnImportCSV() {
    if (CSVHelper::ImportFromCSV(m_hWnd, m_db)) {
        LoadData();
    }
}

void MainWindow::OnExportCSV() {
    CSVHelper::ExportToCSV(m_hWnd, m_db);
}

void MainWindow::OnDownloadTemplate() {
    CSVHelper::DownloadTemplate(m_hWnd);
}

void MainWindow::OnClearFilters() {
    // 清除搜索文本
    SetWindowTextW(m_hSearchEdit, L"");

    // 重置分类下拉框
    ComboBox_SetCurSel(m_hCategoryCombo, 0);

    // 重置状态下拉框
    ComboBox_SetCurSel(m_hStatusCombo, 0);

    // 重置排序状态
    m_sortColumn = -1;
    m_sortAscending = true;
    m_sortState = 0;

    // 重置列头文本
    ResetColumnHeaders();

    // 重新加载数据
    LoadData();
}

int MainWindow::MessageLoop() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, nullptr, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        LPCREATESTRUCTW pCreate = (LPCREATESTRUCTW)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (MainWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            return 0;

        case WM_SIZE: {
            // 调整控件大小
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            if (m_hListView) {
                SetWindowPos(m_hListView, nullptr, 10, 45, width - 20, height - 75,
                    SWP_NOZORDER);
            }

            if (m_hStatusBar) {
                SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
            }

            // 调整状态栏各部分宽度
            int parts[2] = {width - 200, -1};
            SendMessage(m_hStatusBar, SB_SETPARTS, 2, (LPARAM)parts);

            return 0;
        }

        case WM_NOTIFY: {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            if (pnmhdr->hwndFrom == m_hListView) {
                if (pnmhdr->code == LVN_ITEMCHANGED) {
                    LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
                    if (pnmlv->uNewState & LVIS_SELECTED) {
                        int idx = pnmlv->iItem;
                        if (idx >= 0 && idx < (int)m_assets.size()) {
                            m_selectedAssetId = m_assets[idx].id;
                        }
                    }
                } else if (pnmhdr->code == NM_DBLCLK) {
                    OnEditAsset();
                } else if (pnmhdr->code == LVN_COLUMNCLICK) {
                    LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
                    OnColumnClick(pnmlv->iSubItem);
                }
            }
            break;
        }

        case WM_COMMAND: {
            WORD wmId = LOWORD(wParam);
            switch (wmId) {
                case IDM_FILE_EXIT:
                    PostMessage(m_hWnd, WM_CLOSE, 0, 0);
                    break;

                case IDM_ASSET_ADD:
                    OnAddAsset();
                    break;

                case IDM_ASSET_EDIT:
                    OnEditAsset();
                    break;

                case IDM_ASSET_DELETE:
                    OnDeleteAsset();
                    break;

                case IDM_CATEGORY_MANAGE:
                    OnManageCategories();
                    break;

                case IDM_EMPLOYEE_MANAGE:
                    OnManageEmployees();
                    break;

                case IDM_IMPORT_CSV:
                    OnImportCSV();
                    break;

                case IDM_EXPORT_CSV:
                    OnExportCSV();
                    break;

                case IDM_DOWNLOAD_TEMPLATE:
                    OnDownloadTemplate();
                    break;

                case IDM_REFRESH:
                    LoadData();
                    break;

                case IDM_ABOUT:
                    MessageBoxW(m_hWnd, L"固定资产管理系统 v1.0\n\n基于 Win32 API + SQLite",
                        L"关于", MB_OK | MB_ICONINFORMATION);
                    break;

                case ID_EDIT_SEARCH:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        LoadData();
                    }
                    break;

                case ID_COMBO_CATEGORY:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        LoadData();
                    }
                    break;

                case ID_COMBO_STATUS:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        LoadData();
                    }
                    break;

                case IDM_CLEAR_FILTERS:
                    OnClearFilters();
                    break;

                default:
                    return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(m_hWnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}

// 处理列排序 - 三态切换：升序→降序→默认
void MainWindow::OnColumnClick(int column) {
    // 如果点击的是不同的列，重置之前列的状态
    if (m_sortColumn != column && m_sortColumn >= 0) {
        UpdateColumnHeader(m_sortColumn, 0);  // 重置之前列的箭头
    }

    // 切换状态: 0(默认) -> 1(升序) -> 2(降序) -> 0(默认)
    if (m_sortColumn == column) {
        m_sortState = (m_sortState + 1) % 3;
    } else {
        m_sortColumn = column;
        m_sortState = 1;  // 新列从升序开始
    }

    // 更新列头显示
    UpdateColumnHeader(column, m_sortState);

    // 执行排序
    if (m_sortState == 0) {
        // 恢复默认排序（按ID降序）
        m_sortColumn = -1;
        m_sortAscending = true;
        LoadData();  // 重新加载以恢复默认排序
    } else {
        m_sortAscending = (m_sortState == 1);
        SortAssets();
        RefreshListView();
    }
}

// 排序资产列表
void MainWindow::SortAssets() {
    if (m_sortColumn < 0 || m_assets.empty()) {
        return;
    }

    std::sort(m_assets.begin(), m_assets.end(), [this](const Asset& a, const Asset& b) {
        bool result = false;

        switch (m_sortColumn) {
            case COL_ID:
                result = a.id < b.id;
                break;

            case COL_CODE:
                result = a.assetCode < b.assetCode;
                break;

            case COL_NAME:
                result = a.name < b.name;
                break;

            case COL_CATEGORY: {
                // 获取分类名称
                std::string catA, catB;
                for (const auto& cat : m_categories) {
                    if (cat.id == a.categoryId) catA = cat.name;
                    if (cat.id == b.categoryId) catB = cat.name;
                }
                result = catA < catB;
                break;
            }

            case COL_USER:
                result = a.userName < b.userName;
                break;

            case COL_PURCHASE_DATE:
                result = a.purchaseDate < b.purchaseDate;
                break;

            case COL_PRICE:
                result = a.price < b.price;
                break;

            case COL_LOCATION:
                result = a.location < b.location;
                break;

            case COL_STATUS:
                result = a.status < b.status;
                break;

            case COL_REMARK:
                result = a.remark < b.remark;
                break;

            default:
                result = false;
        }

        return m_sortAscending ? result : !result;
    });
}

// 重置所有列头文本
void MainWindow::ResetColumnHeaders() {
    const wchar_t* colNames[] = {
        L"ID", L"资产编号", L"资产名称", L"分类",
        L"使用人", L"购入日期", L"金额", L"存放位置",
        L"状态", L"备注"
    };

    for (int i = 0; i < 10; i++) {
        LVCOLUMNW lvc = {};
        lvc.mask = LVCF_TEXT;
        lvc.pszText = (wchar_t*)colNames[i];
        ListView_SetColumn(m_hListView, i, &lvc);
    }
}

// 更新列头文本（显示排序箭头）
void MainWindow::UpdateColumnHeader(int column, int sortState) {
    const wchar_t* colNames[] = {
        L"ID", L"资产编号", L"资产名称", L"分类",
        L"使用人", L"购入日期", L"金额", L"存放位置",
        L"状态", L"备注"
    };

    if (column < 0 || column >= 10) return;

    wchar_t headerText[64];
    switch (sortState) {
        case 1:  // 升序
            swprintf_s(headerText, L"%s ▲", colNames[column]);
            break;
        case 2:  // 降序
            swprintf_s(headerText, L"%s ▼", colNames[column]);
            break;
        default:  // 默认
            wcscpy_s(headerText, colNames[column]);
            break;
    }

    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT;
    lvc.pszText = headerText;
    ListView_SetColumn(m_hListView, column, &lvc);
}
