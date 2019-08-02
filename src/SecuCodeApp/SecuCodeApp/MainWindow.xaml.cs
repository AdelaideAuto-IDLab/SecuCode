using LiveCharts;
using LiveCharts.Wpf;
using Microsoft.Win32;
using Org.LLRP.LTK.LLRPV1;
using Org.LLRP.LTK.LLRPV1.DataType;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace SecuCodeApp
{
    public partial class MainWindow : Window
    {
        ReaderManager reader = new ReaderManager();
        private bool isConnected = false;
        private byte[] bytesToSend = null;
        private SecuCode secuCode = new SecuCode();

        private Dictionary<string, int> epcLookupTable = new Dictionary<string, int>();

        public ObservableCollection<EpcEntry> MacAddressCounts { get; set; } = new ObservableCollection<EpcEntry>();
        public SeriesCollection SeriesCollection { get; set; } = new SeriesCollection();

        private GraphManager graphManager;

        public string ReaderIpText { get; set; } = Properties.Settings.Default.ConnectionString;

        public MainWindow()
        {
            this.DataContext = this;
            this.InitializeComponent();

            this.toggleConnectionButton.Click += this.ToggleConnectionButton_Click;
            this.sendDataButton.Click += this.SendAuthenticateButton_Click;

            this.graphManager = new GraphManager(this.valueAxis, this.SeriesCollection);

            Task.Run(() => this.graphManager.UpdateGraphsTask());
        }

        private void SendAuthenticateButton_Click(object sender, RoutedEventArgs e)
        {
            this.SendDataCommand();
        }

        private async void GetKeyButton_Click(object sender, RoutedEventArgs e)
        {
            await this.secuCode.SendAuthenticate(this.reader, this.GetAuthenticateMode());
            this.UpdateKeyTextBox();
        }

        private SecuCode.Mode GetAuthenticateMode()
        {
            var mode = SecuCode.Mode.Default;

            if (this.batchedDecryptionCheckBox.IsChecked != true)
            {
                mode |= SecuCode.Mode.DecryptAfterEachPacket;
            }

            if (this.ipProtectCheckBox.IsChecked == true)
            {
                mode |= SecuCode.Mode.IpProtect;
            }

            if (this.fixedKeyCheckBox.IsChecked == true)
            {
                mode |= SecuCode.Mode.FixedKey;
            }

            if (this.blake128RadioButton.IsChecked == true)
            {
                mode |= SecuCode.Mode.BlakeHash;
            }
            else if (this.blake256RadioButton.IsChecked == true)
            {
                mode |= SecuCode.Mode.BlakeHash | SecuCode.Mode.HashLength256;
            }
            else if (this.macRadioButton.IsChecked == true)
            {
                mode |= SecuCode.Mode.AesCmac;
            }
            else if (this.noneRadioButton.IsChecked == true)
            {
                mode |= SecuCode.Mode.Crc64;
            }

            return mode;
        }

        private void ClearKeyAndSetStatus(string value)
        {
            this.helperDataTextBox.Dispatcher.Invoke(() =>
            {
                this.helperDataTextBox.Text = "";
                this.keyTextBox.Text = "";
                this.keyStatusTextBox.Text = value;
            });
        }

        private void UpdateKeyTextBox()
        {
            this.helperDataTextBox.Dispatcher.Invoke(() =>
            {
                if (this.secuCode.CrcMode())
                {
                    this.keyStatusTextBox.Text = "Crc only mode";
                    this.hashTextBox.Text = this.secuCode.GetHash(this.bytesToSend).ToByteString();
                    return;
                }

                if (this.secuCode.helperData == null || this.secuCode.key == null)
                {
                    this.keyStatusTextBox.Text = "Error recovering key (timeout)";
                    return;
                }

                this.helperDataTextBox.Text = this.secuCode.mode.HasFlag(SecuCode.Mode.FixedKey) ?
                    "Fixed key mode" :
                    $"{this.secuCode.helperData.ToByteString()} (challenge = {this.secuCode.challenge % 4})";

                this.keyTextBox.Text = this.secuCode.key.ToByteString();
                this.nonceTextBox.Text = this.secuCode.nonce.ToByteString();
                this.hashTextBox.Text = this.secuCode.GetHash(this.bytesToSend).ToByteString();

                if (this.secuCode.corruptedBlocks == 0)
                {
                    this.keyStatusTextBox.Text = "Key successfully recovered";
                }
                else
                {
                    this.keyStatusTextBox.Text = $"Error recovering key (errors found in {this.secuCode.corruptedBlocks} blocks)";
                }
            });
        }

        private void ToggleConnectionButton_Click(object sender, RoutedEventArgs e)
        {
            if (this.isConnected)
            {
                this.UpdateConnectionStatus(false);
                this.Disconnect();
                this.sendDataButton.IsEnabled = false;
            }
            else
            {
                this.UpdateConnectionStatus(true);
                this.Connect();
                this.sendDataButton.IsEnabled = (this.bytesToSend != null && this.isConnected);
            }
        }

        public void Connect()
        {
            var status = this.reader.Connect(this.ReaderIpText.Trim());
            if (status != ENUM_ConnectionAttemptStatusType.Success)
            {
                if ((int)status == -1)
                {
                    // I sometimes get this error but I have no idea what it is. Running the Impinj MultiReader tool first seems to fix it though
                    MessageBox.Show("An unknown error occured.\n\nTry running the Impinj MultiReader tool first", "Error connecting to reader");
                }
                else
                {
                    MessageBox.Show(status.ToString(), "Error connecting to reader");
                }
                this.UpdateConnectionStatus(false);
                return;
            }

            Properties.Settings.Default.ConnectionString = this.ReaderIpText.Trim();
            Properties.Settings.Default.Save();

            this.reader.reader.OnRoAccessReportReceived += this.Reader_OnRoAccessReportReceived;
            this.reader.ResetState();
        }

        private void UpdateConnectionStatus(bool isConnected)
        {
            this.isConnected = isConnected;
            this.toggleConnectionButton.Content = isConnected ? "Disconnect" : "Connect";
            this.inventoryButton.IsEnabled = isConnected;
            this.getHelperDataButton.IsEnabled = isConnected;
        }

        public async void SendDataCommand()
        {
            try
            {
                this.ClearKeyAndSetStatus("Loading");

                // Ensure the tag is in bootloader mode.
                var response = await this.reader.SendControlWithPowerCheck("0B", TagState.RestartInBootMode, 0, TimeSpan.FromSeconds(10.0));
                if (!response.IsSuccess())
                {
                    MessageBox.Show($"Unable to start tag in bootloader mode: {response.status}.", "Error sending data");
                    return;
                }

                this.ClearKeyAndSetStatus("Retrieving helper data");

                // Retry authenticate command until we are successfully able to decode the tag key
                while (true)
                {
                    var status = await this.secuCode.SendAuthenticate(this.reader, this.GetAuthenticateMode());
                    if (status == SecuCode.StatusCode.Success)
                    {
                        this.UpdateKeyTextBox();
                        break;
                    }
                    else if (status == SecuCode.StatusCode.KeyDecodingFailure)
                    {
                        this.UpdateKeyTextBox();
                        await this.reader.SendControlWithPowerCheck("0B", TagState.RestartInBootMode, 0, TimeSpan.FromSeconds(10.0));
                    }
                    else
                    {
                        MessageBox.Show($"Sending authenticate command failed: {status}.", "Error sending data");
                        return;
                    }
                }

                // Now send firmware to the tag, with mac generated using the decoded key
                response = await this.secuCode.SendData(this.reader, this.bytesToSend);
                if (!response.IsSuccess())
                {
                    MessageBox.Show($"Failed to send data: {response.status}.", "Error sending data");
                    return;
                }

                // Keep inventorying for a bit so that we can see any sensor data
                await this.reader.Inventory(TimeSpan.FromSeconds(100.0));
            }
            catch (Exception ex)
            {
                MessageBox.Show($"{ex.Message}\n{ex.StackTrace}", "Error sending data");
            }
        }

        public void Disconnect()
        {
            this.reader.Disconnect();
        }

        private void Reader_OnRoAccessReportReceived(MSG_RO_ACCESS_REPORT message)
        {
            if (message.TagReportData == null)
            {
                return;
            }

            foreach (var tagReportData in message.TagReportData)
            {
                if (tagReportData.EPCParameter.Count > 0)
                {
                    this.UpdateTagData(tagReportData);
                }
            }
        }

        private void UpdateTagData(PARAM_TagReportData tagReportData)
        {
            var epc = Utils.GetEpcString(tagReportData.EPCParameter[0]);
            try
            {
                this.listBox.Dispatcher.Invoke(() =>
                {
                    var maskedEpc = Utils.MaskEpc(epc);
                    this.graphManager.UpdateSensorValues(Utils.GetEpcBytes(tagReportData.EPCParameter[0]));

                    if (!this.epcLookupTable.ContainsKey(maskedEpc))
                    {
                        this.epcLookupTable.Add(maskedEpc, this.MacAddressCounts.Count);
                        this.MacAddressCounts.Add(new EpcEntry { Epc = maskedEpc, Count = 0 });
                    }

                    this.MacAddressCounts[this.epcLookupTable[maskedEpc]].Count += 1;
                });

                this.mostRecentEpcTextBox.Dispatcher.Invoke(() =>
                {
                    this.mostRecentEpcTextBox.Text = epc;
                });
            }
            catch { }
        }

        private void LoadDataButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                int.TryParse(this.lengthBox.Text?.Trim(), out var length);

                var dialog = new OpenFileDialog();
                if (dialog.ShowDialog() == true)
                {
                    this.bytesToSend = File.ReadAllBytes(dialog.FileName);
                    if (length != 0)
                    {
                        this.bytesToSend = this.bytesToSend.Take(length).ToArray();
                    }

                    this.bytesToSend = this.bytesToSend.Concat(new byte[this.bytesToSend.Length % 2]).ToArray();

                    this.loadedFileName.Text = $"{dialog.FileName} ({this.bytesToSend.Length} bytes)";
                    this.loadedBytes.Text = this.bytesToSend.ToByteString();
                    this.sendDataButton.IsEnabled = (this.bytesToSend != null && this.isConnected);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"{ex.Message}\n{ex.StackTrace}", "Failed to load file");
            }
        }

        private void LoadProgramButton_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog();
            if (dialog.ShowDialog() == true)
            {
                try
                {
                    var appendRunCommand = this.appendRunCommandCheckBox.IsChecked == true;

                    var firmware = new MspFirmware(dialog.FileName);
                    this.bytesToSend = MspBoot.EncodeProgram(firmware, appendRunCommand);
                    this.loadedFileName.Text = $"{dialog.FileName} (raw: {firmware.TotalBytes()} bytes, encoded: {this.bytesToSend.Length} bytes)";
                    this.loadedBytes.Text = this.bytesToSend.ToByteString();
                    Console.WriteLine(this.bytesToSend.ToByteString("0x", ","));
                    Console.WriteLine($"Length = {this.bytesToSend.Length}");
                    this.sendDataButton.IsEnabled = (this.bytesToSend != null && this.isConnected);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.StackTrace, "Failed to load program");
                }
            }
        }

        private async void InventoryButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await this.reader.Inventory(TimeSpan.FromSeconds(1000));
            }
            catch { }
        }

        private void BenchmarkButton_Click(object sender, RoutedEventArgs e)
        {
            if (this.reader.IsConnected() && this.bytesToSend != null)
            {
                var benchmarkWindow = new BenchmarkWindow(this.reader, this.GetAuthenticateMode(), this.bytesToSend);
                benchmarkWindow.Show();
            }
            else
            {
                MessageBox.Show("Must be connected to reader and have loaded a firmware before running benchmarks.");
            }
        }

        private void DebugButton_Click(object sender, RoutedEventArgs e)
        {
            var debugWindow = new DebugWindow();
            debugWindow.Show();
        }
    }

    public class GraphManager
    {
        public SeriesCollection SeriesCollection { get; set; }

        private LineSeries temperature = new LineSeries { Title = "Temperature", Values = new ChartValues<double>(), PointGeometry = null, Fill = Brushes.Transparent };
        private LineSeries accelX = new LineSeries { Title = "Accel_x", Values = new ChartValues<double>(), PointGeometry = null, Fill = Brushes.Transparent };
        private LineSeries accelY = new LineSeries { Title = "Accel_y", Values = new ChartValues<double>(), PointGeometry = null, Fill = Brushes.Transparent };
        private LineSeries accelZ = new LineSeries { Title = "Accel_z", Values = new ChartValues<double>(), PointGeometry = null, Fill = Brushes.Transparent };

        private MeanTracker currentAccelX = new MeanTracker();
        private MeanTracker currentAccelY = new MeanTracker();
        private MeanTracker currentAccelZ = new MeanTracker();
        private MeanTracker currentTemp = new MeanTracker();
        private Axis valueAxis;

        public GraphManager(Axis valueAxis, SeriesCollection series)
        {
            this.SeriesCollection = series;
            this.valueAxis = valueAxis;
            this.SeriesCollection.Add(this.temperature);
            this.SeriesCollection.Add(this.accelX);
            this.SeriesCollection.Add(this.accelY);
            this.SeriesCollection.Add(this.accelZ);
        }

        public async void UpdateGraphsTask()
        {
            var i = 0;
            try
            {
                while (true)
                {
                    await Task.Delay(50);
                    this.accelX.Values.Add(this.currentAccelX.Reset());
                    this.accelY.Values.Add(this.currentAccelY.Reset());
                    this.accelZ.Values.Add(this.currentAccelZ.Reset());
                    this.temperature.Values.Add(this.currentTemp.Reset());

                    if (i > 40)
                    {
                        this.accelX.Values.RemoveAt(0);
                        this.accelY.Values.RemoveAt(0);
                        this.accelZ.Values.RemoveAt(0);
                        this.temperature.Values.RemoveAt(0);
                    }
                    i += 1;
                }
            }
            catch (TaskCanceledException)
            {
                // Ignore
            }
        }

        public void UpdateSensorValues(byte[] epc)
        {
            if (epc[1] == 0xA1)
            {
                this.valueAxis.MinValue = -2.0;
                this.valueAxis.MaxValue = 2.0;

                this.currentAccelX.Next(ConvertAccelValue(epc[6]));
                this.currentAccelY.Next(ConvertAccelValue(epc[4]));
                this.currentAccelZ.Next(ConvertAccelValue(epc[8]));
            }
            else
            {
                this.currentAccelX.Next(0.0);
                this.currentAccelY.Next(0.0);
                this.currentAccelZ.Next(0.0);
            }

            if (epc[1] == 0xA0)
            {
                this.currentTemp.Next(ConvertTempValue(epc[7], epc[8]));
                this.valueAxis.MinValue = double.NaN;
                this.valueAxis.MaxValue = double.NaN;
            }
            else
            {
                this.currentTemp.Next(0.0);
            }
        }

        private static double ConvertAccelValue(byte lsb)
        {
            return 4.0 * (lsb - 128) / 250.0;
        }

        private static double ConvertTempValue(byte msb, byte lsb)
        {
            return ((msb << 8) | lsb) / 1000.0;
        }
    }

    public class EpcEntry : INotifyPropertyChanged
    {
        public string Epc { get; set; }

        public int count;
        public int Count
        {
            get { return this.count; }
            set { this.count = value; this.OnPropertyChanged(nameof(this.Count)); }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(string name)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}
