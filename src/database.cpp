/**
 * @file database.cpp
 * @brief 数据库访问层实现
 */

#include "database.h"
#include <sstream>
#include <iomanip>

// 辅助函数：从 sqlite3_stmt 构建 Asset 对象
static void BuildAssetFromStmt(sqlite3_stmt* stmt, Asset& asset) {
    asset.id = sqlite3_column_int(stmt, 0);
    asset.assetCode = (const char*)sqlite3_column_text(stmt, 1);
    asset.name = (const char*)sqlite3_column_text(stmt, 2);
    asset.categoryId = sqlite3_column_int(stmt, 3);
    asset.userId = sqlite3_column_int(stmt, 4);
    const char* purchaseDate = (const char*)sqlite3_column_text(stmt, 5);
    asset.purchaseDate = purchaseDate ? purchaseDate : "";
    asset.price = sqlite3_column_double(stmt, 6);
    const char* location = (const char*)sqlite3_column_text(stmt, 7);
    const char* status = (const char*)sqlite3_column_text(stmt, 8);
    const char* remark = (const char*)sqlite3_column_text(stmt, 9);
    const char* catName = (const char*)sqlite3_column_text(stmt, 10);
    const char* userName = (const char*)sqlite3_column_text(stmt, 11);
    const char* deptName = (const char*)sqlite3_column_text(stmt, 12);
    asset.location = location ? location : "";
    asset.status = status ? status : "在用";
    asset.remark = remark ? remark : "";
    asset.categoryName = catName ? catName : "";
    asset.userName = userName ? userName : "";
    asset.departmentName = deptName ? deptName : "";
}

Database::Database() : m_db(nullptr) {
}

Database::~Database() {
    Close();
}

bool Database::Initialize() {
    const char* dbPath = "assets.db";

    int rc = sqlite3_open(dbPath, &m_db);
    if (rc != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // 启用外键约束
    char* errMsg = nullptr;
    rc = sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_lastError = errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    // 性能优化
    sqlite3_exec(m_db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, &errMsg);
    sqlite3_exec(m_db, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, &errMsg);
    sqlite3_exec(m_db, "PRAGMA cache_size = 10000;", nullptr, nullptr, &errMsg);

    if (!CreateTables()) {
        return false;
    }

    if (!InitializeDefaultData()) {
        return false;
    }

    return true;
}

void Database::Close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool Database::CreateTables() {
    char* errMsg = nullptr;
    int rc;

    // 创建分类表
    const char* createCategoryTable = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        );
    )";

    rc = sqlite3_exec(m_db, createCategoryTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_lastError = errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    // 创建部门表
    const char* createDepartmentTable = R"(
        CREATE TABLE IF NOT EXISTS departments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            created_at INTEGER DEFAULT (strftime('%s', 'now'))
        );
    )";

    rc = sqlite3_exec(m_db, createDepartmentTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_lastError = errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    // 创建员工表
    const char* createEmployeeTable = R"(
        CREATE TABLE IF NOT EXISTS employees (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            department_id INTEGER,
            created_at INTEGER DEFAULT (strftime('%s', 'now')),
            updated_at INTEGER DEFAULT (strftime('%s', 'now')),
            FOREIGN KEY (department_id) REFERENCES departments(id) ON DELETE SET NULL
        );
    )";

    rc = sqlite3_exec(m_db, createEmployeeTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_lastError = errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    // 创建资产表
    const char* createAssetTable = R"(
        CREATE TABLE IF NOT EXISTS assets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            asset_code TEXT NOT NULL UNIQUE,
            name TEXT NOT NULL,
            category_id INTEGER,
            user_id INTEGER,
            purchase_date TEXT,
            price REAL DEFAULT 0,
            location TEXT,
            status TEXT DEFAULT '在用',
            remark TEXT,
            created_at INTEGER DEFAULT (strftime('%s', 'now')),
            updated_at INTEGER DEFAULT (strftime('%s', 'now')),
            FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE SET NULL,
            FOREIGN KEY (user_id) REFERENCES employees(id) ON DELETE SET NULL
        );
    )";

    rc = sqlite3_exec(m_db, createAssetTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_lastError = errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    // 创建索引
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_assets_category ON assets(category_id);", nullptr, nullptr, &errMsg);
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_assets_user ON assets(user_id);", nullptr, nullptr, &errMsg);
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_assets_status ON assets(status);", nullptr, nullptr, &errMsg);

    // 创建变更日志表
    const char* createChangeLogTable = R"(
        CREATE TABLE IF NOT EXISTS asset_change_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            asset_id INTEGER NOT NULL,
            asset_code TEXT NOT NULL,
            asset_name TEXT NOT NULL,
            field_name TEXT NOT NULL,
            old_value TEXT,
            new_value TEXT,
            change_time TEXT DEFAULT (datetime('now', 'localtime')),
            FOREIGN KEY (asset_id) REFERENCES assets(id) ON DELETE CASCADE
        );
    )";

    rc = sqlite3_exec(m_db, createChangeLogTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_lastError = errMsg;
        sqlite3_free(errMsg);
        return false;
    }

    // 创建变更日志索引
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_changelog_asset ON asset_change_logs(asset_id);", nullptr, nullptr, &errMsg);
    sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_changelog_time ON asset_change_logs(change_time);", nullptr, nullptr, &errMsg);

    return true;
}

