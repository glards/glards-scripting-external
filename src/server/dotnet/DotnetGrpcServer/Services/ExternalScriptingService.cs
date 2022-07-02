using Externalscripting;
using Grpc.Core;

namespace DotnetGrpcServer.Services
{
    public class ExternalScriptingService : Externalscripting.ExternalScripting.ExternalScriptingBase
    {
        public override Task<EventResponse> TriggerEvent(EventData request, ServerCallContext context)
        {
            Console.WriteLine($"> Received event : {request.EventName}");
            return Task.FromResult<EventResponse>(new EventResponse());
        }
    }
}
