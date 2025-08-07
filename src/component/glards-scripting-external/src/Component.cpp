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


class GrpcEndpoint : public grpc::ClientBidiReactor<externalscripting::EventData, externalscripting::ClientEventData>
{
public:
	GrpcEndpoint(const std::string url)
	{
		url_ = url;

		grpc::ChannelArguments gargs;

		auto sslOpt = getSslOptions();
		auto creds = grpc::SslCredentials(sslOpt);

		auto channel = grpc::CreateCustomChannel(url, creds, gargs);

		stub_ = externalscripting::ExternalScripting::NewStub(channel);
		data_ = std::vector<externalscripting::EventData>();

		stub_->async()->EventStream(&context_, this);


	}

	void AddEvent(std::string name, std::string payload, std::string source)
	{
		auto data = externalscripting::EventData();
		data.set_eventname(name);
		data.set_eventpayload(payload);
		data.set_eventsource(source);
		data_.emplace_back(std::move(data));
	}

	//void SendEvents()
	//{
	//	for (auto ev : data_)
	//	{
	//		grpc::ClientContext ctx;
	//		externalscripting::EventResponse response;
	//		auto status = stub_->TriggerEvent(&ctx, ev, &response);
	//		if (!status.ok())
	//		{
	//			trace("Unable to send RPC data to %s : %s\n\n%s\n", url_, status.error_message(), status.error_details());
	//			break;
	//		}
	//	}

	//	data_.clear();
	//}

	void SetEventManager(const fwRefContainer<fx::ResourceEventManagerComponent> eventManager)
	{
		eventManager_ = eventManager;
	}

	void OnDone(const grpc::Status&) override
	{
		
	}

	void OnReadDone(bool) override
	{
		
	}

	void OnWriteDone(bool) override
	{
		
	}

	void OnWritesDoneDone(bool) override
	{
		
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
	fwRefContainer<fx::ResourceEventManagerComponent> eventManager_ = nullptr;
	grpc::ClientContext context_;
	std::vector<externalscripting::EventData> data_;
	std::string url_;
	std::unique_ptr<externalscripting::ExternalScripting::Stub> stub_;

	std::mutex mutex_;
	std::condition_variable cv_;
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

		trace("OnInitializeInstance");

		for (GrpcEndpoint* ep : g_grpcEndpoints)
		{
			ep->SetEventManager(eventComponent);
		}

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
				//for (GrpcEndpoint* ep : g_grpcEndpoints)
				//{
				//	ep->SendEvents();
				//}
		});
	});
});
