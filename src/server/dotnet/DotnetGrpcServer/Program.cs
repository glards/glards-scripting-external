using System.Security.Authentication;
using DotnetGrpcServer.Services;
using Microsoft.AspNetCore.HttpLogging;

var builder = WebApplication.CreateBuilder(args);

//builder.WebHost.ConfigureKestrel(kestrel =>
//{
//    kestrel.ConfigureHttpsDefaults(https =>
//    {
//        https.SslProtocols = SslProtocols.Tls12;
//        https.AllowAnyClientCertificate();
//    });
//});



// Additional configuration is required to successfully run gRPC on macOS.
// For instructions on how to configure Kestrel and gRPC clients on macOS, visit https://go.microsoft.com/fwlink/?linkid=2099682

// Add services to the container.
//builder.Services.AddHttpLogging(opt =>
//{
//    opt.LoggingFields = HttpLoggingFields.Request;
//});
builder.Services.AddGrpc();

var app = builder.Build();
//app.UseHttpLogging();
// Configure the HTTP request pipeline.
app.MapGrpcService<ExternalScriptingService>();
app.MapGet("/", () => "Communication with gRPC endpoints must be made through a gRPC client. To learn how to create a client, visit: https://go.microsoft.com/fwlink/?linkid=2086909");

app.Run();
