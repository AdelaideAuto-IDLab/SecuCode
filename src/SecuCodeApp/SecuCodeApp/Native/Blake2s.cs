using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace SecuCodeApp.Native
{
    public class Blake2s
    {
        public static byte[] Hash(byte[] nonce, byte[] data, int outputBytes)
        {
            var output = new byte[outputBytes];

            var dataPtr = Marshal.AllocHGlobal(Marshal.SizeOf(data[0]) * data.Length);
            var outputPtr = Marshal.AllocHGlobal(Marshal.SizeOf(output[0]) * output.Length);

            try
            {
                Marshal.Copy(data, 0, dataPtr, data.Length);
                Marshal.Copy(output, 0, outputPtr, output.Length);
                HashInternal(outputPtr, (IntPtr)output.Length, IntPtr.Zero, IntPtr.Zero, dataPtr, (IntPtr)data.Length);

                Marshal.Copy(outputPtr, output, 0, output.Length);
            }
            finally
            {
                Marshal.FreeHGlobal(dataPtr);
                Marshal.FreeHGlobal(outputPtr);
            }

            return output;
        }

        [DllImport("hash.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "blake2s")]
        extern static void HashInternal(IntPtr @out, IntPtr outlen, IntPtr key, IntPtr keylen, IntPtr @in, IntPtr inlen);
    }
}
