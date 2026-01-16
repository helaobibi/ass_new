cmake_minimum_required(VERSION 3.20)
project(AssetManager VERSION 1.0.0 LANGUAGES C CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 源文件
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/database.cpp
    src/AssetEditDialog.cpp
    src/CategoryManageDialog.cpp
    src/EmployeeManageDialog.cpp
    src/CSVHelper.cpp
    include/sqlite3.c
)

# 头文件
set(HEADERS
    include/models.h
    include/database.h
    include/MainWindow.h
    include/AssetEditDialog.h
    include/CategoryManageDialog.h
    include/EmployeeManageDialog.h
    include/CSVHelper.h
)

# 资源文件
set(RESOURCES
    res/app_fixed.rc
)

# 可执行文件
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接库
# SQLite 使用源码编译 (sqlite3.c 已添加到 SOURCES)

# Windows 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    comctl32
    gdi32
    shell32
)

# 编译选项
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /utf-8
    )
    # 设置字符集为 Unicode
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
    )
    # 设置字符集为 Unicode (MinGW)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

    # MinGW 静态链接 - 生成独立可执行文件，无需依赖 DLL
    target_link_options(${PROJECT_NAME} PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

# 设置子系统为 Windows
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup"
    )
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 复制 SQLite DLL (如果需要)
# install(FILES ${SQLITE3_DLL} DESTINATION .)
