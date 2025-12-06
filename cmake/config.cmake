# Configuration for different platforms and compilers
# 根据不同平台和编译器配置编译选项

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 通用编译选项
set(COMMON_COMPILE_OPTIONS)
set(COMMON_COMPILE_DEFINITIONS)

# 根据构建类型设置编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND COMMON_COMPILE_DEFINITIONS DEBUG _DEBUG)
    message(STATUS "Debug build configuration")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND COMMON_COMPILE_DEFINITIONS NDEBUG)
    message(STATUS "Release build configuration")
endif()

# ================== 平台特定配置 ==================

# Windows 平台配置
if(WIN32)
    message(STATUS "Configuring for Windows platform")
    
    list(APPEND COMMON_COMPILE_DEFINITIONS
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        _SCL_SECURE_NO_WARNINGS
    )
    
    # Windows 特定编译选项
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND COMMON_COMPILE_OPTIONS /MDd)
    else()
        list(APPEND COMMON_COMPILE_OPTIONS /MD)
    endif()

# Linux 平台配置
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux platform")
    
    list(APPEND COMMON_COMPILE_DEFINITIONS
        _GNU_SOURCE
        _POSIX_C_SOURCE=200809L
    )
    
    # Linux 特定编译选项
    list(APPEND COMMON_COMPILE_OPTIONS
        -fPIC
        -pthread
    )

# macOS 平台配置
elseif(APPLE)
    message(STATUS "Configuring for macOS platform")
    
    list(APPEND COMMON_COMPILE_DEFINITIONS
        _DARWIN_C_SOURCE
    )
    
    # macOS 特定编译选项
    list(APPEND COMMON_COMPILE_OPTIONS
        -fPIC
    )
endif()

# ================== 编译器特定配置 ==================

# MSVC 编译器配置
if(MSVC)
    message(STATUS "Configuring for MSVC compiler")
    
    list(APPEND COMMON_COMPILE_OPTIONS
    #     /W4                    # 警告级别4
    #     /WX-                   # 警告不作为错误处理
    #     /permissive-           # 禁用非标准扩展
        /Zc:__cplusplus        # 正确的 __cplusplus 宏值
    #     /Zc:inline             # 移除未引用的函数
        /utf-8                 # UTF-8 源码编码
    #     /bigobj                # 大对象文件支持
    )
    
    # Debug 模式特定选项
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")

        list(APPEND COMMON_COMPILE_OPTIONS
            /Od     # 禁用优化
            /Zi     # 生成调试信息
            /RTC1   # 运行时检查
        )

    else()
        # Release 模式特定选项
        
        list(APPEND COMMON_COMPILE_OPTIONS
            /O2     # 最大化速度优化
            /Ob2    # 内联函数扩展
            /GL     # 全程序优化
        )

        # Release 链接器选项
       
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
    endif()

# GCC 编译器配置
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Configuring for GCC compiler")
    
    list(APPEND COMMON_COMPILE_OPTIONS
        -Wall                  # 基本警告
        -Wextra                # 额外警告
        -Wpedantic             # 严格标准警告
        -Wno-unused-parameter  # 忽略未使用参数警告
        -fdiagnostics-color=always  # 彩色诊断输出
    )
    
    # GCC 版本特定配置
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "7.0")
        list(APPEND COMMON_COMPILE_OPTIONS
            -Wduplicated-cond
            -Wduplicated-branches
            -Wnull-dereference
        )
    endif()
    
    # Debug 模式特定选项
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND COMMON_COMPILE_OPTIONS
            -O0     # 无优化
            -g3     # 最详细调试信息
            -ggdb   # GDB 调试信息
            -fno-omit-frame-pointer
        )
    else()
        # Release 模式特定选项
        list(APPEND COMMON_COMPILE_OPTIONS
            -O3     # 最高优化级别
            -DNDEBUG
            -flto   # 链接时优化
        )
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto")
    endif()

# Clang 编译器配置
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Configuring for Clang compiler")
    
    list(APPEND COMMON_COMPILE_OPTIONS
        -Wall                  # 基本警告
        -Wextra                # 额外警告
        -Wpedantic             # 严格标准警告
        -Wno-unused-parameter  # 忽略未使用参数警告
        -fcolor-diagnostics    # 彩色诊断输出
        -fdiagnostics-show-template-tree  # 显示模板树
    )
    
    # Debug 模式特定选项
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND COMMON_COMPILE_OPTIONS
            -O0     # 无优化
            -g      # 调试信息
            -fno-omit-frame-pointer
            # -fsanitize-address-use-after-scope  # 地址消毒器
        )
    else()
        # Release 模式特定选项
        list(APPEND COMMON_COMPILE_OPTIONS
            -O3     # 最高优化级别
            -DNDEBUG
            -flto   # 链接时优化
        )
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto")
    endif()

# Intel C++ 编译器配置
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    message(STATUS "Configuring for Intel C++ compiler")
    
    if(WIN32)
        list(APPEND COMMON_COMPILE_OPTIONS
            /W3
            /Qstd=c++17
        )
    else()
        list(APPEND COMMON_COMPILE_OPTIONS
            -Wall
            -std=c++17
        )
    endif()
endif()

# ================== 架构特定配置 ==================

# 64位架构特定配置
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Configuring for 64-bit architecture")
    list(APPEND COMMON_COMPILE_DEFINITIONS ARCH_64BIT)
    
    # x64 特定优化
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        if(MSVC)
            list(APPEND COMMON_COMPILE_OPTIONS /arch:AVX2)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            list(APPEND COMMON_COMPILE_OPTIONS -march=native -mtune=native)
        endif()
    endif()
else()
    message(STATUS "Configuring for 32-bit architecture")
    list(APPEND COMMON_COMPILE_DEFINITIONS ARCH_32BIT)
endif()

# ================== 功能配置 ==================

# 启用 AddressSanitizer (仅在 Debug 模式)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        list(APPEND COMMON_COMPILE_OPTIONS -fsanitize=address -fno-omit-frame-pointer)
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
        set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address")
    endif()
endif()

# 启用 ThreadSanitizer (仅在 Debug 模式)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
if(ENABLE_TSAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        list(APPEND COMMON_COMPILE_OPTIONS -fsanitize=thread)
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=thread")
        set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=thread")
    endif()
endif()

# ================== 应用配置的函数 ==================

# 为目标应用编译选项的函数
function(apply_compiler_config target)
    target_compile_options(${target} PRIVATE ${COMMON_COMPILE_OPTIONS})
    target_compile_definitions(${target} PRIVATE ${COMMON_COMPILE_DEFINITIONS})
    
    # 设置目标属性
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    
    # Windows 特定属性
    if(WIN32 AND MSVC)
        set_target_properties(${target} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif()
    
    message(STATUS "Applied compiler configuration to target: ${target}")
endfunction()

# 打印当前配置信息
message(STATUS "=== Compiler Configuration Summary ===")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
if(COMMON_COMPILE_OPTIONS)
    message(STATUS "Compile Options: ${COMMON_COMPILE_OPTIONS}")
endif()
if(COMMON_COMPILE_DEFINITIONS)
    message(STATUS "Compile Definitions: ${COMMON_COMPILE_DEFINITIONS}")
endif()
message(STATUS "=====================================")
