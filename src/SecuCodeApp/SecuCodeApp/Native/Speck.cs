using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace SecuCodeApp.Native
{
    public class Speck
    {
        public static byte[] Hash(UInt64 nonce, byte[] data)
        {
            var state = new byte[8];

            var dataPtr = Marshal.AllocHGlobal(Marshal.SizeOf(data[0]) * data.Length);
            var statePtr = Marshal.AllocHGlobal(Marshal.SizeOf(state[0]) * state.Length);

            try
            {
                Marshal.Copy(data, 0, dataPtr, data.Length);
                Marshal.Copy(state, 0, statePtr, state.Length);
                HashInternal(nonce, dataPtr, (UInt16)data.Length, statePtr);

                Marshal.Copy(statePtr, state, 0, state.Length);
            }
            finally
            {
                Marshal.FreeHGlobal(dataPtr);
                Marshal.FreeHGlobal(statePtr);
            }

            return state;
        }

        [DllImport("hash.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "HASH_SPECK128")]
        extern static void HashInternal(UInt64 nonce, IntPtr data, UInt16 size, IntPtr state);
    }
}
