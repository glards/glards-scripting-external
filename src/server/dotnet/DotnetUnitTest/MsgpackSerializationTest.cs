using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MessagePack;
using Xunit;

namespace DotnetUnitTest
{
    public class MsgpackSerializationTest
    {
        [Fact]
        public void UnpackPingEvent()
        {
            string eventPayload = "kc0hJA==";

            var data = Convert.FromBase64String(eventPayload);

            MessagePackReader reader = new MessagePackReader(data);

            Assert.Equal(MessagePackType.Array, reader.NextMessagePackType);

            int cnt = reader.ReadArrayHeader();
            Assert.Equal(1, cnt);

            Assert.Equal(MessagePackType.Integer, reader.NextMessagePackType);
            var timestamp = reader.ReadInt64();
            Assert.Equal(8484, timestamp);

            Assert.True(reader.End);
        }

        [Fact]
        public void UnpackerPingEvent()
        {
            string eventPayload = "kc0hJA==";

            var data = Convert.FromBase64String(eventPayload);

            var o = DotnetGrpcServer.Utils.MessagePack.Unpack(data);

            Assert.NotNull(o);
            var ts = Assert.Single(o);
            Assert.Equal((long)8484, ts);
        }

        [Fact]
        public void RoundTripTest()
        {
            string eventPayload = "kc0hJA==";

            var data = Convert.FromBase64String(eventPayload);

            var o = DotnetGrpcServer.Utils.MessagePack.Unpack(data);

            var dataRepack = DotnetGrpcServer.Utils.MessagePack.Pack(o);

            string eventPayloadRepack = Convert.ToBase64String(dataRepack);

            Assert.Equal(eventPayload, eventPayloadRepack);
        }
    }
}
