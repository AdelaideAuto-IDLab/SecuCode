using Microsoft.Win32;
using SecuCodeApp.Native;
using System;
using System.IO;
using System.Linq;
using System.Windows;

namespace SecuCodeApp
{
    /// <summary>
    /// Interaction logic for DebugWindow.xaml
    /// </summary>
    public partial class DebugWindow : Window
    {
        public DebugWindow()
        {
            this.InitializeComponent();
            var key = new byte[] { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18 };
            this.keyTextBox.Text = key.ToByteString();
            this.nonceTextBox.Text = key.ToByteString();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var key = StringToByteArray(this.keyTextBox.Text);
                var nonce = StringToByteArray(this.nonceTextBox.Text);
                var data = StringToByteArray(this.dataTextBox.Text);

                var crc = Crc64.GenerateCrc64(data);
                this.crcTextBox.Text = crc.ToByteString();

                var mac = AesCmac.GenerateAesCmac(key, nonce, data);
                this.hashTextBox.Text = mac.ToByteString();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString(), "Failed to create mac");
            }
        }

        public static byte[] StringToByteArray(string hex)
        {
            return Enumerable.Range(0, hex.Length)
                             .Where(x => x % 2 == 0)
                             .Select(x => Convert.ToByte(hex.Substring(x, 2), 16))
                             .ToArray();
        }

        private void LoadButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var dialog = new OpenFileDialog();
                if (dialog.ShowDialog() == true)
                {
                    this.dataTextBox.Text = File.ReadAllBytes(dialog.FileName).ToByteString();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"{ex.Message}\n{ex.StackTrace}", "Failed to load file");
            }
        }
    }
}
