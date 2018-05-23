using System.IO;
using Xunit;

namespace IdpProtocol.Tests
{
    public class IdpParserTests
    {
        [Fact]
        public void Can_Parse_Frame_With_1Byte_Payload_NoCRC()
        {
            var incomingData = new MemoryStream();

            var parser = new IdpParser(incomingData);

            var data = new byte[] { 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0xAA, 0x03 };

            incomingData.Write(data, 0, data.Length);

            incomingData.Seek(0, SeekOrigin.Begin);

            byte[] payload = null;

            parser.PacketParsed += (sender, e) =>
            {
                payload = e.Payload;
            };

            parser.Parse();

            Assert.Equal(0xaa, payload[0]);
        }
    }
}
