/**
 * @file CSVHelper.h
 * @brief CSV 导入导出辅助类
 */

#ifndef CSVHELPER_H
#define CSVHELPER_H

#include "models.h"
#include "database.h"
#include <windows.h>
#include <string>
#include <vector>

/**
 * @brief CSV 导入导出辅助类
 */
class CSVHelper {
public:
    /**
     * @brief 导出资产数据到 CSV 文件
     * @param hWnd 父窗口句柄
     * @param db 数据库引用
     * @return 成功返回 true
     */
    static bool ExportToCSV(HWND hWnd, Database& db);

    /**
     * @brief 从 CSV 文件导入资产数据
     * @param hWnd 父窗口句柄
     * @param db 数据库引用
     * @return 成功返回 true
     */
    static bool ImportFromCSV(HWND hWnd, Database& db);

    /**
     * @brief 下载 CSV 导入模板
     * @param hWnd 父窗口句柄
     * @return 成功返回 true
     */
    static bool DownloadTemplate(HWND hWnd);

private:
    /**
     * @brief 解析 CSV 行
     */
    static std::vector<std::string> ParseCSVLine(const std::string& line);

    /**
     * @brief 转义 CSV 字段
     */
    static std::string EscapeCSVField(const std::string& field);

    /**
     * @brief 显示文件选择对话框（保存）
     */
    static bool ShowSaveDialog(HWND hWnd, std::wstring& filePath);

    /**
     * @brief 显示文件选择对话框（打开）
     */
    static bool ShowOpenDialog(HWND hWnd, std::wstring& filePath);
};

#endif  // CSVHELPER_H
