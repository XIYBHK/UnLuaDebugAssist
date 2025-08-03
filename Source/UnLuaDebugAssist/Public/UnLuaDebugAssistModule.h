#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#if WITH_UNLUA
// UnLua前向声明
namespace UnLua
{
    class FLuaEnv;
}
#endif

class FUnLuaDebugAssistModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
#if WITH_UNLUA
    /**
     * @brief 当UnLua环境初始化完成后被调用的函数
     * @param LuaEnv UnLua环境实例
     */
    void OnUnLuaReady(UnLua::FLuaEnv& LuaEnv);

    /**
     * @brief 用于在关闭模块时解绑委托
     */
    FDelegateHandle OnPostInitDelegateHandle;
#endif

    /**
     * @brief 获取平台的真实物理路径
     * @param InPath 输入的路径
     * @return 如果是软链接，返回真实路径；否则返回原路径
     */
    FString GetRealPath(const FString& InPath);

    /**
     * @brief 智能发现项目中的所有软链接
     * @param OutSymlinks 输出发现的软链接对（软链接路径，真实路径）
     */
    void DiscoverSymlinks(TArray<TPair<FString, FString>>& OutSymlinks);

    /**
     * @brief 为指定的软链接生成智能路径映射
     * @param LuaEnv UnLua环境实例
     * @param SymlinkPath 软链接路径
     * @param RealPath 真实路径
     */
    void GenerateSmartPathMapping(UnLua::FLuaEnv& LuaEnv, const FString& SymlinkPath, const FString& RealPath);
};