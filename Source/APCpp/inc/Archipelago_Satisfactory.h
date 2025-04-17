#include <vector>
#include <string>

int AP_GetCurrentPlayerTeam();

std::string AP_GetItemName(std::string game, int64_t id);

std::vector<std::pair<int,std::string>> AP_GetAllPlayers();

void AP_SetLoggingCallback(void (*f_log)(std::string));

std::vector<int64_t> AP_GetAllLocations();
