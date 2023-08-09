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
            PublicAdditionalLibraries.Add(Path.Combine(LibFolder, "APCpp-static.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibFolder, "ixwebsocket.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibFolder, "jsoncpp.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibFolder, "mbedcrypto.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibFolder, "mbedtls.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibFolder, "mbedx509.lib"));

            PublicSystemLibraries.Add("crypt32.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            throw new Exception("Not Supported Platform");
        }
    }
}