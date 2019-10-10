#define NO_OFFSET_MESSAGE

using Org.LLRP.LTK.LLRPV1;
using Org.LLRP.LTK.LLRPV1.DataType;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace SecuCodeApp
{
    public enum TagState : ushort
    {
        None = 0x00,

        SetOffset = 0x01,
        Done = 0x02,

        Authenticate = 0x03,

        PerformHash = 0x04,

        RestartInBootMode = 0x7F,
    }


    public class OperationResponse
    {
        public SecuCode.StatusCode status;

        public static OperationResponse Success = new OperationResponse { status = SecuCode.StatusCode.Success };
        public static OperationResponse UnknownError = new OperationResponse { status = SecuCode.StatusCode.UnknownError };

        public bool IsSuccess()
        {
            return this.status == SecuCode.StatusCode.Success;
        }
    }

    /// This class implements helper functions for communicating with an Impinj RFID reader.
    ///
    /// Note: This implementation isn't robust (or performant), particularly in the way it checks
    /// for state changes in the tag. This is partially due to the underlying library (LTK) and
    /// partially due to how the implementation evolved over time.
    public class ReaderManager
    {
        public LLRPClient reader;

        private int nextAccessCmdId = 1;

        private CancellationTokenSource connectionCancellationSource;

        /// <summary>
        /// Attempts to connect to an RFID reader using LLRP.
        /// </summary>
        /// <param name="address">The name / address of the reader to connect to</param>
        /// <returns>Success, or why the connection attempt failed</returns>
        public ENUM_ConnectionAttemptStatusType Connect(string address)
        {
            if (this.connectionCancellationSource != null)
            {
                this.connectionCancellationSource.Cancel();
            }

            this.reader = new LLRPClient();
            this.reader.Open(address, 2000, out var status);
            this.reader.Enable_ImpinjExtensions();
            this.reader.Enable_EventsAndReports();

            this.nextAccessCmdId = 1;

            this.connectionCancellationSource = new CancellationTokenSource();

            return status;
        }

        /// <summary>
        /// Returns whether the ReaderManager is currently connected to a reader.
        /// Note: even if this call returns true, sending a command may fail.
        /// </summary>
        public bool IsConnected()
        {
            return this.reader != null && this.reader.IsConnected;
        }

        /// <summary>
        /// Disconnects from the reader cancelling any pending tasks
        /// </summary>
        public void Disconnect()
        {
            if (this.IsConnected())
            {
                this.connectionCancellationSource.Cancel();
                this.ResetState();
            }

            if (this.reader != null)
            {
                this.reader.Dispose();
                this.reader = null;
            }
        }


        /// <summary>
        /// Throws an exception if the reader is not currently connected
        /// </summary>
        private void EnsureConnected()
        {
            if (!this.IsConnected())
            {
                throw new Exception("Not connected");
            }
        }

        /// <summary>
        /// Attempts to reset the reader's state by clearing any active AccessSpecs and RoSpecs.
        /// </summary>
        public void ResetState()
        {
            this.EnsureConnected();
            this.reader.Delete_AccessSpec();
            this.reader.Delete_RoSpec(1);
        }

        /// <summary>
        /// Sends a `Read` command to a tag (with EPC starting with `tagId`) and waits for the result
        /// </summary>
        public Task<(OperationResponse response, byte[] payload)>
            Read(string tagId, int memoryBank, int address, int length, TimeSpan? timeout = null)
        {
            this.EnsureConnected();
            this.AddReadAccessSpec(tagId, (ushort)memoryBank, (ushort)address, (ushort)length);
            return this.StartAndWaitForBytes(length, timeout);
        }

        /// <summary>
        /// Runs a simple inventory command for a specified `duration`
        /// </summary>
        public async Task Inventory(TimeSpan duration)
        {
            this.EnsureConnected();

            this.reader.Delete_AccessSpec();
            this.reader.Add_RoSpec(TimeSpan.FromSeconds(3.0));
            this.reader.Enable_RoSpec(1);

            await Task.Delay(duration, this.connectionCancellationSource.Token);

            this.reader.Delete_RoSpec(1);
        }

        /// <summary>
        /// Sends a control message to a tag (with EPC starting with `tagId`). The message is encoded a special
        /// 1 word BlockWrite command.
        /// 
        /// Note: this will always return a power failure error if it is the first control message that is sent.
        /// </summary>
        public Task<OperationResponse> SendControlWithPowerCheck(string tagId, TagState state, ushort value, TimeSpan? timeout = null, bool checkPowerFailure = true)
        {
            this.EnsureConnected();

            Console.WriteLine($"Sending control message: state: {state}, value: {value}");
            this.AddControlAccessSpec(tagId, (ushort)state, value);
            return this.StartAndWaitForEpc(epc =>
            {
                if (epc[1] == (ushort)state) { return SecuCode.StatusCode.Success; }
                else if (checkPowerFailure && epc[1] == (ushort)TagState.RestartInBootMode) { return SecuCode.StatusCode.PowerFailure; }
                return null;
            }, timeout);
        }

        /// <summary>
        /// Sends a control message to a tag (with EPC starting with `tagId`). The message is encoded a special
        /// 1 word BlockWrite command.
        /// </summary>
        public Task<OperationResponse> SendControl(string tagId, TagState state, ushort value, ushort prevEpc, TimeSpan? timeout = null)
        {
            this.EnsureConnected();

            Console.WriteLine($"Sending control message: state: {state}, value: {value}");
            this.AddControlAccessSpec(tagId, (ushort)state, value);
            return this.StartAndWaitForEpc(epc =>
            {
                if (epc[1] != prevEpc) { return SecuCode.StatusCode.Success; }
                return null;
            }, timeout);
        }

        /// <summary>
        /// Sends a control message to a tag (with EPC starting with `tagId`). The message is encoded a special
        /// 1 word BlockWrite command.
        /// 
        /// Note: waiting for an ACK is less reliable than the EPC based method above
        /// </summary>
        public Task<OperationResponse> SendControlWaitForAck(string tagId, TagState state, ushort value, TimeSpan? timeout = null)
        {
            this.EnsureConnected();

            Console.WriteLine($"Sending control message: state: {state}, value: {value}");
            var command = this.AddControlAccessSpec(tagId, (ushort)state, value);
            return this.StartAndWaitBlockWriteAcks(new List<PARAM_C1G2BlockWrite> { command });
        }

        /// <summary>
        /// Sends an array of `data` to a tag (with EPC starting with `tagId`) to a specific `address`.
        /// 
        /// This transfer is done by sending BlockWrite commands individually, configuring the reader
        /// to resend on failure before sending the next BlockWrite command.
        /// </summary>
        public async Task<OperationResponse> SendData(string tagId, byte[] data, int bytesPerChunk, ushort address, TimeSpan? timeout = null)
        {
            this.EnsureConnected();

            var stopWatch = new Stopwatch();
            stopWatch.Start();

            var targetTag = LLRPHelpers.WispTag(tagId);
            var offset = address;

            var controlResponse = await this.SendControlWithPowerCheck(tagId, TagState.SetOffset, 0x0000);
            if (controlResponse.status != SecuCode.StatusCode.Success)
            {
                return controlResponse;
            }

            foreach (var chunk in data.Split(bytesPerChunk))
            {
                Console.WriteLine($"Sending chunk ({chunk.Count()} bytes) to offset: {offset}");
                // Previously the WISP firmware only handled BlockWrite commands with a max offset
                // of 0x7F (* 2), this required us to send a special message every 0x7F words to
                // allow writing to additional memory.
                //
                // However we have now updated the firmware to support much larger offsets so 
                // this no longer needed in v0.15 and above.
                /*
                var controlResponse = await SendControl(tagId, TagState.SetOffset, offset);
                if (controlResponse.status != SecuCode.StatusCode.Success)
                {
                    return controlResponse;
                }
                */

                var bytes = chunk.ToArray();
                var sendResponse = await this.SendDataBlock(targetTag, offset, bytes, timeout);
                if (sendResponse.status != SecuCode.StatusCode.Success)
                {
                    return sendResponse;
                }

                offset += (ushort)(bytes.Length / 2);
            }

            stopWatch.Stop();
            Console.WriteLine($"Wrote: {data.Length / 2} words, {data.ToByteString()} in {stopWatch.Elapsed.TotalSeconds} seconds");

            return new OperationResponse { status = SecuCode.StatusCode.Success };
        }

        /// <summary>
        /// Sends the 
        /// </summary>
        private Task<OperationResponse> SendDataBlock(PARAM_C1G2TargetTag targetTag, ushort offset, byte[] dataBlock, TimeSpan? timeout = null)
        {
            var accessSpec = this.BlockWriteCmd(offset, dataBlock);
            this.reader.Add_ContinuousAccessSpec(targetTag, new List<IParameter> { accessSpec });
            return this.StartAndWaitBlockWriteAcks(new List<PARAM_C1G2BlockWrite> { accessSpec }, timeout);
        }

        /// <summary>
        /// Generates a "true" BlockWrite command from an offset and an array of data, taking care to
        /// avoid overlapping with the control messages we generate
        /// </summary>
        private PARAM_C1G2BlockWrite BlockWriteCmd(ushort offset, byte[] dataBlock)
        {
            var words = new UInt16Array();
            for (var i = 0; i < dataBlock.Length; i += 2)
            {
                words.Add((ushort)((dataBlock[i + 1] << 8) | dataBlock[i]));
            }

            var accessSpec = LLRPHelpers.DataMessage(words, offset, (ushort)this.nextAccessCmdId);
            this.nextAccessCmdId += 1;

            return accessSpec;
        }

        /// <summary>
        /// Adds a new access spec for reading a number of bytes from the tag
        /// </summary>
        private void AddReadAccessSpec(string tagId, ushort memoryBank, ushort address, ushort length)
        {
            var accessSpecOp = LLRPHelpers.ReadMessage(memoryBank, address, length);
            var targetTag = LLRPHelpers.WispTag(tagId);

            this.reader.Add_ContinuousAccessSpec(targetTag, new List<IParameter> { accessSpecOp });
        }

        /// <summary>
        /// Adds a new access spec for sending a control message
        /// </summary>
        private PARAM_C1G2BlockWrite AddControlAccessSpec(string tagId, ushort state, ushort value)
        {
            var accessSpecOp = LLRPHelpers.ControlMessage(state, value);
            var targetTag = LLRPHelpers.WispTag(tagId);
            this.reader.Add_ContinuousAccessSpec(targetTag, new List<IParameter> { accessSpecOp });
            return accessSpecOp;
        }

        
        /// <summary>
        /// Starts the currently configured access spec, and waits for the task to end, or a 
        /// certain EPC value to be seen
        /// </summary>
        private async Task<OperationResponse> StartAndWaitForEpc(Func<byte[], SecuCode.StatusCode?> check, TimeSpan? timeout = null)
        {
            this.reader.Enable_AccessSpec();

            this.reader.Add_RoSpec(timeout);
            this.reader.Enable_RoSpec(1);

            var response = await new TagReportTask(this.reader, OnEpc(check), timeout, this.connectionCancellationSource.Token).Task;

            this.reader.Delete_AccessSpec();
            this.reader.Delete_RoSpec(1);

            return response;
        }

        /// <summary>
        /// Start a new task with a list of C1G2BlockWrite commands, waiting for each command to be
        /// acked successfully before returning
        /// </summary>
        private async Task<OperationResponse> StartAndWaitBlockWriteAcks(List<PARAM_C1G2BlockWrite> commands, TimeSpan? timeout = null)
        {
            this.reader.Enable_AccessSpec();

            this.reader.Add_RoSpec(timeout);
            this.reader.Enable_RoSpec(1);

            var response = await new TagReportTask(this.reader, OnBlockWriteAck(commands), timeout, this.connectionCancellationSource.Token).Task;

            this.reader.Delete_AccessSpec();
            this.reader.Delete_RoSpec(1);

            return response;
        }


        /// <summary>
        /// Start a new task with a C1G2Read command, waiting for a certain number of bytes to be 
        /// read before returning
        /// </summary>
        private async Task<(OperationResponse, byte[])> StartAndWaitForBytes(int numBytes, TimeSpan? timeout = null)
        {
            Console.WriteLine(this.reader.Enable_AccessSpec());

            Console.WriteLine(this.reader.Add_RoSpec());
            Console.WriteLine(this.reader.Enable_RoSpec(1));

            byte[] bytes = null;

            var task = new TagReportTask(this.reader, (message) =>
            {
                if (message.TagReportData == null)
                {
                    return null;
                }

                foreach (var data in message.TagReportData)
                {
                    for (var i = 0; i < data.AccessCommandOpSpecResult.Count; ++i)
                    {
                        if (data.AccessCommandOpSpecResult[i] is PARAM_C1G2ReadOpSpecResult result && result.ReadData.Count >= numBytes)
                        {
                            bytes = DataHelpers.Msp430WordsToBytes(result.ReadData);
                            return SecuCode.StatusCode.Success;
                        }
                    }

                    // Check if the EPC has changed to a restart value (indicates power failure)
                    if (data.EPCParameter.Count > 0)
                    {
                        if (DataHelpers.GetEpc(data.EPCParameter[0])[1] == (byte)TagState.RestartInBootMode)
                        {
                            return SecuCode.StatusCode.PowerFailure;
                        }
                    }
                }

                return null;
            }, null, this.connectionCancellationSource.Token);

            var response = await task.Task;

            this.reader.Delete_AccessSpec();
            this.reader.Delete_RoSpec(1);
            return (response, bytes);
        }

        /// <summary>
        /// Helper method for generating a callback that processes BlockWrite commands and determining whether they succeed or not.
        /// </summary>
        private static Func<MSG_RO_ACCESS_REPORT, SecuCode.StatusCode?> OnBlockWriteAck(List<PARAM_C1G2BlockWrite> commands, int maxFailures = -1)
        {
            var failures = 0;
            var lockObject = new object();
            return (MSG_RO_ACCESS_REPORT message) =>
            {
                if (message.TagReportData == null)
                {
                    return null;
                }

                if (message.TagReportData
                    .Where(data => data.EPCParameter.Count > 0)
                    .Select(data => DataHelpers.GetEpc(data.EPCParameter[0]))
                    .Where(data => data.Length > 10)
                    .Any(epc => epc[1] == (ushort)TagState.RestartInBootMode))
                {
                    return SecuCode.StatusCode.PowerFailure;
                }

                var results = message.TagReportData
                    .SelectMany(data => data.AccessCommandOpSpecResult.ToList())
                    .Select(data => data as PARAM_C1G2BlockWriteOpSpecResult)
                    .Where(result => result != null);

                // We need to lock here since we are manipulating the commands object and this 
                // callback could be called multiple times while it is still processing the data.
                //
                // Even with the lock, I'm still unsure whether this is fully safe since the
                // LTK library may also use the command buffer at the same time.
                //
                // TODO: Write our own LLRP library?
                lock (lockObject)
                {
                    foreach (var result in results)
                    {
                        var command = commands.Find(cmd => cmd.OpSpecID == result.OpSpecID);
                        if (command == null)
                        {
                            continue;
                        }

                        if (result.Result == ENUM_C1G2BlockWriteResultType.Success && result.NumWordsWritten == command.WriteData.Count)
                        {
                            Console.WriteLine($"Successfully sent: {command.OpSpecID}");
                            commands.Remove(command);
                        }

                        failures += 1;
                        if (maxFailures > 0 && failures > maxFailures)
                        {
                            return SecuCode.StatusCode.FailureCountExceeded;
                        }
                    }

                    if (commands.Count == 0)
                    {
                        return SecuCode.StatusCode.Success;
                    }
                }

                return null;
            };
        }

        static Func<MSG_RO_ACCESS_REPORT, SecuCode.StatusCode?> OnEpc(Func<byte[], SecuCode.StatusCode?> epcCheck)
        {
            return (MSG_RO_ACCESS_REPORT message) =>
            {
                if (message.TagReportData == null)
                {
                    return null;
                }

                var tags = message.TagReportData
                    .Where(data => data.EPCParameter.Count > 0)
                    .Select(data => DataHelpers.GetEpc(data.EPCParameter[0]))
                    .Where(data => data.Length > 10);

                foreach (var tag in tags)
                {
                    var check = epcCheck(tag);
                    if (check != null)
                    {
                        Console.WriteLine("------------- Tag state changed ------------");
                        Console.WriteLine(tag.ToByteString());
                        Console.WriteLine(check);
                        return check;
                    }
                }
                return null;
            };
        }
    }

    /// <summary>
    /// Wrapper class for making it easier to write callbacks that process access reports.
    /// </summary>
    class TagReportTask
    {
        private readonly Func<MSG_RO_ACCESS_REPORT, SecuCode.StatusCode?> onAccessReport;
        private readonly TaskCompletionSource<OperationResponse> taskCompletionSource = new TaskCompletionSource<OperationResponse>();
        private readonly LLRPClient client;

        public Task<OperationResponse> Task { get { return this.taskCompletionSource.Task; } }

        private CancellationTokenSource timerCancellationSource;
        private CancellationTokenSource linkedCancellationTokenSource;
        private CancellationToken cancellationToken;

        private readonly object stateLock = new object();
        private bool done = false;

        public TagReportTask(
            LLRPClient client,
            Func<MSG_RO_ACCESS_REPORT, SecuCode.StatusCode?> onAccessReport,
            TimeSpan? timeout,
            CancellationToken cancellationToken
        )
        {
            this.onAccessReport = onAccessReport;
            this.client = client;
            this.cancellationToken = cancellationToken;

            this.client.OnRoAccessReportReceived += this.OnRoAccessReportReceived;
            this.client.OnReaderEventNotification += this.OnReaderEventNotification;

            if (timeout != null)
            {
                this.timerCancellationSource = new CancellationTokenSource();
                System.Threading.Tasks.Task.Run(async () => await this.TimeoutTask(timeout.Value), this.timerCancellationSource.Token);
                this.linkedCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(this.timerCancellationSource.Token, this.cancellationToken);
                this.cancellationToken = this.linkedCancellationTokenSource.Token;
            }
        }

        private async Task TimeoutTask(TimeSpan timeout)
        {
            try
            {
                await System.Threading.Tasks.Task.Delay(timeout, this.cancellationToken);
                lock (this.stateLock)
                {
                    this.timerCancellationSource = null;
                    if (!this.done)
                    {
                        Console.Write("Timeout");
                        this.taskCompletionSource.SetResult(new OperationResponse { status = SecuCode.StatusCode.TimeOut });
                        this.Done();
                    }
                }
            }
            catch (Exception)
            {
                lock (this.stateLock)
                {
                    this.timerCancellationSource = null;
                    if (!this.done)
                    {
                        this.Done();
                    }
                }
            }
        }

        private void Done()
        {
            this.client.OnRoAccessReportReceived -= this.OnRoAccessReportReceived;
            this.client.OnReaderEventNotification -= this.OnReaderEventNotification;

            this.done = true;
            if (this.timerCancellationSource != null)
            {
                this.timerCancellationSource.Cancel();
            }
        }

        private void OnReaderEventNotification(MSG_READER_EVENT_NOTIFICATION msg)
        {
            lock (this.stateLock)
            {
                if (this.done)
                {
                    return;
                }

                if (msg.ReaderEventNotificationData.ROSpecEvent.EventType == ENUM_ROSpecEventType.End_Of_ROSpec)
                {
                    this.taskCompletionSource.SetResult(new OperationResponse { status = SecuCode.StatusCode.TimeOut });
                    this.Done();
                }
                else if (msg.ReaderEventNotificationData.ConnectionCloseEvent != null)
                {
                    this.taskCompletionSource.SetResult(new OperationResponse { status = SecuCode.StatusCode.ReaderError });
                    this.Done();
                }
            }
        }

        private void OnRoAccessReportReceived(MSG_RO_ACCESS_REPORT message)
        {
            lock (this.stateLock)
            {
                if (this.done)
                {
                    return;
                }

                var reportStatus = this.onAccessReport(message);
                if (reportStatus != null)
                {
                    this.Done();
                    this.taskCompletionSource.SetResult(new OperationResponse { status = reportStatus.Value });
                }
            }
        }
    }
}
