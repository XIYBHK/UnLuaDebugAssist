# UnLuaDebugAssist Plugin

## 🎯 **功能概述**
自动检测软链接并为EmmyLua调试器提供智能路径映射，解决UnrealEngine项目中Lua脚本断点无法命中的问题。

## 🚀 **核心特性**
- **自动软链接检测** - 扫描项目中的Script目录软链接
- **智能路径映射** - 双向映射真实路径↔软链接路径
- **多格式支持** - 处理相对路径、@符号路径、反斜杠路径
- **跨平台兼容** - 支持Windows/Mac/Linux

## 📋 **工作原理**
1. **启动时检测** - UnLua环境创建后自动扫描软链接
2. **注入映射函数** - 覆盖`_G.emmy.fixPath`函数
3. **实时路径转换** - 调试器调用时自动映射路径格式
4. **绝对路径优化** - 相对路径自动转换为绝对路径

## 🔧 **安装使用**
1. 将插件放入项目`Plugins`目录
2. 重新生成项目文件并编译
3. 启动游戏，插件自动工作
4. 在IDEA中正常设置断点即可

## 📊 **日志输出**
```
LogUnLuaDebugAssist: 检测到软链接
LogUnLua: [UnLuaDebugAssist] fixPath called with: /real/path/script.lua
LogUnLua: [UnLuaDebugAssist] Path mapped: /real/path -> /symlink/path
```

## ⚙️ **技术实现**
- **软链接检测**: `GetRealPath()` + 路径比较
- **Lua脚本注入**: 动态生成映射规则和处理函数
- **路径规范化**: 处理`../`、`./`、多重斜杠
- **文件验证**: 确保映射后路径文件存在

## 🎯 **适用场景**
- UnrealEngine + UnLua + EmmyLua调试环境
- 使用软链接管理Lua脚本的项目
- 需要跨IDE调试的开发团队

## 📁 **项目结构**
```
UnLuaDebugAssist/
├── UnLuaDebugAssist.uplugin          # 插件配置文件
└── Source/
    └── UnLuaDebugAssist/
        ├── UnLuaDebugAssist.Build.cs # 构建配置
        ├── Public/
        │   └── UnLuaDebugAssistModule.h
        └── Private/
            └── UnLuaDebugAssistModule.cpp
```

## 🤝 **贡献**
欢迎提交Issue和Pull Request来改进这个插件！

## 📄 **许可证**
MIT License