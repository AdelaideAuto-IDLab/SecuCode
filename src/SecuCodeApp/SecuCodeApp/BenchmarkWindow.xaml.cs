using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;
using System.Windows;

namespace SecuCodeApp
{
    /// <summary>
    /// Interaction logic for BenchmarkWindow.xaml
    /// </summary>
    public partial class BenchmarkWindow : Window
    {
        private ReaderManager reader;
        private byte[] firmware;
        private SecuCode.Mode mode;
        private SecuCode secuCode;

        public BenchmarkWindow(ReaderManager reader, SecuCode.Mode mode, byte[] firmware)
        {
            this.InitializeComponent();
            this.reader = reader;
            this.firmware = firmware;
            this.mode = mode;
        }

        private async void EndToEndButton_Click(object sender, RoutedEventArgs e)
        {
            await this.RunTest(async () => await this.EndToEndRound(this.firmware));
        }

        private async void SendDataButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var prepResult = await this.InitHashRound(this.firmware);
                if (!prepResult.IsSuccess())
                {
                    MessageBox.Show($"Failed to initialize tag for Hash benchmark: {prepResult.status}");
                }
                else
                {
                    MessageBox.Show($"Successfully uploaded: {this.firmware.Length} bytes of data");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"An unknown error occured: {ex.Message}\n{ex.StackTrace}");
            }
        }

        private async Task<OperationResponse> InitHashRound(byte[] data)
        {
            var prepResponse = await this.reader.SendControlWithPowerCheck("0B", TagState.RestartInBootMode, 0, TimeSpan.FromSeconds(10.0));
            if (!prepResponse.IsSuccess())
            {
                return prepResponse;
            }

            var authenticateResponse = await this.reader.SendControlWithPowerCheck("0B", TagState.Authenticate, (ushort)this.mode, TimeSpan.FromSeconds(20.0), false);
            if (!authenticateResponse.IsSuccess())
            {
                return authenticateResponse;
            }

            return await this.reader.SendData("0B", data, 64, 0, TimeSpan.FromSeconds(30.0));
        }

        private async Task RunTest(Func<Task<RoundResult>> test)
        {
            var iterCount = int.Parse(this.iterationCount.Text);
            this.iterationText.Text = $"0 of {iterCount}";
            this.powerLossText.Text = "0";
            this.timeoutText.Text = "0";
            this.successesText.Text = "0";
            this.averageDelayText.Text = "0";

            var powerFailures = 0;
            var timeouts = 0;
            var successCount = 0;
            var successDurations = 0.0;
            var keyFailureCounts = 0;

            var output = new List<RoundResult>();
            var outputPath = this.logFileTextBox.Text.Trim();

            for (var i = 0; i < iterCount; ++i)
            {
                var result = await test();
                output.Add(result);

                switch (result.statusCode)
                {
                    case RoundResult.Code.Success:
                        successCount += 1;
                        successDurations += result.duration.TotalSeconds;
                        break;
                    case RoundResult.Code.Timeout:
                        timeouts += 1;
                        break;
                    case RoundResult.Code.PowerReset:
                        powerFailures += 1;
                        break;
                    case RoundResult.Code.PrepFailure:
                        timeouts += 1;
                        break;
                    case RoundResult.Code.KeyFailure:
                        keyFailureCounts += 1;
                        break;
                }

                this.iterationText.Text = $"{i + 1} of {iterCount}";
                this.powerLossText.Text = $"{powerFailures}";
                this.timeoutText.Text = $"{timeouts}";
                this.successesText.Text = $"{successCount}";
                this.keyFailure.Text = $"{keyFailureCounts}";

                if (successCount != 0)
                {
                    this.averageDelayText.Text = $"{successDurations / successCount}";
                }

#if !LOG_AT_END
                if (!string.IsNullOrEmpty(outputPath))
                {
                    File.AppendAllText(outputPath, $"{result.statusCode},{result.duration.TotalSeconds}\n");
                }
#endif
            }

#if LOG_AT_END
            if (!string.IsNullOrEmpty(outputPath))
            {
                using (var fs = File.Create(outputPath))
                {
                    var header = new UTF8Encoding(true).GetBytes("code,duration\n");
                    fs.Write(header, 0, header.Length);
                    foreach (var entry in output)
                    {
                        var row = new UTF8Encoding(true).GetBytes($"{entry.statusCode},{entry.duration.TotalSeconds}\n");
                        fs.Write(row, 0, row.Length);
                    }
                }
            }
#endif
        }

        private async Task<RoundResult> EndToEndRound(byte[] firmware)
        {
            // This first restart ensures that the tag correctly restarts into bootloader mode
            // from the old fimrware
            var prepResponse = await this.reader.SendControlWithPowerCheck("0B", TagState.RestartInBootMode, 0, TimeSpan.FromSeconds(10.0));
            if (!prepResponse.IsSuccess())
            {
                return new RoundResult { duration = TimeSpan.Zero, statusCode = RoundResult.Code.PrepFailure };
            }

#if ALTERNATE_CHECK
            // This second restart returns immediately after the command has been processed after
            // which we stop the reader to ensure SRAM state loss.
            prepResponse = await this.reader.SendControlWaitForAck("0B", TagState.RestartInBootMode, 0, TimeSpan.FromSeconds(10.0));
            if (!prepResponse.IsSuccess())
            {
                return new RoundResult { duration = TimeSpan.Zero, statusCode = RoundResult.Code.PrepFailure };
            }
#endif
            this.reader.ResetState();
            await Task.Delay(int.Parse(this.sleepTime.Text.Trim()));

            // Now the tag is ready to process commands.
            this.secuCode = new SecuCode();
            var (duration, endToEndResponse) = await TimeFunction(async () =>
            {
                var status = await this.secuCode.SendAuthenticate(this.reader, this.mode);
                if (status != SecuCode.StatusCode.Success)
                {
                    return status;
                }

                var response = await this.secuCode.SendData(this.reader, firmware);
                return response.status;
            });

            return new RoundResult(duration, endToEndResponse);
        }

        private static async Task<(TimeSpan, T)> TimeFunction<T>(Func<Task<T>> func)
        {
            var stopWatch = new Stopwatch();
            stopWatch.Start();

            var result = await func();

            stopWatch.Stop();
            return (stopWatch.Elapsed, result);
        }

        private void LogFileButton_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new SaveFileDialog();
            if (dialog.ShowDialog() == true)
            {
                this.logFileTextBox.Text = dialog.FileName;
            }
            else
            {
                this.logFileTextBox.Text = "";
            }
        }
    }

    class RoundResult
    {
        public enum Code
        {
            Success,
            Timeout,
            PowerReset,
            PrepFailure,
            KeyFailure
        }
        public static RoundResult FromStatusCode(SecuCode.StatusCode code)
        {
            switch (code)
            {
                case SecuCode.StatusCode.Success:
                    return new RoundResult { duration = TimeSpan.Zero, statusCode = Code.Success };
                case SecuCode.StatusCode.TimeOut:
                    return new RoundResult { duration = TimeSpan.Zero, statusCode = Code.Timeout };
                case SecuCode.StatusCode.KeyDecodingFailure:
                    return new RoundResult { duration = TimeSpan.Zero, statusCode = Code.KeyFailure };
                case SecuCode.StatusCode.PowerFailure:
                    return new RoundResult { duration = TimeSpan.Zero, statusCode = Code.PowerReset };
                case SecuCode.StatusCode.UnknownError:
                    return new RoundResult { duration = TimeSpan.Zero, statusCode = Code.PrepFailure };
                case SecuCode.StatusCode.ReaderError:
                    return new RoundResult { duration = TimeSpan.Zero, statusCode = Code.PrepFailure };
                default:
                    break;
            }
            return null;
        }

        public TimeSpan duration;
        public Code statusCode;

        public RoundResult() { }

        public RoundResult(TimeSpan duration, SecuCode.StatusCode status)
        {
            this.duration = duration;
            this.statusCode = FromStatusCode(status).statusCode;
        }
    }
}
