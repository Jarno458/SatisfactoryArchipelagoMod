using UnrealBuildTool;
using System.IO;
using System;

public class APCpp : ModuleRules
{
    public APCpp (ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "inc"));

        var PlatformName = Target.Platform.ToString();

        var LibFolder = Path.Combine(ModuleDirectory, "lib", PlatformName);

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.AddRange(Directory.EnumerateFiles(LibFolder, "*.lib"));

            PublicSystemLibraries.Add("crypt32.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            throw new Exception("Not Supported Platform");
        }
    }
}