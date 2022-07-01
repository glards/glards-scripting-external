/*
 * This file is part of the CitizenFX project - http://citizen.re/
 *
 * See LICENSE and MENTIONS in the root of the source tree for information
 * regarding licensing.
 */

#include <unordered_set>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include "externalscripting.pb.h"
#include "externalscripting.grpc.pb.h"

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


class GrpcEndpoint
{
public:
	GrpcEndpoint(const std::string url)
	{
		m_url = url;
		auto channel = grpc::CreateChannel(url, grpc::InsecureChannelCredentials());
		m_stub = externalscripting::ExternalScripting::NewStub(channel);
		m_data = std::vector<externalscripting::EventData>();
	}

	void AddEvent(std::string name, std::string payload, std::string source)
	{
		auto data = externalscripting::EventData();
		data.set_eventname(name);
		data.set_eventpayload(payload);
		data.set_eventsource(source);
		m_data.emplace_back(data);
	}

	void SendEvents()
	{
		grpc::ClientContext ctx;
		for (auto ev : m_data)
		{
			externalscripting::EventResponse response;
			auto status = m_stub->TriggerEvent(&ctx, ev, &response);
			if (!status.ok())
			{
				trace("Unable to send RPC data to %s : %s", m_url, status.error_message());
				break;
			}
		}

		m_data.clear();
	}

private:

	std::vector<externalscripting::EventData> m_data;
	std::string m_url;
	std::unique_ptr<externalscripting::ExternalScripting::Stub> m_stub;
};

static std::unordered_set<GrpcEndpoint*> g_grpcEndpoints;


static InitFunction initFunction([]()
{
	static ConsoleCommand addGrpcEndpoint("add_grpc_endpoint", [](const std::string& url)
	{
		trace("Add GRPC URL %s\n", url);

		g_grpcEndpoints.emplace(new GrpcEndpoint{url});
	});

	fx::ResourceManager::OnInitializeInstance.Connect([](fx::ResourceManager* self)
	{
		const fwRefContainer<fx::ResourceEventManagerComponent> eventComponent = self->GetComponent<fx::ResourceEventManagerComponent>();
		
		if (eventComponent.GetRef())
		{
			eventComponent->OnTriggerEvent.Connect([](const std::string& eventName, const std::string& eventPayload, const std::string& eventSource, bool* eventCanceled)
			{
				for (GrpcEndpoint* ep : g_grpcEndpoints)
				{
					ep->AddEvent(eventName, eventPayload, eventSource);
				}
			});
		}

		self->OnTick.Connect([]()
		{
			//TODO: Send events remotely and dispatch local/client events
		});
	});
});
