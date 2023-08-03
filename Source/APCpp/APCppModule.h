#pragma once

#include "Modules/ModuleManager.h"

class FAPCppModule : public IModuleInterface {
	public:
		virtual void StartupModule() override;
};