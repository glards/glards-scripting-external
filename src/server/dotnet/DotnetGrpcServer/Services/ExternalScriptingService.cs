using System.Diagnostics;
using Externalscripting;
using Grpc.Core;

namespace DotnetGrpcServer.Services
{
    public class ExternalScriptingService : Externalscripting.ExternalScripting.ExternalScriptingBase
    {
        private static Stopwatch s_stopwatch = new Stopwatch();

        public override Task<EventResponse> TriggerEvent(EventData request, ServerCallContext context)
        {
            if (request.EventName == "Start")
            {
                s_stopwatch.Restart();
            }
            
            Console.WriteLine($"> Received event : {request.EventName}");
            
            if (request.EventName == "Stop")
            {
                s_stopwatch.Stop();
                Console.WriteLine($"Done -> {s_stopwatch.Elapsed.TotalMilliseconds}");
            }

            return Task.FromResult<EventResponse>(new EventResponse());
        }
    }
}
