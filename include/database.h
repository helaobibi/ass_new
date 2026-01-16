/**
 * @file database.h
 * @brief 固定资产管理系统 - 数据库访问层 (SQLite C API)
 *
 * 提供：
 * - 数据库初始化和连接管理
 * - CRUD 操作
 * - 查询和统计功能
 */

#ifndef DATABASE_H
#define DATABASE_H

#include "models.h"
#include <vector>
#include <unordered_map>
#include <sqlite3.h>

/**
 * @brief 数据库管理类
 *
 * 使用 RAII 模式管理 SQLite 连接
 */
class Database {
public:
    Database();
    ~Database();

    // 禁止拷贝
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /**
     * @brief 初始化数据库
     * @return 成功返回 true
     */
    bool Initialize();

    /**
     * @brief 关闭数据库连接
     */
    void Close();

    /**
     * @brief 检查数据库是否已打开
     */
    bool IsOpen() const { return m_db != nullptr; }

    // ========== 分类操作 ==========

    /**
     * @brief 获取所有分类
     */
    std::vector<Category> GetAllCategories();

    /**
     * @brief 根据ID获取分类
     */
    bool GetCategoryById(int id, Category& category);

    /**
     * @brief 根据名称获取分类
     */
    bool GetCategoryByName(const std::string& name, Category& category);

    /**
     * @brief 添加分类
     */
    bool AddCategory(Category& category);

    /**
     * @brief 更新分类
     */
    bool UpdateCategory(const Category& category);

    /**
     * @brief 删除分类
     */
    bool DeleteCategory(int id);

    // ========== 部门操作 ==========

    /**
     * @brief 获取所有部门
     */
    std::vector<Department> GetAllDepartments();

    /**
     * @brief 根据ID获取部门
     */
    bool GetDepartmentById(int id, Department& dept);

    /**
     * @brief 根据名称获取部门
     */
    bool GetDepartmentByName(const std::string& name, Department& dept);

    /**
     * @brief 添加部门
     */
    bool AddDepartment(Department& dept);

    /**
     * @brief 更新部门
     */
    bool UpdateDepartment(const Department& dept);

    /**
     * @brief 删除部门
     */
    bool DeleteDepartment(int id);

    /**
     * @brief 获取部门下的员工数量
     */
    int GetDepartmentEmployeeCount(int deptId);

    // ========== 员工操作 ==========

    /**
     * @brief 获取所有员工
     */
    std::vector<Employee> GetAllEmployees();

    /**
     * @brief 根据ID获取员工
     */
    bool GetEmployeeById(int id, Employee& employee);

    /**
     * @brief 根据姓名获取员工（可能有多个同名）
     */
    std::vector<Employee> GetEmployeesByName(const std::string& name);

    /**
     * @brief 添加员工
     */
    bool AddEmployee(Employee& employee);

    /**
     * @brief 更新员工
     */
    bool UpdateEmployee(const Employee& employee);

    /**
     * @brief 删除员工
     */
    bool DeleteEmployee(int id);

    /**
     * @brief 获取员工的资产数量
     */
    int GetEmployeeAssetCount(int employeeId);

    /**
     * @brief 批量获取所有员工的资产数量
     * @return 员工ID到资产数量的映射
     */
    std::unordered_map<int, int> GetAllEmployeeAssetCounts();

    /**
     * @brief 搜索员工
     * @param searchText 搜索关键词
     * @param departmentId 部门筛选（-1表示全部）
     */
    std::vector<Employee> SearchEmployees(const std::string& searchText, int departmentId = -1);

    // ========== 资产操作 ==========

    /**
     * @brief 获取所有资产（带关联数据）
     */
    std::vector<Asset> GetAllAssets();

    /**
     * @brief 搜索资产
     * @param searchText 搜索关键词
     * @param categoryId 分类筛选（-1表示全部）
     * @param status 状态筛选（空字符串表示全部）
     */
    std::vector<Asset> SearchAssets(const std::string& searchText,
                                    int categoryId = -1,
                                    const std::string& status = "");

    /**
     * @brief 根据ID获取资产
     */
    bool GetAssetById(int id, Asset& asset);

    /**
     * @brief 根据资产编号获取资产
     */
    bool GetAssetByCode(const std::string& code, Asset& asset);

    /**
     * @brief 获取最后一条资产记录（用于生成编号）
     */
    bool GetLastAsset(Asset& asset);

    /**
     * @brief 添加资产
     */
    bool AddAsset(Asset& asset);

    /**
     * @brief 更新资产
     */
    bool UpdateAsset(const Asset& asset);

    /**
     * @brief 删除资产
     */
    bool DeleteAsset(int id);

    /**
     * @brief 获取资产总数和总价值
     */
    bool GetAssetStats(int& count, double& totalPrice);

    // ========== 变更日志操作 ==========

    /**
     * @brief 添加变更日志
     */
    bool AddChangeLog(const AssetChangeLog& log);

    /**
     * @brief 批量添加变更日志
     */
    bool AddChangeLogs(const std::vector<AssetChangeLog>& logs);

    /**
     * @brief 获取指定资产的变更日志
     */
    std::vector<AssetChangeLog> GetChangeLogsByAssetId(int assetId);

    /**
     * @brief 获取所有变更日志（支持分页）
     * @param limit 每页数量，-1表示不限制
     * @param offset 偏移量
     */
    std::vector<AssetChangeLog> GetAllChangeLogs(int limit = -1, int offset = 0);

    /**
     * @brief 搜索变更日志
     * @param searchText 搜索关键词（资产编号、名称、字段名）
     * @param startDate 开始日期（空字符串表示不限）
     * @param endDate 结束日期（空字符串表示不限）
     */
    std::vector<AssetChangeLog> SearchChangeLogs(const std::string& searchText,
                                                  const std::string& startDate = "",
                                                  const std::string& endDate = "");

    /**
     * @brief 获取变更日志总数
     */
    int GetChangeLogCount();

    // ========== 辅助功能 ==========

    /**
     * @brief 获取最后一个错误信息
     */
    const std::string& GetLastError() const { return m_lastError; }

    /**
     * @brief 开始事务
     */
    bool BeginTransaction();

    /**
     * @brief 提交事务
     */
    bool Commit();

    /**
     * @brief 回滚事务
     */
    bool Rollback();

private:
    sqlite3* m_db;
    std::string m_lastError;

    /**
     * @brief 设置错误信息
     */
    void SetError(const std::string& error) {
        m_lastError = error;
    }

    /**
     * @brief 创建表结构
     */
    bool CreateTables();

    /**
     * @brief 初始化默认数据
     */
    bool InitializeDefaultData();
};

#endif  // DATABASE_H
