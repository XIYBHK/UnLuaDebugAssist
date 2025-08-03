using UnrealBuildTool;

public class UnLuaDebugAssist : ModuleRules
{
    public UnLuaDebugAssist(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Projects",
                "ApplicationCore"
            }
        );

        // 仅在编辑器环境下添加编辑器相关依赖
        if (Target.Type == TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "ToolMenus",
                    "EditorStyle",
                    "EditorWidgets",
                    "UnrealEd",
                    "ToolWidgets",
                    "PropertyEditor"
                }
            );
        }

        // 直接添加UnLua依赖
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnLua",
                "Lua"
            }
        );

        // 启用UnLua支持
        PublicDefinitions.Add("WITH_UNLUA=1");
    }
}