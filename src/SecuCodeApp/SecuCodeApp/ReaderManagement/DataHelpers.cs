using Org.LLRP.LTK.LLRPV1;
using Org.LLRP.LTK.LLRPV1.DataType;
using System.Collections.Generic;
using System.Linq;

namespace SecuCodeApp
{
    public static class DataHelpers
    {
        /// <summary>
        /// Extracts the raw bytes from an EPC field of any format.
        /// </summary>
        public static byte[] GetEpc(IParameter epcField)
        {
            switch (epcField)
            {
                case PARAM_EPC_96 epc:
                    return epc.EPC.ToBytes();
                case PARAM_EPCData epc:
                    return epc.EPC.ToBytes();
                default:
                    return new byte[0] { };
            }
        }

        /// <summary>
        /// Converts an internal LTK parameter list to a C# list.
        /// </summary>
        public static List<IParameter> ToList(this UNION_AccessCommandOpSpecResult data)
        {
            var output = new List<IParameter>();
            for (var i = 0; i < data.Count; ++i)
            {
                output.Add(data[i]);
            }
            return output;
        }

        /// <summary>
        /// Converts a UInt16Array of words stored in MSP430 native endiness to a byte array.
        /// </summary>
        public static byte[] Msp430WordsToBytes(UInt16Array words)
        {
            var bytes = new byte[words.Count * 2];
            for (var i = 0; i < words.Count; ++i)
            {
                bytes[i * 2] = (byte)((words[i] & 0xFF00) >> 8);
                bytes[i * 2 + 1] = (byte)(words[i] & 0x00FF);
            }
            return bytes;
        }

        /// <summary>
        /// Converts a LLRPBit array into a "packed" byte array
        /// (e.g. the bit array [1, 0, 0, 1, 1] is converted to [0b00010011])
        /// </summary>
        public static byte[] ToBytes(this LLRPBitArray array)
        {
            var bytes = new List<byte>();

            var byteOffset = 0;
            byte currentByte = 0;

            for (var i = 0; i < array.Count; ++i)
            {
                currentByte <<= 1;
                if (array[i])
                {
                    currentByte |= 1;
                }
                byteOffset += 1;
                if (byteOffset == 8)
                {
                    bytes.Add(currentByte);
                    byteOffset = 0;
                    currentByte = 0;
                }
            }

            bytes.Add(currentByte);
            return bytes.ToArray();
        }

        /// <summary>
        /// Formats an iterator of bytes as a string with an optimal prefix and seperator.
        /// </summary>
        public static string ToByteString(this IEnumerable<byte> bytes, string prefix = "", string separator = "")
        {
            return string.Join(separator, bytes.Select(b => string.Format("{0}{1:X2}", prefix, b)));
        }

        /// <summary>
        /// Splits an array into an iterator of iterators, where each of the inner iterators
        /// returns up to `size` items.
        /// </summary>
        public static IEnumerable<IEnumerable<T>> Split<T>(this T[] array, int size)
        {
            var segments = array.Length / size;

            for (var i = 0; i < segments; i++)
            {
                yield return array.Skip(i * size).Take(size);
            }

            var finalSegment = array.Length - segments * size;
            if (finalSegment > 0)
            {
                yield return array.Skip(array.Length - finalSegment).Take(finalSegment);
            }
        }
    }
}