bool Database::InitializeDefaultData() {
    // 不再创建默认分类和部门，由用户自行添加
    return true;
}

// ========== 分类操作实现 ==========

std::vector<Category> Database::GetAllCategories() {
    std::vector<Category> result;
    result.reserve(32);  // 预分配，减少内存重分配
    sqlite3_stmt* stmt;

    const char* sql = "SELECT id, name FROM categories ORDER BY name;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Category cat;
            cat.id = sqlite3_column_int(stmt, 0);
            cat.name = (const char*)sqlite3_column_text(stmt, 1);
            result.push_back(std::move(cat));
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

bool Database::GetCategoryById(int id, Category& category) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, name FROM categories WHERE id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            category.id = sqlite3_column_int(stmt, 0);
            category.name = (const char*)sqlite3_column_text(stmt, 1);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }

    return false;
}

bool Database::GetCategoryByName(const std::string& name, Category& category) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, name FROM categories WHERE name = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            category.id = sqlite3_column_int(stmt, 0);
            category.name = (const char*)sqlite3_column_text(stmt, 1);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }

    return false;
}

bool Database::AddCategory(Category& category) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO categories (name) VALUES (?);";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, category.name.c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE) {
            category.id = (int)sqlite3_last_insert_rowid(m_db);
        }
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::UpdateCategory(const Category& category) {
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE categories SET name = ? WHERE id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, category.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, category.id);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::DeleteCategory(int id) {
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM categories WHERE id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

// ========== 部门操作实现 ==========

std::vector<Department> Database::GetAllDepartments() {
    std::vector<Department> result;
    result.reserve(32);  // 预分配，减少内存重分配
    sqlite3_stmt* stmt;

    const char* sql = "SELECT id, name FROM departments ORDER BY name;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Department dept;
            dept.id = sqlite3_column_int(stmt, 0);
            dept.name = (const char*)sqlite3_column_text(stmt, 1);
            result.push_back(std::move(dept));
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

bool Database::GetDepartmentById(int id, Department& dept) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, name FROM departments WHERE id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            dept.id = sqlite3_column_int(stmt, 0);
            dept.name = (const char*)sqlite3_column_text(stmt, 1);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }

    return false;
}

bool Database::GetDepartmentByName(const std::string& name, Department& dept) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, name FROM departments WHERE name = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            dept.id = sqlite3_column_int(stmt, 0);
            dept.name = (const char*)sqlite3_column_text(stmt, 1);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }

    return false;
}

