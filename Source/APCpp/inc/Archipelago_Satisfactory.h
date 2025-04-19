#include <vector>
#include <string>
#include <functional>

int AP_GetCurrentPlayerTeam();

std::string AP_GetItemName(std::string game, int64_t id);

std::vector<std::pair<int,std::string>> AP_GetAllPlayers();

void AP_SetLoggingCallback(std::function<void(std::string)> f_log);

std::vector<int64_t> AP_GetAllLocations();
