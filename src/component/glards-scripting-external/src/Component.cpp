/*
 * This file is part of the CitizenFX project - http://citizen.re/
 *
 * See LICENSE and MENTIONS in the root of the source tree for information
 * regarding licensing.
 */

#include <unordered_set>

#include "StdInc.h"
#include "ComponentLoader.h"
#include "ResourceEventComponent.h"
#include "ResourceManager.h"
#include "console/Console.CommandHelpers.h"

class ComponentInstance : public Component
{
public:
	virtual bool Initialize();

	virtual bool DoGameLoad(void* module);

	virtual bool Shutdown();
};

bool ComponentInstance::Initialize()
{
	trace("ComponentInstance::Initialize\n");
	InitFunctionBase::RunAll();


	return true;
}

bool ComponentInstance::DoGameLoad(void* module)
{
	trace("ComponentInstance::DoGameLoad\n");

	HookFunction::RunAll();

	return true;
}

bool ComponentInstance::Shutdown()
{
	trace("ComponentInstance::Shutdown\n");
	return true;
}

extern "C" DLL_EXPORT Component* CreateComponent()
{
	return new ComponentInstance();
}


static std::mutex urlMutex;
static std::unordered_set<std::string> grpcUrls;

static InitFunction initFunction([]()
{
	static ConsoleCommand addGrpcEndpoint("add_grpc_endpoint", [](const std::string& url)
	{
		trace("Add GRPC URL %s\n", url);

		const std::lock_guard lock(urlMutex);
		grpcUrls.emplace(url);
	});

	fx::ResourceManager::OnInitializeInstance.Connect([](fx::ResourceManager* self)
	{
		const fwRefContainer<fx::ResourceEventManagerComponent> eventComponent = self->GetComponent<fx::ResourceEventManagerComponent>();
		
		if (eventComponent.GetRef())
		{
			eventComponent->OnTriggerEvent.Connect([](const std::string& eventName, const std::string& eventPayload, const std::string& eventSource, bool* eventCanceled)
			{
				//TODO: Store events for dispatch
			});
		}

		self->OnTick.Connect([]()
		{
			//TODO: Send events remotely and dispatch local/client events
		});
	});
});
