using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SecuCodeApp.Native
{
    public static class Extensions
    {
        public static UInt32[] ToUint32Array(this byte[] bytes)
        {
            var result = new UInt32[bytes.Length / 4];
            Buffer.BlockCopy(bytes, 0, result, 0, bytes.Length);
            return result;
        }

        public static byte[] ToByteArray(this UInt32[] words)
        {
            var bytes = new byte[words.Length * 4];
            Buffer.BlockCopy(words, 0, bytes, 0, bytes.Length);
            return bytes;
        }

        public static byte[] ToByteArray(this Int32[] words)
        {
            var bytes = new byte[words.Length * 4];
            Buffer.BlockCopy(words, 0, bytes, 0, bytes.Length);
            return bytes;
        }
    }
}
