using Org.LLRP.LTK.LLRPV1;
using Org.LLRP.LTK.LLRPV1.DataType;

namespace SecuCodeApp
{
    public static class LLRPHelpers
    {
        public static PARAM_C1G2TargetTag WispTag(string id)
        {
            return new PARAM_C1G2TargetTag()
            {
                Match = true,
                MB = new TwoBits(1),
                Pointer = 0x20,

                // TODO: Support looking at more than the first two bytes like we do in SecuCodeBatch
                TagMask = LLRPBitArray.FromHexString("FF"),
                TagData = LLRPBitArray.FromHexString(id)
            };
        }

        public static PARAM_C1G2BlockWrite ControlMessage(ushort state, ushort value)
        {
            var data = new UInt16Array();
            data.Add(value);

            return new PARAM_C1G2BlockWrite()
            {
                MB = new TwoBits(0),
                OpSpecID = 111,
                WordPointer = state,
                WriteData = data,
                AccessPassword = 0,
            };
        }

        public static PARAM_C1G2BlockWrite DataMessage(UInt16Array data, ushort offset, ushort opSpecId = 111)
        {
            return new PARAM_C1G2BlockWrite()
            {
                MB = new TwoBits(3),
                OpSpecID = opSpecId,
                WordPointer = offset,
                WriteData = data,
                AccessPassword = 0,
            };
        }

        public static PARAM_C1G2Read ReadMessage(ushort memoryBank, ushort address, ushort length)
        {
            return new PARAM_C1G2Read()
            {
                MB = new TwoBits(memoryBank),
                OpSpecID = 111,
                WordPointer = address,
                WordCount = length,
                AccessPassword = 0
            };
        }
    }

    public class Result<T> where T : Message
    {
        private MSG_ERROR_MESSAGE error;
        private readonly T response;

        public Result(T response, MSG_ERROR_MESSAGE error)
        {
            this.response = response;
            this.error = error;
        }

        public override string ToString()
        {
            if (this.response != null)
            {
                return this.response.ToString();
            }
            else if (this.error != null)
            {
                return this.error.ToString();
            }

            return "Timeout Error.";
        }
    }
}
