/**
 * @file main.cpp
 * @brief 固定资产管理系统 - 主程序入口
 */

#include "MainWindow.h"
#include <windows.h>
#include <commctrl.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    // 初始化通用控件
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icc);

    // 设置高 DPI 感知
    SetProcessDPIAware();

    // 创建并显示主窗口
    MainWindow mainWindow;
    if (!mainWindow.Create(hInstance)) {
        MessageBoxW(nullptr, L"窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 进入消息循环
    return mainWindow.MessageLoop();
}
