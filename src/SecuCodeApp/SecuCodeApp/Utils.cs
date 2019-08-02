using Org.LLRP.LTK.LLRPV1;
using Org.LLRP.LTK.LLRPV1.DataType;

namespace SecuCodeApp
{
    public class Utils
    {
        public static string MaskEpc(string baseEpc)
        {
            return baseEpc.Substring(0, 2) + ".................." + baseEpc.Substring(2 * 9);
        }

        public static string GetEpcString(IParameter epcField)
        {
            switch (epcField)
            {
                case PARAM_EPC_96 epc:
                    return epc.EPC.ToHexString();

                case PARAM_EPCData epc:
                    return epc.EPC.ToHexString();

                default:
                    return "Unknown";
            }
        }

        public static byte[] GetEpcBytes(IParameter epcField)
        {
            switch (epcField)
            {
                case PARAM_EPC_96 epc:
                    return epc.EPC.ToBytes();

                case PARAM_EPCData epc:
                    return epc.EPC.ToBytes();

                default:
                    return null;
            }
        }
    }

    /// <summary>
    /// Helper class manager a static mean counter in a thread safe manner.
    /// </summary>
    public class MeanTracker
    {
        private readonly object _lock = new object();

        double sum = 0.0;
        double count = 0;

        double prev = 0.0;

        public double Next(double value)
        {
            lock (this._lock)
            {
                if (this.count == 0)
                {
                    this.sum = value;
                }

                this.sum += value;
                this.count += 1;

                return this.sum / this.count;
            }
        }

        public double Reset()
        {
            lock (this._lock)
            {
                if (this.count == 0)
                {
                    return this.prev;
                }
                this.prev = this.sum / this.count;

                this.sum = 0.0;
                this.count = 0;

                return this.prev;
            }
        }
    }

}