bool Database::AddDepartment(Department& dept) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO departments (name) VALUES (?);";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dept.name.c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE) {
            dept.id = (int)sqlite3_last_insert_rowid(m_db);
        }
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::UpdateDepartment(const Department& dept) {
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE departments SET name = ? WHERE id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, dept.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, dept.id);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

int Database::GetDepartmentEmployeeCount(int deptId) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM employees WHERE department_id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    int count = 0;
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, deptId);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return count;
}

bool Database::DeleteDepartment(int id) {
    // 检查部门是否有员工
    int empCount = GetDepartmentEmployeeCount(id);
    if (empCount > 0) {
        m_lastError = "该部门下还有员工，无法删除";
        return false;
    }

    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM departments WHERE id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

// ========== 员工操作实现 ==========

std::vector<Employee> Database::GetAllEmployees() {
    std::vector<Employee> result;
    result.reserve(64);  // 预分配，减少内存重分配
    sqlite3_stmt* stmt;

    const char* sql = R"(
        SELECT e.id, e.name, e.department_id
        FROM employees e
        LEFT JOIN departments d ON e.department_id = d.id
        ORDER BY e.name;
    )";

    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Employee emp;
            emp.id = sqlite3_column_int(stmt, 0);
            emp.name = (const char*)sqlite3_column_text(stmt, 1);
            emp.departmentId = sqlite3_column_int(stmt, 2);
            result.push_back(std::move(emp));
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

bool Database::GetEmployeeById(int id, Employee& emp) {
    sqlite3_stmt* stmt;
    const char* sql = R"(
        SELECT id, name, department_id
        FROM employees WHERE id = ?;
    )";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            emp.id = sqlite3_column_int(stmt, 0);
            emp.name = (const char*)sqlite3_column_text(stmt, 1);
            emp.departmentId = sqlite3_column_int(stmt, 2);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }

    return false;
}

std::vector<Employee> Database::GetEmployeesByName(const std::string& name) {
    std::vector<Employee> result;
    result.reserve(8);  // 同名员工通常较少
    sqlite3_stmt* stmt;
    const char* sql = R"(
        SELECT e.id, e.name, e.department_id, d.name as dept_name
        FROM employees e
        LEFT JOIN departments d ON e.department_id = d.id
        WHERE e.name = ?;
    )";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Employee emp;
            emp.id = sqlite3_column_int(stmt, 0);
            emp.name = (const char*)sqlite3_column_text(stmt, 1);
            emp.departmentId = sqlite3_column_int(stmt, 2);
            result.push_back(std::move(emp));
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

bool Database::AddEmployee(Employee& employee) {
    sqlite3_stmt* stmt;
    const char* sql = R"(
        INSERT INTO employees (name, department_id)
        VALUES (?, ?);
    )";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, employee.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, employee.departmentId);

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE) {
            employee.id = (int)sqlite3_last_insert_rowid(m_db);
        }
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::UpdateEmployee(const Employee& employee) {
    sqlite3_stmt* stmt;
    const char* sql = R"(
        UPDATE employees
        SET name = ?, department_id = ?,
            updated_at = strftime('%s', 'now')
        WHERE id = ?;
    )";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, employee.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, employee.departmentId);
        sqlite3_bind_int(stmt, 3, employee.id);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::DeleteEmployee(int id) {
    // 开始事务
    if (!BeginTransaction()) {
        return false;
    }

    // 首先清空该员工名下的所有资产（将 user_id 设为 NULL，状态改为闲置）
    sqlite3_stmt* stmt;
    const char* sqlUpdate = "UPDATE assets SET user_id = NULL, status = '闲置' WHERE user_id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sqlUpdate, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) {
            Rollback();
            m_lastError = sqlite3_errmsg(m_db);
            return false;
        }
    } else {
        Rollback();
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }

    // 然后删除员工记录
    const char* sqlDelete = "DELETE FROM employees WHERE id = ?;";
    rc = sqlite3_prepare_v2(m_db, sqlDelete, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_DONE) {
            Commit();
            return true;
        }
    }

    Rollback();
    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

