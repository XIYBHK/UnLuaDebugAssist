#include "UnLuaDebugAssistModule.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#if WITH_UNLUA
#include "UnLua.h"
#endif

// 平台相关头文件
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <fileapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_MAC || PLATFORM_LINUX
#include <limits.h>
#include <stdlib.h>
#endif

#define LOCTEXT_NAMESPACE "FUnLuaDebugAssistModule"

// 定义插件专用的日志分类
DEFINE_LOG_CATEGORY_STATIC(LogUnLuaDebugAssist, Log, All);

void FUnLuaDebugAssistModule::StartupModule()
{
    UE_LOG(LogUnLuaDebugAssist, Log, TEXT("插件启动"));

#if WITH_UNLUA
    // 绑定委托，当UnLua环境创建完毕后，会调用我们的 OnUnLuaReady 函数
    OnPostInitDelegateHandle = UnLua::FLuaEnv::OnCreated.AddLambda([this](UnLua::FLuaEnv& LuaEnv)
    {
        this->OnUnLuaReady(LuaEnv);
    });
#else
    UE_LOG(LogUnLuaDebugAssist, Warning, TEXT("UnLua插件未找到，路径修复功能将不可用"));
#endif
}

void FUnLuaDebugAssistModule::ShutdownModule()
{
    UE_LOG(LogUnLuaDebugAssist, Log, TEXT("插件关闭"));

#if WITH_UNLUA
    // 解绑委托，防止悬空指针
    if (OnPostInitDelegateHandle.IsValid())
    {
        UnLua::FLuaEnv::OnCreated.Remove(OnPostInitDelegateHandle);
        OnPostInitDelegateHandle.Reset();
    }
#endif
}

#if WITH_UNLUA
void FUnLuaDebugAssistModule::OnUnLuaReady(UnLua::FLuaEnv& LuaEnv)
{
    UE_LOG(LogUnLuaDebugAssist, Log, TEXT("开始智能路径检测..."));

    // 动态发现所有可能的脚本目录软链接
    TArray<TPair<FString, FString>> DetectedSymlinks;
    DiscoverSymlinks(DetectedSymlinks);

    if (DetectedSymlinks.Num() == 0)
    {
        UE_LOG(LogUnLuaDebugAssist, Log, TEXT("未检测到软链接路径"));
        return;
    }

    // 为每个检测到的软链接生成智能映射
    for (const auto& SymlinkPair : DetectedSymlinks)
    {
        const FString& SymlinkPath = SymlinkPair.Key;
        const FString& RealPath = SymlinkPair.Value;

        UE_LOG(LogUnLuaDebugAssist, Warning, TEXT("检测到软链接"));
        UE_LOG(LogUnLuaDebugAssist, Warning, TEXT("    软链接路径: %s"), *SymlinkPath);
        UE_LOG(LogUnLuaDebugAssist, Warning, TEXT("    真实路径: %s"), *RealPath);

        // 生成智能路径映射脚本
        GenerateSmartPathMapping(LuaEnv, SymlinkPath, RealPath);
    }
}

void FUnLuaDebugAssistModule::DiscoverSymlinks(TArray<TPair<FString, FString>>& OutSymlinks)
{
    // 搜索常见的脚本目录位置
    TArray<FString> SearchPaths = {
        FPaths::ProjectContentDir(),
        FPaths::ProjectDir(),
        FPaths::ProjectDir() / TEXT("Source"),
        FPaths::ProjectDir() / TEXT("Scripts"),
        FPaths::ProjectDir() / TEXT("Lua")
    };

    TArray<FString> CommonScriptDirNames = { TEXT("Script"), TEXT("Scripts"), TEXT("Lua"), TEXT("LuaSource") };

    for (const FString& SearchPath : SearchPaths)
    {
        for (const FString& DirName : CommonScriptDirNames)
        {
            FString PotentialSymlink = SearchPath / DirName;
            FPaths::NormalizeDirectoryName(PotentialSymlink);

            if (FPaths::DirectoryExists(PotentialSymlink))
            {
                FString RealPath = GetRealPath(PotentialSymlink);
                FPaths::NormalizeDirectoryName(RealPath);

                // 如果路径不同，说明是软链接
                if (PotentialSymlink != RealPath)
                {
                    // 标准化路径格式
                    PotentialSymlink.ReplaceInline(TEXT("\\"), TEXT("/"));
                    RealPath.ReplaceInline(TEXT("\\"), TEXT("/"));

                    OutSymlinks.Add(TPair<FString, FString>(PotentialSymlink, RealPath));
                    UE_LOG(LogUnLuaDebugAssist, Log, TEXT("发现软链接: %s -> %s"), *PotentialSymlink, *RealPath);
                }
            }
        }
    }
}

