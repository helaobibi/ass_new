/**
 * @file models.h
 * @brief 固定资产管理系统 - 数据模型定义
 *
 * 数据模型：
 * - Asset: 固定资产
 * - Category: 资产分类
 * - Department: 部门
 * - Employee: 员工
 */

#ifndef MODELS_H
#define MODELS_H

#include <windows.h>
#include <string>

/**
 * @brief 资产分类
 */
struct Category {
    int id;
    std::string name;
};

/**
 * @brief 部门
 */
struct Department {
    int id;
    std::string name;
};

/**
 * @brief 员工
 */
struct Employee {
    int id;
    std::string name;
    int departmentId;       // 可空，-1 表示无部门
    std::string position;
    std::string phone;
    std::string email;
    std::string remark;
    FILETIME createdAt;
    FILETIME updatedAt;
};

/**
 * @brief 固定资产
 */
struct Asset {
    int id;
    std::string assetCode;
    std::string name;
    int categoryId;         // 可空，-1 表示无分类
    int userId;            // 可空，-1 表示无使用人
    std::string purchaseDate;
    double price;
    std::string location;
    std::string status;     // 在用、闲置、维修中、已报废
    std::string remark;
    FILETIME createdAt;
    FILETIME updatedAt;

    // 关联数据显示用
    std::string categoryName;
    std::string userName;
    std::string departmentName;
};

/**
 * @brief 资产状态枚举
 */
enum AssetStatus {
    STATUS_IN_USE = 0,      // 在用
    STATUS_IDLE,            // 闲置
    STATUS_MAINTAINING,     // 维修中
    STATUS_SCRAPPED         // 已报废
};

/**
 * @brief 获取状态字符串
 */
inline const char* GetStatusString(AssetStatus status) {
    switch (status) {
        case STATUS_IN_USE:      return "在用";
        case STATUS_IDLE:        return "闲置";
        case STATUS_MAINTAINING: return "维修中";
        case STATUS_SCRAPPED:    return "已报废";
        default:                 return "未知";
    }
}

/**
 * @brief 解析状态字符串
 */
inline AssetStatus ParseStatus(const std::string& statusStr) {
    if (statusStr == "在用")       return STATUS_IN_USE;
    if (statusStr == "闲置")       return STATUS_IDLE;
    if (statusStr == "维修中")     return STATUS_MAINTAINING;
    if (statusStr == "已报废")     return STATUS_SCRAPPED;
    return STATUS_IN_USE;  // 默认
}

#endif  // MODELS_H
