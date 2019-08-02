using System;
using System.Collections.Generic;
using System.Linq;

namespace SecuCodeApp
{
    /// <summary>
    /// Implements functions for encoding a program into MSPBoot commands
    /// See: http://www.ti.com/tool/mspbsl for more details
    /// </summary>
    public class MspBoot
    {
        const byte HEADER = 0x80;
        const byte MAX_PAYLOAD = 254;

        enum Commands
        {
            ERASE_SEGMENT = 0x12,
            ERASE_APP = 0x15,
            RX_DATA_BLOCK = 0x10,
            TX_VERSION = 0x19,
            JUMP2APP = 0x1C
        }

        /// <summary>
        /// Encodes a program into MSPBoot commands
        /// </summary>
        public static byte[] EncodeProgram(MspFirmware program, bool appendRunCommand)
        {
            var bytes = new List<byte>();
            foreach (var (address, data) in program.Segments)
            {
                EncodeSegment(address, data, bytes);
            }

            if (appendRunCommand)
            {
                bytes.Add(HEADER);
                bytes.Add(1);
                bytes.Add((byte)Commands.JUMP2APP);
            }

            return bytes.ToArray();
        }

        private static void EncodeSegment(UInt32 address, byte[] data, List<byte> output)
        {
            // Packet format: [Header (1), Length (1), RX_DATA_BLOCK (1), Addr (3), Data (?)]
            // Note: Checksum is disabled since we validate the packets integrity at a higher level
            foreach (var chunk in data.Split(MAX_PAYLOAD - 4))
            {
                output.Add(HEADER);
                output.Add((byte)(chunk.Count() + 4));
                output.Add((byte)Commands.RX_DATA_BLOCK);

                // Encode the target address in our packet payload
                // Note: address is 20-bits only so the high byte is masked with 0x0F
                output.Add((byte)((address >> 0) & 0xFF));
                output.Add((byte)((address >> 8) & 0xFF));
                output.Add((byte)((address >> 16) & 0x0F));

                output.AddRange(chunk);
                address += (UInt32)(chunk.Count());
            }
        }
    }
}
