#include <vector>
#include <string>
#include <functional>

int AP_GetCurrentPlayerTeam();

std::string AP_GetItemName(std::string game, int64_t id);

std::vector<AP_NetworkPlayer> AP_GetAllPlayers();

void AP_SetLoggingCallback(std::function<void(std::string)> f_log);

void AP_SetPackageReceivedCallback(std::function<void(std::string)> onPackageReceived);

std::vector<int64_t> AP_GetAllLocations();

std::string AP_GetSlotData();

void AP_EnabledDeathlinkAnyway();

void AP_Send(std::string cmd);