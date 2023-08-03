using UnrealBuildTool;
using System.IO;
using System;

public class APCpp : ModuleRules
{
    public APCpp (ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
        });

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "APCpp-static.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "ixwebsocket.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "jsoncpp.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "mbedcrypto.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "mbedtls.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "mbedx509.lib"));

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.Add("crypt32.lib");
        }
    }
}