int Database::GetEmployeeAssetCount(int employeeId) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM assets WHERE user_id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, employeeId);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return count;
        }
        sqlite3_finalize(stmt);
    }
    return 0;
}

std::unordered_map<int, int> Database::GetAllEmployeeAssetCounts() {
    std::unordered_map<int, int> result;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT user_id, COUNT(*) FROM assets WHERE user_id IS NOT NULL GROUP BY user_id;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int userId = sqlite3_column_int(stmt, 0);
            int count = sqlite3_column_int(stmt, 1);
            result[userId] = count;
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

std::vector<Employee> Database::SearchEmployees(const std::string& searchText, int departmentId) {
    std::vector<Employee> result;
    result.reserve(64);  // 预分配，减少内存重分配
    sqlite3_stmt* stmt;

    std::string sql = "SELECT id, name, department_id FROM employees WHERE 1=1";

    if (!searchText.empty()) {
        sql += " AND name LIKE ?";
    }
    if (departmentId >= 0) {
        sql += " AND department_id = ?";
    }
    sql += " ORDER BY name;";

    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return result;
    }

    int paramIdx = 1;
    if (!searchText.empty()) {
        std::string pattern = "%" + searchText + "%";
        sqlite3_bind_text(stmt, paramIdx++, pattern.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (departmentId >= 0) {
        sqlite3_bind_int(stmt, paramIdx++, departmentId);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Employee emp;
        emp.id = sqlite3_column_int(stmt, 0);
        emp.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        emp.departmentId = sqlite3_column_type(stmt, 2) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 2);
        result.push_back(std::move(emp));
    }

    sqlite3_finalize(stmt);
    return result;
}

// ========== 资产操作实现 ==========

std::vector<Asset> Database::GetAllAssets() {
    return SearchAssets("", -1, "");
}

std::vector<Asset> Database::SearchAssets(const std::string& searchText,
                                           int categoryId, const std::string& status) {
    std::vector<Asset> result;
    result.reserve(128);  // 预分配，减少内存重分配
    sqlite3_stmt* stmt;

    std::string sql = R"(
        SELECT a.id, a.asset_code, a.name, a.category_id, a.user_id,
               a.purchase_date, a.price, a.location, a.status, a.remark,
               c.name as cat_name, e.name as user_name, d.name as dept_name
        FROM assets a
        LEFT JOIN categories c ON a.category_id = c.id
        LEFT JOIN employees e ON a.user_id = e.id
        LEFT JOIN departments d ON e.department_id = d.id
        WHERE 1=1
    )";

    if (!searchText.empty()) {
        sql += " AND (a.asset_code LIKE ? OR a.name LIKE ? OR e.name LIKE ? OR a.remark LIKE ?)";
    }
    if (categoryId >= 0) {
        sql += " AND a.category_id = ?";
    }
    if (!status.empty()) {
        sql += " AND a.status = ?";
    }
    sql += " ORDER BY a.id DESC;";

    std::string searchPattern = "%" + searchText + "%";
    int paramIdx = 1;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        if (!searchText.empty()) {
            sqlite3_bind_text(stmt, paramIdx++, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, paramIdx++, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, paramIdx++, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, paramIdx++, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
        }
        if (categoryId >= 0) {
            sqlite3_bind_int(stmt, paramIdx++, categoryId);
        }
        if (!status.empty()) {
            sqlite3_bind_text(stmt, paramIdx++, status.c_str(), -1, SQLITE_TRANSIENT);
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Asset asset;
            BuildAssetFromStmt(stmt, asset);
            result.push_back(std::move(asset));
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

bool Database::GetAssetById(int id, Asset& asset) {
    sqlite3_stmt* stmt;
    const char* sql = R"(
        SELECT a.id, a.asset_code, a.name, a.category_id, a.user_id,
               a.purchase_date, a.price, a.location, a.status, a.remark,
               c.name as cat_name, e.name as user_name, d.name as dept_name
        FROM assets a
        LEFT JOIN categories c ON a.category_id = c.id
        LEFT JOIN employees e ON a.user_id = e.id
        LEFT JOIN departments d ON e.department_id = d.id
        WHERE a.id = ?;
    )";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            BuildAssetFromStmt(stmt, asset);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }

    return false;
}

bool Database::GetAssetByCode(const std::string& code, Asset& asset) {
    sqlite3_stmt* stmt;
    // 直接查询完整资产信息，避免二次查询
    const char* sql = R"(
        SELECT a.id, a.asset_code, a.name, a.category_id, a.user_id,
               a.purchase_date, a.price, a.location, a.status, a.remark,
               c.name as cat_name, e.name as user_name, d.name as dept_name
        FROM assets a
        LEFT JOIN categories c ON a.category_id = c.id
        LEFT JOIN employees e ON a.user_id = e.id
        LEFT JOIN departments d ON e.department_id = d.id
        WHERE a.asset_code = ?;
    )";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            BuildAssetFromStmt(stmt, asset);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }

    return false;
}

bool Database::GetLastAsset(Asset& asset) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, asset_code FROM assets ORDER BY id DESC LIMIT 1;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            asset.id = sqlite3_column_int(stmt, 0);
            asset.assetCode = (const char*)sqlite3_column_text(stmt, 1);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }

    return false;
}