void FUnLuaDebugAssistModule::GenerateSmartPathMapping(UnLua::FLuaEnv& LuaEnv, const FString& SymlinkPath, const FString& RealPath)
{
    FString LuaScript;
    
    // 生成路径映射脚本
    LuaScript += TEXT("do\n");
    LuaScript += TEXT("    local symlinkPath = '") + SymlinkPath + TEXT("'\n");
    LuaScript += TEXT("    local realPath = '") + RealPath + TEXT("'\n");
    LuaScript += TEXT("    \n");
    LuaScript += TEXT("    -- 确保emmy全局表存在\n");
    LuaScript += TEXT("    if not _G.emmy then\n");
    LuaScript += TEXT("        _G.emmy = {}\n");
    LuaScript += TEXT("    end\n");
    LuaScript += TEXT("    \n");
    LuaScript += TEXT("    -- 保存原始的fixPath函数\n");
    LuaScript += TEXT("    local oldFixPath = _G.emmy.fixPath\n");
    LuaScript += TEXT("    \n");
    LuaScript += TEXT("    -- 创建映射规则\n");
    LuaScript += TEXT("    local mappingRules = {}\n");
    LuaScript += TEXT("    table.insert(mappingRules, {from = realPath, to = symlinkPath, name = 'real->symlink'})\n");
    LuaScript += TEXT("    table.insert(mappingRules, {from = symlinkPath, to = realPath, name = 'symlink->real'})\n");
    LuaScript += TEXT("    \n");
    LuaScript += TEXT("    -- 智能路径修复函数\n");
    LuaScript += TEXT("    _G.emmy.fixPath = function(path)\n");
    LuaScript += TEXT("        print('[UnLuaDebugAssist] fixPath called with: ' .. tostring(path))\n");
    LuaScript += TEXT("        if not path or path == '' then \n");
    LuaScript += TEXT("            return path \n");
    LuaScript += TEXT("        end\n");
    LuaScript += TEXT("        \n");
    LuaScript += TEXT("        local newPath = path\n");
    LuaScript += TEXT("        \n");
    LuaScript += TEXT("        local applied = false\n");
    LuaScript += TEXT("        \n");
    LuaScript += TEXT("        -- 尝试所有映射规则\n");
    LuaScript += TEXT("        for _, rule in ipairs(mappingRules) do\n");
    LuaScript += TEXT("            local transformed = string.gsub(newPath, rule.from, rule.to)\n");
    LuaScript += TEXT("            if transformed ~= newPath then\n");
    LuaScript += TEXT("                newPath = transformed\n");
    LuaScript += TEXT("                applied = true\n");
    LuaScript += TEXT("                \n");
    LuaScript += TEXT("                -- 如果映射后是相对路径，转换为绝对路径\n");
    LuaScript += TEXT("                if string.sub(newPath, 1, 3) == '../' then\n");
    LuaScript += TEXT("                    local symlinkDir = string.match(symlinkPath, '^(.+)/[^/]+$') or symlinkPath\n");
    LuaScript += TEXT("                    local absolutePath = symlinkDir .. '/' .. string.gsub(newPath, '^%.%./', '')\n");
    LuaScript += TEXT("                    -- 规范化路径\n");
    LuaScript += TEXT("                    absolutePath = string.gsub(absolutePath, '/+', '/')\n");
    LuaScript += TEXT("                    while string.find(absolutePath, '/%.%./') do\n");
    LuaScript += TEXT("                        absolutePath = string.gsub(absolutePath, '/[^/]+/%.%./', '/')\n");
    LuaScript += TEXT("                    end\n");
    LuaScript += TEXT("                    absolutePath = string.gsub(absolutePath, '/%./', '/')\n");
    LuaScript += TEXT("                    \n");
    LuaScript += TEXT("                    -- 检查绝对路径文件是否存在\n");
    LuaScript += TEXT("                    local file = io.open(absolutePath, 'r')\n");
    LuaScript += TEXT("                    if file then\n");
    LuaScript += TEXT("                        file:close()\n");
    LuaScript += TEXT("                        newPath = absolutePath\n");
    LuaScript += TEXT("                    end\n");
    LuaScript += TEXT("                end\n");
    LuaScript += TEXT("                break\n");
    LuaScript += TEXT("            end\n");
    LuaScript += TEXT("        end\n");
    LuaScript += TEXT("        \n");
    LuaScript += TEXT("        -- 处理@符号开头的路径\n");
    LuaScript += TEXT("        if string.sub(newPath, 1, 1) == '@' then\n");
    LuaScript += TEXT("            local pathWithoutAt = string.sub(newPath, 2)\n");
    LuaScript += TEXT("            for _, rule in ipairs(mappingRules) do\n");
    LuaScript += TEXT("                local transformed = string.gsub(pathWithoutAt, rule.from, rule.to)\n");
    LuaScript += TEXT("                if transformed ~= pathWithoutAt then\n");
    LuaScript += TEXT("                    newPath = '@' .. transformed\n");
    LuaScript += TEXT("                    applied = true\n");
    LuaScript += TEXT("                    break\n");
    LuaScript += TEXT("                end\n");
    LuaScript += TEXT("            end\n");
    LuaScript += TEXT("        end\n");
    LuaScript += TEXT("        \n");
    LuaScript += TEXT("        -- 处理反斜杠路径\n");
    LuaScript += TEXT("        if not applied then\n");
    LuaScript += TEXT("            for _, rule in ipairs(mappingRules) do\n");
    LuaScript += TEXT("                local fromBackslash = string.gsub(rule.from, '/', '\\\\\\\\')\n");
    LuaScript += TEXT("                local toBackslash = string.gsub(rule.to, '/', '\\\\\\\\')\n");
    LuaScript += TEXT("                local transformed = string.gsub(newPath, fromBackslash, toBackslash)\n");
    LuaScript += TEXT("                if transformed ~= newPath then\n");
    LuaScript += TEXT("                    newPath = transformed\n");
    LuaScript += TEXT("                    applied = true\n");
    LuaScript += TEXT("                    break\n");
    LuaScript += TEXT("                end\n");
    LuaScript += TEXT("            end\n");
    LuaScript += TEXT("        end\n");
    LuaScript += TEXT("        \n");
    LuaScript += TEXT("        -- 如果没有应用任何映射，尝试备用方案\n");
    LuaScript += TEXT("        if not applied then\n");
    LuaScript += TEXT("            -- 方案1: 尝试将绝对路径转换为项目相对路径\n");
    LuaScript += TEXT("            if string.find(path, '/MyLuaTest/Content/Script/') then\n");
    LuaScript += TEXT("                local relativePath = string.gsub(path, '.*/MyLuaTest/Content/Script/', 'Content/Script/')\n");
    LuaScript += TEXT("                newPath = relativePath\n");
    LuaScript += TEXT("                applied = true\n");
    LuaScript += TEXT("            -- 方案2: 尝试直接使用软链接路径\n");
    LuaScript += TEXT("            elseif string.find(path, realPath) then\n");
    LuaScript += TEXT("                local symlinkVersion = string.gsub(path, realPath, symlinkPath)\n");
    LuaScript += TEXT("                newPath = symlinkVersion\n");
    LuaScript += TEXT("                applied = true\n");
    LuaScript += TEXT("            end\n");
    LuaScript += TEXT("        end\n");
    LuaScript += TEXT("        \n");
    LuaScript += TEXT("        -- 调用原始的fixPath函数\n");
    LuaScript += TEXT("        if oldFixPath and type(oldFixPath) == 'function' then\n");
    LuaScript += TEXT("            newPath = oldFixPath(newPath)\n");
    LuaScript += TEXT("        end\n");
    LuaScript += TEXT("        \n");
    LuaScript += TEXT("        -- 输出关键映射信息\n");
    LuaScript += TEXT("        if newPath ~= path then\n");
    LuaScript += TEXT("            print('[UnLuaDebugAssist] Path mapped: ' .. path .. ' -> ' .. newPath)\n");
    LuaScript += TEXT("        end\n");
    LuaScript += TEXT("        \n");
    LuaScript += TEXT("        return newPath\n");
    LuaScript += TEXT("    end\n");
    LuaScript += TEXT("    \n");
    LuaScript += TEXT("    print('[UnLuaDebugAssist] Smart path mapping function is ready')\n");
    LuaScript += TEXT("end\n");

    // 执行Lua脚本
    if (LuaEnv.DoString(TCHAR_TO_UTF8(*LuaScript)))
    {
        UE_LOG(LogUnLuaDebugAssist, Log, TEXT("✅ 路径映射脚本执行成功"));
    }
    else
    {
        UE_LOG(LogUnLuaDebugAssist, Error, TEXT("❌ 路径映射脚本执行失败"));
    }
}

FString FUnLuaDebugAssistModule::GetRealPath(const FString& Path)
{
#if PLATFORM_WINDOWS
    TCHAR Buffer[MAX_PATH];
    DWORD Result = GetFinalPathNameByHandle(
        CreateFile(*Path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
                   NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL),
        Buffer, MAX_PATH, FILE_NAME_NORMALIZED);
    
    if (Result > 0 && Result < MAX_PATH)
    {
        FString RealPath(Buffer);
        // 移除Windows的\\?\前缀
        if (RealPath.StartsWith(TEXT("\\\\?\\")))
        {
            RealPath = RealPath.Mid(4);
        }
        return RealPath;
    }
#elif PLATFORM_MAC || PLATFORM_LINUX
    char Buffer[PATH_MAX];
    if (realpath(TCHAR_TO_UTF8(*Path), Buffer))
    {
        return FString(UTF8_TO_TCHAR(Buffer));
    }
#endif
    
    return Path; // 如果获取失败，返回原路径
}

#endif

IMPLEMENT_MODULE(FUnLuaDebugAssistModule, UnLuaDebugAssist)

#undef LOCTEXT_NAMESPACE