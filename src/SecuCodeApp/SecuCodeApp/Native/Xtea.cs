using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Runtime.InteropServices;

namespace SecuCodeApp.Native
{
    public class Xtea
    {
        [DllImport("xtea.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "XTEAenc")]
        internal static extern void XteaEncInner(IntPtr data, IntPtr key, Int32 num_rounds);

        public static byte[] XteaEncBlock(UInt32[] data, UInt32[] key, int num_rounds)
        {
            var dataInt = data.Select(x => (Int32)x).ToArray();
            var keyInt = key.Select(x => (Int32)x).ToArray();

            var dataPtr = Marshal.AllocHGlobal(Marshal.SizeOf(dataInt[0]) * dataInt.Length);
            var keyPtr = Marshal.AllocHGlobal(Marshal.SizeOf(keyInt[0]) * keyInt.Length);

            try
            {
                Marshal.Copy(dataInt, 0, dataPtr, data.Length);
                Marshal.Copy(keyInt, 0, keyPtr, key.Length);

                XteaEncInner(dataPtr, keyPtr, num_rounds);

                Marshal.Copy(dataPtr, dataInt, 0, data.Length);
            }
            finally
            {
                Marshal.FreeHGlobal(dataPtr);
                Marshal.FreeHGlobal(keyPtr);
            }

            return dataInt.ToByteArray();
        }

        public static byte[] XteaEncAll(byte[] data, byte[] key)
        {
            var blockKey = key.ToUint32Array();
            return data.Split(8)
                .Select(block => XteaEncBlock(block.ToArray().ToUint32Array(), blockKey, 64))
                .SelectMany(block => block)
                .ToArray();
        }

        [DllImport("xtea.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "XTEAdec")]
        internal static extern void XteaDecInner(IntPtr data, IntPtr key, Int32 num_rounds);

        public static byte[] XteaDecBlock(UInt32[] data, UInt32[] key, int num_rounds)
        {
            var dataInt = data.Select(x => (Int32)x).ToArray();
            var keyInt = key.Select(x => (Int32)x).ToArray();

            var dataPtr = Marshal.AllocHGlobal(Marshal.SizeOf(dataInt[0]) * dataInt.Length);
            var keyPtr = Marshal.AllocHGlobal(Marshal.SizeOf(keyInt[0]) * keyInt.Length);

            try
            {
                Marshal.Copy(dataInt, 0, dataPtr, data.Length);
                Marshal.Copy(keyInt, 0, keyPtr, key.Length);

                XteaDecInner(dataPtr, keyPtr, num_rounds);

                Marshal.Copy(dataPtr, dataInt, 0, data.Length);
            }
            finally
            {
                Marshal.FreeHGlobal(dataPtr);
                Marshal.FreeHGlobal(keyPtr);
            }

            return dataInt.ToByteArray();
        }

        public static byte[] XteaDecAll(byte[] data, byte[] key)
        {
            var blockKey = key.ToUint32Array();
            return data.Split(8)
                .Select(block => XteaDecBlock(block.ToArray().ToUint32Array(), blockKey, 64))
                .SelectMany(block => block)
                .ToArray();
        }
    }
}
