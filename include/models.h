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

    // 关联数据显示用
    std::string categoryName;
    std::string userName;
    std::string departmentName;
};

/**
 * @brief 资产变更日志
 */
struct AssetChangeLog {
    int id;
    int assetId;            // 关联的资产ID
    std::string assetCode;  // 资产编号（冗余存储，便于查询）
    std::string assetName;  // 资产名称（冗余存储）
    std::string fieldName;  // 变更的字段名
    std::string oldValue;   // 旧值
    std::string newValue;   // 新值
    std::string changeTime; // 变更时间
};

#endif  // MODELS_H
