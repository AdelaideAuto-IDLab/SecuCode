using System;

using System.Runtime.InteropServices;

namespace SecuCodeApp.Native
{
    public class AesCmac
    {
        [DllImport("aes-cmac.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "generate_AES_CMAC")]
        private static extern void generate_AES_CMAC(
            IntPtr key,
            IntPtr nonce,
            IntPtr data,
            Int16 data_len,
            IntPtr mac_out);

        public static byte[] GenerateAesCmac(byte[] key, byte[] nonce, byte[] data)
        {
            if (key.Length != 16 || nonce.Length != 16)
            {
                throw new Exception("Bad key or nonce length");
            }

            var macOut = new byte[16];

            var keyPtr = Marshal.AllocHGlobal(Marshal.SizeOf(key[0]) * key.Length);
            var noncePtr = Marshal.AllocHGlobal(Marshal.SizeOf(nonce[0]) * nonce.Length);
            var dataPtr = Marshal.AllocHGlobal(Marshal.SizeOf(data[0]) * data.Length);
            var macOutPtr = Marshal.AllocHGlobal(Marshal.SizeOf(macOut[0]) * macOut.Length);

            try
            {
                Marshal.Copy(key, 0, keyPtr, key.Length);
                Marshal.Copy(nonce, 0, noncePtr, nonce.Length);
                Marshal.Copy(data, 0, dataPtr, data.Length);
                Marshal.Copy(macOut, 0, macOutPtr, macOut.Length);
                generate_AES_CMAC(keyPtr, noncePtr, dataPtr, (short)data.Length, macOutPtr);

                Marshal.Copy(macOutPtr, macOut, 0, macOut.Length);
            }
            finally
            {
                Marshal.FreeHGlobal(keyPtr);
                Marshal.FreeHGlobal(noncePtr);
                Marshal.FreeHGlobal(dataPtr);
                Marshal.FreeHGlobal(macOutPtr);
            }

            return macOut;
        }
    }
}