bool Database::AddAsset(Asset& asset) {
    sqlite3_stmt* stmt;
    const char* sql = R"(
        INSERT INTO assets (asset_code, name, category_id, user_id, purchase_date,
                           price, location, status, remark)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, asset.assetCode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, asset.name.c_str(), -1, SQLITE_TRANSIENT);
        if (asset.categoryId >= 0) {
            sqlite3_bind_int(stmt, 3, asset.categoryId);
        } else {
            sqlite3_bind_null(stmt, 3);
        }
        if (asset.userId >= 0) {
            sqlite3_bind_int(stmt, 4, asset.userId);
        } else {
            sqlite3_bind_null(stmt, 4);
        }
        sqlite3_bind_text(stmt, 5, asset.purchaseDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 6, asset.price);
        sqlite3_bind_text(stmt, 7, asset.location.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 8, asset.status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 9, asset.remark.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE) {
            asset.id = (int)sqlite3_last_insert_rowid(m_db);
        }
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::UpdateAsset(const Asset& asset) {
    // 先获取旧的资产数据用于比较
    Asset oldAsset;
    if (!GetAssetById(asset.id, oldAsset)) {
        m_lastError = "找不到要更新的资产";
        return false;
    }

    // 比较字段变更并记录日志
    std::vector<AssetChangeLog> changeLogs;

    auto addLog = [&](const std::string& fieldName, const std::string& oldVal, const std::string& newVal) {
        if (oldVal != newVal) {
            AssetChangeLog log;
            log.assetId = asset.id;
            log.assetCode = asset.assetCode;
            log.assetName = asset.name;
            log.fieldName = fieldName;
            log.oldValue = oldVal;
            log.newValue = newVal;
            changeLogs.push_back(std::move(log));
        }
    };

    // 比较各字段
    addLog("资产编号", oldAsset.assetCode, asset.assetCode);
    addLog("资产名称", oldAsset.name, asset.name);
    addLog("分类", oldAsset.categoryName, asset.categoryName);
    addLog("使用人", oldAsset.userName, asset.userName);
    addLog("购入日期", oldAsset.purchaseDate, asset.purchaseDate);

    // 价格需要特殊处理
    std::ostringstream oldPriceStr, newPriceStr;
    oldPriceStr << std::fixed << std::setprecision(2) << oldAsset.price;
    newPriceStr << std::fixed << std::setprecision(2) << asset.price;
    addLog("价格", oldPriceStr.str(), newPriceStr.str());

    addLog("存放位置", oldAsset.location, asset.location);
    addLog("状态", oldAsset.status, asset.status);
    addLog("备注", oldAsset.remark, asset.remark);

    // 执行更新
    sqlite3_stmt* stmt;
    const char* sql = R"(
        UPDATE assets
        SET asset_code = ?, name = ?, category_id = ?, user_id = ?,
            purchase_date = ?, price = ?, location = ?, status = ?, remark = ?,
            updated_at = strftime('%s', 'now')
        WHERE id = ?;
    )";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, asset.assetCode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, asset.name.c_str(), -1, SQLITE_TRANSIENT);
        if (asset.categoryId >= 0) {
            sqlite3_bind_int(stmt, 3, asset.categoryId);
        } else {
            sqlite3_bind_null(stmt, 3);
        }
        if (asset.userId >= 0) {
            sqlite3_bind_int(stmt, 4, asset.userId);
        } else {
            sqlite3_bind_null(stmt, 4);
        }
        sqlite3_bind_text(stmt, 5, asset.purchaseDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 6, asset.price);
        sqlite3_bind_text(stmt, 7, asset.location.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 8, asset.status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 9, asset.remark.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 10, asset.id);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_DONE) {
            // 更新成功后记录变更日志
            if (!changeLogs.empty()) {
                AddChangeLogs(changeLogs);
            }
            return true;
        }
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::DeleteAsset(int id) {
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM assets WHERE id = ?;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::GetAssetStats(int& count, double& totalPrice) {
    count = 0;
    totalPrice = 0.0;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*), COALESCE(SUM(price), 0) FROM assets;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
            totalPrice = sqlite3_column_double(stmt, 1);
        }
        sqlite3_finalize(stmt);
        return true;
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::BeginTransaction() {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_lastError = errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::Commit() {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_lastError = errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::Rollback() {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        m_lastError = errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// ========== 变更日志操作实现 ==========

bool Database::AddChangeLog(const AssetChangeLog& log) {
    sqlite3_stmt* stmt;
    const char* sql = R"(
        INSERT INTO asset_change_logs (asset_id, asset_code, asset_name, field_name, old_value, new_value)
        VALUES (?, ?, ?, ?, ?, ?);
    )";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, log.assetId);
        sqlite3_bind_text(stmt, 2, log.assetCode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, log.assetName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, log.fieldName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, log.oldValue.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, log.newValue.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE);
    }

    m_lastError = sqlite3_errmsg(m_db);
    return false;
}

bool Database::AddChangeLogs(const std::vector<AssetChangeLog>& logs) {
    if (logs.empty()) return true;

    for (const auto& log : logs) {
        if (!AddChangeLog(log)) {
            return false;
        }
    }
    return true;
}

std::vector<AssetChangeLog> Database::GetChangeLogsByAssetId(int assetId) {
    std::vector<AssetChangeLog> result;
    result.reserve(32);  // 预分配，减少内存重分配
    sqlite3_stmt* stmt;

    const char* sql = R"(
        SELECT id, asset_id, asset_code, asset_name, field_name, old_value, new_value, change_time
        FROM asset_change_logs
        WHERE asset_id = ?
        ORDER BY change_time DESC, id DESC;
    )";

    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, assetId);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            AssetChangeLog log;
            log.id = sqlite3_column_int(stmt, 0);
            log.assetId = sqlite3_column_int(stmt, 1);
            log.assetCode = (const char*)sqlite3_column_text(stmt, 2);
            log.assetName = (const char*)sqlite3_column_text(stmt, 3);
            log.fieldName = (const char*)sqlite3_column_text(stmt, 4);
            const char* oldVal = (const char*)sqlite3_column_text(stmt, 5);
            const char* newVal = (const char*)sqlite3_column_text(stmt, 6);
            const char* changeTime = (const char*)sqlite3_column_text(stmt, 7);
            log.oldValue = oldVal ? oldVal : "";
            log.newValue = newVal ? newVal : "";
            log.changeTime = changeTime ? changeTime : "";
            result.push_back(std::move(log));
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

