# SatisfactoryArchipelagoMod





# APCpp
This mod uses https://github.com/N00byKing/APCpp to communicate with the AP server
In order to get this to work this mod uses static compiled libraries from APCpp, in order to do a static compile of APCpp add this line below line 9 in the `CMakeLists.txt` inside APCpp
`add_library(APCpp-static STATIC Archipelago.cpp Archipelago.h)` 
To update the used APCpp version, build a STATIC library version then:
copy all *.lib over from `APCpp\build\lib\Release` to `Archipelago\Source\APCpp\lib`
copy `Archipelago.h` over to `APCpp` to `Archipelago\Source\APCpp\public`