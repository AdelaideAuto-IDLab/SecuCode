using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace SecuCodeApp.Native
{
    public class Bch
    {
        public static (int errors, byte[] key) DecodeKey(byte challenge, byte[] helperData)
        {
            var key = new byte[16];
            var errors = key.Length;

            var helperDataPtr = Marshal.AllocHGlobal(Marshal.SizeOf(helperData[0]) * helperData.Length);
            var keyPtr = Marshal.AllocHGlobal(Marshal.SizeOf(key[0]) * key.Length);

            try
            {
                Marshal.Copy(helperData, 0, helperDataPtr, helperData.Length);
                errors = DecodeKeyInternal(challenge, helperDataPtr, keyPtr);

                Marshal.Copy(keyPtr, key, 0, key.Length);
            }
            finally
            {
                Marshal.FreeHGlobal(helperDataPtr);
                Marshal.FreeHGlobal(keyPtr);
            }

            return (errors, key);
        }

        [DllImport("bch.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "decodeKey")]
        extern static int DecodeKeyInternal(byte challenge, IntPtr helperData, IntPtr decodedKey);
    }
}
