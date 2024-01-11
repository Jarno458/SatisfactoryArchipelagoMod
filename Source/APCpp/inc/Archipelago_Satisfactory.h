#include <vector>
#include <string>

int AP_GetCurrentPlayerTeam();

std::string AP_GetItemName(std::string game, int64_t id);

std::vector<std::pair<int,std::string>> AP_GetAllPlayers();

void AP_Say(std::string message);
