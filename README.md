# README.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

固定资产管理系统（AssetManager）- 使用 C++17 和 Win32 API 开发的 Windows 桌面应用程序。

## 构建命令

```bash
# 创建构建目录并配置（MSVC）
cmake -B build -G "Visual Studio 17 2022"

# 或使用 MinGW
cmake -B build -G "MinGW Makefiles"

# 构建
cmake --build build --config Release

# 输出位置
# build/bin/AssetManager.exe
```

## 架构

### 分层结构

```
UI层 (Win32 API)
    ↓
业务逻辑层 (MainWindow, Dialogs)
    ↓
数据访问层 (Database)
    ↓
SQLite (源码编译)
```

### 核心组件

- **MainWindow** (`MainWindow.h/cpp`): 主窗口，管理菜单、工具栏、列表视图、状态栏。使用静态 `WindowProc` + 实例 `HandleMessage` 模式处理消息。
- **Database** (`database.h/cpp`): SQLite C API 封装，RAII 模式管理连接，提供所有 CRUD 操作和事务支持。
- **models.h**: 数据模型定义 - Asset、Category、Department、Employee。

### 对话框组件

- **AssetEditDialog**: 资产新增/编辑，支持自动生成资产编号
- **CategoryManageDialog**: 分类管理
- **EmployeeManageDialog**: 员工和部门管理
- **CSVHelper**: CSV 导入导出功能

### 关键设计模式

- 所有窗口类禁止拷贝（`= delete`）
- 全局窗口指针 `g_mainWindow` 用于 WindowProc 回调
- 数据库使用 WAL 模式和外键约束
- Logger 使用单例模式，输出到 `debug.log`

## 编码规范

- 类名和成员函数：PascalCase
- 成员变量：`m_` 前缀 + 驼峰命名
- 控件 ID：`ID_` 前缀，菜单 ID：`IDM_` 前缀
- 字符集：Unicode（`_UNICODE` 宏）

## 数据模型关系

```
Asset.categoryId → Category.id
Asset.userId → Employee.id
Employee.departmentId → Department.id
```

资产状态值：在用、闲置、维修中、已报废
