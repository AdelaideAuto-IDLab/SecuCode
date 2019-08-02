using System;

using System.Runtime.InteropServices;

namespace SecuCodeApp.Native
{
    public class Crc64
    {
        [DllImport("crc64.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "gen_crc64")]
        private static extern UInt64 gen_crc64(
            UInt64 crc,
            IntPtr data,
            UInt64 data_len);

        public static byte[] GenerateCrc64(byte[] data)
        {
            var dataPtr = Marshal.AllocHGlobal(Marshal.SizeOf(data[0]) * data.Length);
            ulong crc64 = 0;

            try
            {
                Marshal.Copy(data, 0, dataPtr, data.Length);
                crc64 = gen_crc64(crc64, dataPtr, (UInt64)data.Length);
            }
            finally
            {
                Marshal.FreeHGlobal(dataPtr);
            }

            var crc = BitConverter.GetBytes(crc64);
            if (!BitConverter.IsLittleEndian)
            {
                Array.Reverse(crc);
            }

            return crc;
        }
    }
}