std::vector<AssetChangeLog> Database::GetAllChangeLogs(int limit, int offset) {
    std::vector<AssetChangeLog> result;
    result.reserve(limit > 0 ? limit : 128);  // 预分配，减少内存重分配
    sqlite3_stmt* stmt;

    std::string sql = R"(
        SELECT id, asset_id, asset_code, asset_name, field_name, old_value, new_value, change_time
        FROM asset_change_logs
        ORDER BY change_time DESC, id DESC
    )";

    if (limit > 0) {
        sql += " LIMIT " + std::to_string(limit);
        if (offset > 0) {
            sql += " OFFSET " + std::to_string(offset);
        }
    }
    sql += ";";

    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            AssetChangeLog log;
            log.id = sqlite3_column_int(stmt, 0);
            log.assetId = sqlite3_column_int(stmt, 1);
            log.assetCode = (const char*)sqlite3_column_text(stmt, 2);
            log.assetName = (const char*)sqlite3_column_text(stmt, 3);
            log.fieldName = (const char*)sqlite3_column_text(stmt, 4);
            const char* oldVal = (const char*)sqlite3_column_text(stmt, 5);
            const char* newVal = (const char*)sqlite3_column_text(stmt, 6);
            const char* changeTime = (const char*)sqlite3_column_text(stmt, 7);
            log.oldValue = oldVal ? oldVal : "";
            log.newValue = newVal ? newVal : "";
            log.changeTime = changeTime ? changeTime : "";
            result.push_back(std::move(log));
        }
        sqlite3_finalize(stmt);
    }

    return result;
}

