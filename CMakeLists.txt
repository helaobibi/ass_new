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
    src/ChangeLogDialog.cpp
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
    include/ChangeLogDialog.h
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

    # Release 模式下的反逆向选项
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_CONFIGURATION_TYPES)
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:/O2>           # 最大优化
            $<$<CONFIG:Release>:/Ob2>          # 内联函数展开
            $<$<CONFIG:Release>:/Oi>           # 启用内部函数
            $<$<CONFIG:Release>:/Ot>           # 优先速度
            $<$<CONFIG:Release>:/GL>           # 全程序优化
            $<$<CONFIG:Release>:/Gy>           # 函数级链接
            $<$<CONFIG:Release>:/GS->          # 禁用安全检查（减少特征）
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:/LTCG>         # 链接时代码生成
            $<$<CONFIG:Release>:/OPT:REF>      # 移除未引用函数
            $<$<CONFIG:Release>:/OPT:ICF>      # 合并相同函数
            $<$<CONFIG:Release>:/RELEASE>      # 设置校验和
        )
    endif()
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

    # Release 模式下的反逆向选项 (GCC/MinGW)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -O3                        # 最大优化
            -fvisibility=hidden        # 隐藏符号
            -fvisibility-inlines-hidden # 隐藏内联函数符号
            -ffunction-sections        # 每个函数单独段
            -fdata-sections            # 每个数据单独段
            -fno-ident                 # 不输出编译器标识
            -fmerge-all-constants      # 合并常量
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            -Wl,--gc-sections          # 移除未使用段
            -Wl,-s                     # 剥离所有符号
            -Wl,--build-id=none        # 不生成 build-id
        )
    endif()
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
