using System.Diagnostics;
using Externalscripting;
using Google.Protobuf;
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

            if (!request.EventName.StartsWith("gl"))
            {
                return Task.FromResult<EventResponse>(new EventResponse());
            }
            
            Console.WriteLine($"> Received event {request.EventName} from '{request.EventSource}' : {request.EventPayload.ToBase64()}");


            if (request.EventName == "Stop")
            {
                s_stopwatch.Stop();
                Console.WriteLine($"Done -> {s_stopwatch.Elapsed.TotalMilliseconds}");
            }

            return Task.FromResult<EventResponse>(new EventResponse());
        }

        public override async Task EventStream(IAsyncStreamReader<EventData> requestStream, IServerStreamWriter<ClientEventData> responseStream, ServerCallContext context)
        {
            await foreach (var ev in requestStream.ReadAllAsync())
            {
                if (ev.EventName.Equals("gl_admin:ping"))
                {
                    int source = ParseSource(ev.EventSource);

                    var payload = Utils.MessagePack.Unpack(ev.EventPayload.ToByteArray());

                    ClientEventData clientEvent = new ClientEventData();
                    clientEvent.Destination = source;
                    clientEvent.EventPayload = ByteString.CopyFrom(Utils.MessagePack.Pack(payload[0], payload[0]));
                    clientEvent.EventName = "gl_admin:pong";

                    await responseStream.WriteAsync(clientEvent);
                }
            }
        }

        private static int ParseSource(string src)
        {
            int source = -1;
            string? idx = null;
            if (src.StartsWith("net:"))
            {
                idx = src.Substring("net:".Length);
            } else if (src.StartsWith("internal-net:"))
            {
                idx = src.Substring("internal-net:".Length);
            }

            if (idx != null)
            {
                Int32.TryParse(idx, out source);
            }

            return source;
        }
    }
}