std::vector<AssetChangeLog> Database::SearchChangeLogs(const std::string& searchText,
                                                        const std::string& startDate,
                                                        const std::string& endDate) {
    std::vector<AssetChangeLog> result;
    result.reserve(128);  // 预分配，减少内存重分配
    sqlite3_stmt* stmt;

    std::string sql = R"(
        SELECT id, asset_id, asset_code, asset_name, field_name, old_value, new_value, change_time
        FROM asset_change_logs
        WHERE 1=1
    )";

    if (!searchText.empty()) {
        sql += " AND (asset_code LIKE ? OR asset_name LIKE ? OR field_name LIKE ?)";
    }
    if (!startDate.empty()) {
        sql += " AND change_time >= ?";
    }
    if (!endDate.empty()) {
        sql += " AND change_time <= ?";
    }
    sql += " ORDER BY change_time DESC, id DESC;";

    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return result;
    }

    int paramIdx = 1;
    std::string searchPattern = "%" + searchText + "%";

    if (!searchText.empty()) {
        sqlite3_bind_text(stmt, paramIdx++, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, paramIdx++, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, paramIdx++, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (!startDate.empty()) {
        sqlite3_bind_text(stmt, paramIdx++, startDate.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (!endDate.empty()) {
        std::string endDateTime = endDate + " 23:59:59";
        sqlite3_bind_text(stmt, paramIdx++, endDateTime.c_str(), -1, SQLITE_TRANSIENT);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AssetChangeLog log;
        log.id = sqlite3_column_int(stmt, 0);
        log.assetId = sqlite3_column_int(stmt, 1);
        log.assetCode = (const char*)sqlite3_column_text(stmt, 2);
        log.assetName = (const char*)sqlite3_column_text(stmt, 3);
        log.fieldName = (const char*)sqlite3_column_text(stmt, 4);
        const char* oldVal = (const char*)sqlite3_column_text(stmt, 5);
        const char* newVal = (const char*)sqlite3_column_text(stmt, 6);
        const char* changeTime = (const char*)sqlite3_column_text(stmt, 7);
        log.oldValue = oldVal ? oldVal : "";
        log.newValue = newVal ? newVal : "";
        log.changeTime = changeTime ? changeTime : "";
        result.push_back(std::move(log));
    }

    sqlite3_finalize(stmt);
    return result;
}

int Database::GetChangeLogCount() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM asset_change_logs;";
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);

    int count = 0;
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return count;
}
