/**
 * @file CSVHelper.cpp
 * @brief CSV 导入导出辅助类实现
 */

#include "CSVHelper.h"
#include "Logger.h"
#include <commdlg.h>
#include <fstream>
#include <sstream>
#include <iomanip>

bool CSVHelper::ShowSaveDialog(HWND hWnd, std::wstring& filePath) {
    OPENFILENAMEW ofn = {0};
    wchar_t szFile[MAX_PATH] = {0};

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"csv";

    // 生成默认文件名（使用英文避免编码问题）
    SYSTEMTIME st;
    GetLocalTime(&st);
    swprintf_s(szFile, L"assets_%04d%02d%02d.csv", st.wYear, st.wMonth, st.wDay);

    if (GetSaveFileNameW(&ofn)) {
        filePath = szFile;
        return true;
    }
    return false;
}

bool CSVHelper::ShowOpenDialog(HWND hWnd, std::wstring& filePath) {
    OPENFILENAMEW ofn = {0};
    wchar_t szFile[MAX_PATH] = {0};

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        filePath = szFile;
        return true;
    }
    return false;
}

std::string CSVHelper::EscapeCSVField(const std::string& field) {
    // 如果包含逗号、引号或换行，需要用引号包裹并转义
    bool needQuote = (field.find(',') != std::string::npos) ||
                     (field.find('"') != std::string::npos) ||
                     (field.find('\n') != std::string::npos) ||
                     (field.find('\r') != std::string::npos);

    if (!needQuote) {
        return field;
    }

    std::string result = "\"";
    for (char c : field) {
        if (c == '"') {
            result += "\"\"";  // 转义引号
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

std::vector<std::string> CSVHelper::ParseCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::string field;
    bool inQuotes = false;

    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];

        if (c == '"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                // 转义的引号
                field += '"';
                i++;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            result.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    result.push_back(field);

    return result;
}

bool CSVHelper::ExportToCSV(HWND hWnd, Database& db) {
    std::wstring filePath;
    if (!ShowSaveDialog(hWnd, filePath)) {
        return false;
    }

    // 使用宽字符路径打开文件
    std::ofstream file(filePath.c_str(), std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        MessageBoxW(hWnd, L"无法创建文件", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    // 写入 UTF-8 BOM
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    file.write((const char*)bom, 3);

    // 写入表头
    file << "资产编号,资产名称,分类,使用人,部门,购入日期,金额,存放位置,状态,备注\n";

    // 获取所有资产
    std::vector<Asset> assets = db.GetAllAssets();

    for (const auto& asset : assets) {
        file << EscapeCSVField(asset.assetCode) << ","
             << EscapeCSVField(asset.name) << ","
             << EscapeCSVField(asset.categoryName) << ","
             << EscapeCSVField(asset.userName) << ","
             << EscapeCSVField(asset.departmentName) << ","
             << EscapeCSVField(asset.purchaseDate) << ","
             << EscapeCSVField(std::to_string(asset.price)) << ","
             << EscapeCSVField(asset.location) << ","
             << EscapeCSVField(asset.status) << ","
             << EscapeCSVField(asset.remark) << "\n";
    }

    file.close();

    wchar_t msg[512];
    swprintf_s(msg, L"已导出 %d 条记录到:\n%s", (int)assets.size(), filePath.c_str());
    MessageBoxW(hWnd, msg, L"成功", MB_OK | MB_ICONINFORMATION);

    return true;
}

bool CSVHelper::ImportFromCSV(HWND hWnd, Database& db) {
    LOG_INFO("========== 开始CSV导入 ==========");

    std::wstring filePath;
    if (!ShowOpenDialog(hWnd, filePath)) {
        LOG_INFO("用户取消了文件选择");
        return false;
    }

    // 转换文件路径为UTF-8用于日志
    char pathBuf[1024];
    WideCharToMultiByte(65001, 0, filePath.c_str(), -1, pathBuf, 1024, nullptr, nullptr);
    LOG_INFO("选择的文件: " + std::string(pathBuf));

    // 使用宽字符路径打开文件
    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("无法打开文件: " + std::string(pathBuf));
        MessageBoxW(hWnd, L"无法打开文件", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }
    LOG_INFO("文件打开成功");

    // 跳过 UTF-8 BOM
    unsigned char bom[3] = {0};
    file.read((char*)bom, 3);
    bool isUtf8 = (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF);
    if (!isUtf8) {
        file.seekg(0);
        LOG_INFO("文件编码: GBK");
    } else {
        LOG_INFO("文件编码: UTF-8");
    }

    // 开始事务
    LOG_INFO("开始数据库事务");
    db.BeginTransaction();

    int successCount = 0;
    int skipCount = 0;
    std::vector<std::string> errors;

    // 缓存数据
    LOG_INFO("加载缓存数据...");
    std::vector<Category> categories = db.GetAllCategories();
    std::vector<Department> departments = db.GetAllDepartments();
    std::vector<Employee> employees = db.GetAllEmployees();
    LOG_INFO("缓存数据加载完成 - 分类: " + std::to_string(categories.size()) +
             ", 部门: " + std::to_string(departments.size()) +
             ", 员工: " + std::to_string(employees.size()));

    std::string line;
    bool isFirstLine = true;
    int lineNumber = 0;

    LOG_INFO("开始逐行解析CSV文件...");
    while (std::getline(file, line)) {
        lineNumber++;

        // 跳过空行
        if (line.empty()) {
            LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 空行，跳过");
            continue;
        }

        // 如果不是UTF-8文件,需要从GBK转换为UTF-8
        if (!isUtf8 && !line.empty()) {
            // GBK -> UTF-16
            int wlen = MultiByteToWideChar(936, 0, line.c_str(), -1, nullptr, 0);
            if (wlen > 0) {
                std::wstring wstr(wlen, 0);
                MultiByteToWideChar(936, 0, line.c_str(), -1, &wstr[0], wlen);

                // UTF-16 -> UTF-8
                int utf8len = WideCharToMultiByte(65001, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (utf8len > 0) {
                    std::string utf8str(utf8len, 0);
                    WideCharToMultiByte(65001, 0, wstr.c_str(), -1, &utf8str[0], utf8len, nullptr, nullptr);
                    // 移除字符串末尾的null字符
                    if (!utf8str.empty() && utf8str.back() == '\0') {
                        utf8str.pop_back();
                    }
                    line = utf8str;
                }
            }
        }

        // 跳过表头
        if (isFirstLine) {
            isFirstLine = false;
            if (line.find("资产编号") != std::string::npos) {
                LOG_INFO("第" + std::to_string(lineNumber) + "行: 表头行，跳过");
                continue;
            }
        }

        // 解析行
        std::vector<std::string> fields = ParseCSVLine(line);
        if (fields.size() < 2) {
            LOG_WARNING("第" + std::to_string(lineNumber) + "行: 字段不足（需要至少2个字段），跳过");
            continue;
        }

        LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 开始处理，字段数=" + std::to_string(fields.size()));

        try {
            std::string assetCode = fields[0];
            std::string assetName = fields[1];

            // 验证必填项
            if (assetCode.empty() || assetName.empty()) {
                LOG_WARNING("第" + std::to_string(lineNumber) + "行: 资产编号或名称为空，跳过");
                continue;
            }

            // 去除首尾空格
            auto trim = [](std::string s) {
                size_t start = s.find_first_not_of(" \t\r\n");
                if (start == std::string::npos) return std::string();
                size_t end = s.find_last_not_of(" \t\r\n");
                return s.substr(start, end - start + 1);
            };

            assetCode = trim(assetCode);
            assetName = trim(assetName);

            LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 资产编号=" + assetCode + ", 名称=" + assetName);

            // 检查是否已存在
            Asset existing;
            if (db.GetAssetByCode(assetCode, existing)) {
                skipCount++;
                std::string msg = "资产编号 " + assetCode + " 已存在，跳过";
                errors.push_back(msg);
                LOG_WARNING("第" + std::to_string(lineNumber) + "行: " + msg);
                continue;
            }
            LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 资产编号不存在，可以添加");

            // 处理分类
            int categoryId = -1;
            if (fields.size() > 2 && !fields[2].empty()) {
                std::string catName = trim(fields[2]);
                LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 分类名称=" + catName);

                // 查找分类
                for (const auto& cat : categories) {
                    if (cat.name == catName) {
                        categoryId = cat.id;
                        LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 找到已存在的分类，ID=" + std::to_string(categoryId));
                        break;
                    }
                }
                // 不存在则创建
                if (categoryId < 0) {
                    LOG_INFO("第" + std::to_string(lineNumber) + "行: 分类不存在，创建新分类: " + catName);
                    Category newCat{0, catName};
                    if (db.AddCategory(newCat)) {
                        categoryId = newCat.id;
                        categories.push_back(newCat);
                        LOG_INFO("第" + std::to_string(lineNumber) + "行: 分类创建成功，ID=" + std::to_string(categoryId));
                    } else {
                        LOG_ERROR("第" + std::to_string(lineNumber) + "行: 分类创建失败: " + catName);
                    }
                }
            } else {
                LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 无分类信息");
            }

            // 处理部门和员工
            int userId = -1;
            if (fields.size() > 3 && !fields[3].empty()) {
                std::string userName = trim(fields[3]);
                std::string deptName;
                if (fields.size() > 4) {
                    deptName = trim(fields[4]);
                }
                LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 使用人=" + userName + ", 部门=" + (deptName.empty() ? "(空)" : deptName));

                // 查找或创建部门
                int deptId = -1;
                if (!deptName.empty()) {
                    for (const auto& dept : departments) {
                        if (dept.name == deptName) {
                            deptId = dept.id;
                            LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 找到已存在的部门，ID=" + std::to_string(deptId));
                            break;
                        }
                    }
                    if (deptId < 0) {
                        LOG_INFO("第" + std::to_string(lineNumber) + "行: 部门不存在，创建新部门: " + deptName);
                        Department newDept{0, deptName};
                        if (db.AddDepartment(newDept)) {
                            deptId = newDept.id;
                            departments.push_back(newDept);
                            LOG_INFO("第" + std::to_string(lineNumber) + "行: 部门创建成功，ID=" + std::to_string(deptId));
                        } else {
                            LOG_ERROR("第" + std::to_string(lineNumber) + "行: 部门创建失败: " + deptName);
                        }
                    }
                }

                // 查找员工
                for (const auto& emp : employees) {
                    if (emp.name == userName &&
                        (deptId < 0 || emp.departmentId == deptId)) {
                        userId = emp.id;
                        LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 找到已存在的员工，ID=" + std::to_string(userId));
                        break;
                    }
                }

                // 不存在则创建
                if (userId < 0) {
                    LOG_INFO("第" + std::to_string(lineNumber) + "行: 员工不存在，创建新员工: " + userName);
                    Employee newEmp{0, userName, deptId, "", "", "", ""};
                    if (db.AddEmployee(newEmp)) {
                        userId = newEmp.id;
                        employees.push_back(newEmp);
                        LOG_INFO("第" + std::to_string(lineNumber) + "行: 员工创建成功，ID=" + std::to_string(userId));
                    } else {
                        LOG_ERROR("第" + std::to_string(lineNumber) + "行: 员工创建失败: " + userName);
                    }
                }
            } else {
                LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 无使用人信息");
            }

            // 其他字段
            std::string purchaseDate = (fields.size() > 5) ? trim(fields[5]) : "";
            double price = 0.0;
            if (fields.size() > 6 && !fields[6].empty()) {
                price = atof(trim(fields[6]).c_str());
            }
            std::string location = (fields.size() > 7) ? trim(fields[7]) : "";
            std::string status = (fields.size() > 8 && !trim(fields[8]).empty())
                                ? trim(fields[8]) : "在用";
            std::string remark = (fields.size() > 9) ? trim(fields[9]) : "";

            LOG_DEBUG("第" + std::to_string(lineNumber) + "行: 购入日期=" + purchaseDate +
                     ", 价格=" + std::to_string(price) +
                     ", 位置=" + location +
                     ", 状态=" + status);

            // 创建资产
            Asset asset{0};
            asset.assetCode = assetCode;
            asset.name = assetName;
            asset.categoryId = categoryId;
            asset.userId = userId;
            asset.purchaseDate = purchaseDate;
            asset.price = price;
            asset.location = location;
            asset.status = status;
            asset.remark = remark;

            LOG_INFO("第" + std::to_string(lineNumber) + "行: 准备添加资产 - 编号=" + assetCode +
                    ", 名称=" + assetName +
                    ", 分类ID=" + std::to_string(categoryId) +
                    ", 使用人ID=" + std::to_string(userId));

            if (db.AddAsset(asset)) {
                successCount++;
                LOG_INFO("第" + std::to_string(lineNumber) + "行: ✓ 资产添加成功，新ID=" + std::to_string(asset.id));
            } else {
                std::string errMsg = "添加资产 " + assetCode + " 失败";
                std::string dbErr = db.GetLastError();
                if (!dbErr.empty()) {
                    errMsg += " (数据库错误: " + dbErr + ")";
                }
                errors.push_back(errMsg);
                LOG_ERROR("第" + std::to_string(lineNumber) + "行: ✗ " + errMsg);
            }

        } catch (const std::exception& e) {
            std::string errMsg = std::string("解析行失败: ") + e.what();
            errors.push_back(errMsg);
            LOG_ERROR("第" + std::to_string(lineNumber) + "行: 异常 - " + errMsg);
        }
    }

    file.close();
    LOG_INFO("CSV文件解析完成，共处理 " + std::to_string(lineNumber) + " 行");

    // 提交事务
    LOG_INFO("提交数据库事务...");
    db.Commit();
    LOG_INFO("事务提交成功");

    LOG_INFO("========== CSV导入完成 ==========");
    LOG_INFO("成功导入: " + std::to_string(successCount) + " 条");
    LOG_INFO("跳过: " + std::to_string(skipCount) + " 条");
    LOG_INFO("错误: " + std::to_string(errors.size()) + " 条");

    // 显示结果
    wchar_t resultMsg[1024];
    swprintf_s(resultMsg, L"导入完成！\n\n成功导入：%d 条\n跳过：%d 条",
               successCount, skipCount);

    if (!errors.empty() && errors.size() <= 10) {
        wcscat_s(resultMsg, L"\n\n详细信息：\n");
        for (const auto& err : errors) {
            std::wstring werr;
            int len = MultiByteToWideChar(65001, 0, err.c_str(), -1, nullptr, 0);
            werr.resize(len);
            MultiByteToWideChar(65001, 0, err.c_str(), -1, &werr[0], len);
            wcscat_s(resultMsg, L"\n");
            wcscat_s(resultMsg, werr.c_str());
        }
    } else if (errors.size() > 10) {
        swprintf_s(resultMsg + wcslen(resultMsg), 200,
                   L"\n\n还有 %d 条信息", (int)(errors.size() - 10));
    }

    MessageBoxW(hWnd, resultMsg, L"导入结果", MB_OK | MB_ICONINFORMATION);

    return true;
}

bool CSVHelper::DownloadTemplate(HWND hWnd) {
    std::wstring filePath;
    if (!ShowSaveDialog(hWnd, filePath)) {
        return false;
    }

    // 使用宽字符路径打开文件
    std::ofstream file(filePath.c_str(), std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        MessageBoxW(hWnd, L"无法创建文件", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }

    // 写入 UTF-8 BOM
    file.write("\xEF\xBB\xBF", 3);

    // 写入表头
    file << "资产编号,资产名称,分类,使用人,部门,购入日期,金额,存放位置,状态,备注\n";

    // 写入示例数据和说明
    file << "ZC001,联想笔记本电脑,电脑设备,张三,技术部,2024-01-15,6999,3楼研发部,在用,\n";
    file << "ZC002,办公桌,办公家具,李四,行政部,2024-01-20,1200,2楼办公室,在用,\n";
    file << "ZC003,打印机,电子设备,,,2024-02-01,3500,1楼前台,闲置,待分配\n";
    file << "\n";
    file << "# 填写说明：\n";
    file << "# 1. 资产编号和资产名称为必填项\n";
    file << "# 2. 分类、部门、使用人如果不存在会自动创建\n";
    file << "# 3. 状态可选：在用、闲置、维修中、已报废\n";
    file << "# 4. 日期格式：YYYY-MM-DD\n";
    file << "# 5. 金额为数字，不要包含货币符号\n";
    file << "# 6. 以 # 开头的行为注释，导入时会被忽略\n";

    file.close();

    MessageBoxW(hWnd, L"模板文件已生成！", L"成功", MB_OK | MB_ICONINFORMATION);
    return true;
}
