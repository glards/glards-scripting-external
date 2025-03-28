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

#include <wincrypt.h>

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

		grpc::ChannelArguments gargs;

		auto sslOpt = getSslOptions();
		auto creds = grpc::SslCredentials(sslOpt);

		auto channel = grpc::CreateCustomChannel(url, creds, gargs);

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
		for (auto ev : m_data)
		{
			grpc::ClientContext ctx;
			externalscripting::EventResponse response;
			auto status = m_stub->TriggerEvent(&ctx, ev, &response);
			if (!status.ok())
			{
				trace("Unable to send RPC data to %s : %s\n\n%s\n", m_url, status.error_message(), status.error_details());
				break;
			}
		}

		m_data.clear();
	}

private:

	std::string utf8Encode(const std::wstring& wstr)
	{
		if (wstr.empty())
			return std::string();

		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
			NULL, 0, NULL, NULL);
		std::string strTo(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
			&strTo[0], sizeNeeded, NULL, NULL);
		return strTo;
	}

	grpc::SslCredentialsOptions getSslOptions()
	{
		// Fetch root certificate as required on Windows (s. issue 25533).
		grpc::SslCredentialsOptions result;

		// Open root certificate store.
		HANDLE hRootCertStore = CertOpenSystemStoreW(NULL, L"ROOT");
		if (!hRootCertStore)
			return result;

		// Get all root certificates.
		PCCERT_CONTEXT pCert = NULL;
		while ((pCert = CertEnumCertificatesInStore(hRootCertStore, pCert)) != NULL)
		{
			// Append this certificate in PEM formatted data.
			DWORD size = 0;
			CryptBinaryToStringW(pCert->pbCertEncoded, pCert->cbCertEncoded,
				CRYPT_STRING_BASE64HEADER, NULL, &size);
			std::vector<WCHAR> pem(size);
			CryptBinaryToStringW(pCert->pbCertEncoded, pCert->cbCertEncoded,
				CRYPT_STRING_BASE64HEADER, pem.data(), &size);

			result.pem_root_certs += utf8Encode(pem.data());
		}

		CertCloseStore(hRootCertStore, 0);
		return result;
	}

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
				for (GrpcEndpoint* ep : g_grpcEndpoints)
				{
					ep->SendEvents();
				}
		});
	});
});
