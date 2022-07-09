using System.Buffers;
using System.Text;
using MessagePack;

namespace DotnetGrpcServer.Utils
{
    public static class MessagePack
    {
        public static object[] Unpack(byte[] data)
        {
            MessagePackReader reader = new MessagePackReader(data);

            if (reader.NextMessagePackType != MessagePackType.Array)
            {
                return Array.Empty<object>();
            }

            if (!reader.TryReadArrayHeader(out int count))
            {
                return Array.Empty<object>();
            }

            object[] result = new object[count];

            for (int i = 0; i < count; i++)
            {
                switch (reader.NextMessagePackType)
                {
                    case MessagePackType.Unknown:
                        return Array.Empty<object>();
                    case MessagePackType.Integer:
                        result[i] = reader.ReadInt64();
                        break;
                    case MessagePackType.Nil:
                        return Array.Empty<object>();
                    case MessagePackType.Boolean:
                        result[i] = reader.ReadBoolean();
                        break;
                    case MessagePackType.Float:
                        result[i] = reader.ReadDouble();
                        break;
                    case MessagePackType.String:
                        result[i] = reader.ReadString();
                        break;
                    case MessagePackType.Binary:
                        return Array.Empty<object>();
                    case MessagePackType.Array:
                        return Array.Empty<object>();
                    case MessagePackType.Map:
                        return Array.Empty<object>();
                    case MessagePackType.Extension:
                        return Array.Empty<object>();
                    default:
                        return Array.Empty<object>();
                }
            }

            return result;
        }

        public static byte[] Pack(params object[] objects)
        {
            ArrayBufferWriter<byte> buffer = new ArrayBufferWriter<byte>();

            MessagePackWriter writer = new MessagePackWriter(buffer);

            writer.WriteArrayHeader(objects.Length);

            foreach (var o in objects)
            {
                if (o is Int16 i16)
                {
                    writer.Write(i16);
                } else if (o is Int32 i32)
                {
                    writer.Write(i32);
                } else if (o is Int64 i64)
                {
                    writer.Write(i64);
                }
                else if (o is UInt16 ui16)
                {
                    writer.Write(ui16);
                }
                else if (o is UInt32 ui32)
                {
                    writer.Write(ui32);
                }
                else if (o is UInt64 ui64)
                {
                    writer.Write(ui64);
                } else if (o is string s)
                {
                    writer.WriteString(Encoding.UTF8.GetBytes(s));
                } else if (o is float f)
                {
                    writer.Write((double)f);
                } else if (o is double d)
                {
                    writer.Write(d);
                }
            }

            writer.Flush();

            return buffer.WrittenMemory.ToArray();
        }
    }
}
