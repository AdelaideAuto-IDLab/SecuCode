using SecuCodeApp.Native;
using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Threading.Tasks;

namespace SecuCodeApp
{
    /// <summary>
    /// Implementation of the SecuCode protocol.
    /// 
    /// Example usage:
    /// 
    /// <code>
    /// var mode = ...;
    /// var reader = ...;
    /// 
    /// var secuCode = new SecuCode();
    /// var status = await secuCode.SendAuthenticate(reader, mode);
    /// if (status != SecuCode.StatusCode.Success)
    /// {
    ///     // Handle error
    /// }
    /// var response = await secuCode.SendData(reader, bytesToSend);
    /// if (!response.IsSuccess())
    /// {
    ///     // Handle error
    /// }
    /// 
    /// await reader.Inventory(TimeSpan.FromSeconds(100.0));
    /// </code>
    /// </summary>
    public class SecuCode
    {
        public enum StatusCode
        {
            Success,
            TimeOut,
            KeyDecodingFailure,
            HashFailure,
            PowerFailure,
            FailureCountExceeded,
            UnknownError,
            ReaderError,
        }

        /// <summary>
        /// The security mode flags (CSI field in Authenticate)
        /// </summary>
        [Flags]
        public enum Mode : ushort
        {
            Default = 0,
            IpProtect = 1 << 0,
            FixedKey = 1 << 1,
            BlakeHash = 1 << 2,
            HashLength256 = 1 << 3,
            DecryptAfterEachPacket = 1 << 4,
            AesCmac = 1 << 5,
            Crc64 = 1 << 6,
        }

        public Mode mode;

        private const int HELPER_DATA_LENGTH = 16;

        public byte[] key;
        public byte[] helperData;
        public byte[] nonce;
        public byte challenge;

        public int corruptedBlocks;

        /// <summary>
        /// Sends the "Authenticate" command to the tag, this does the following:
        /// 
        /// * Ensures that the tag is in firmware receive mode
        /// * Transfers the CSI parameters to the tag
        /// * Retreives helper data from the tag
        /// * Decodes and saves the key
        /// </summary>
        public async Task<StatusCode> SendAuthenticate(ReaderManager reader, Mode mode)
        {
            this.mode = mode;
           
            var response = await reader.SendControlWithPowerCheck("0B", TagState.Authenticate, (ushort)mode, TimeSpan.FromSeconds(20.0), false);
            if (response.status != StatusCode.Success)
            {
                return response.status;
            }

            if (this.CrcMode())
            {
                return StatusCode.Success;
            }

            var nonceLength = 8;
            if (this.mode.HasFlag(Mode.AesCmac))
            {
                nonceLength = 16;
            }

            // The reader doesn't support the ReadBuffer command so we just use a regular read
            // command here to get the helper data. The tag knows how to 
            var helperDataResponse = await reader.Read("0B", 3, 0, (HELPER_DATA_LENGTH + nonceLength) / 2, TimeSpan.FromSeconds(20.0));
            if (helperDataResponse.response.status != StatusCode.Success)
            {
                return helperDataResponse.response.status;
            }

            var payload = helperDataResponse.payload;

            this.helperData = payload.Take(HELPER_DATA_LENGTH).ToArray();
            this.nonce = payload.Skip(HELPER_DATA_LENGTH).ToArray();

            if (this.mode.HasFlag(Mode.FixedKey))
            {
                this.key = new byte[] { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
                return StatusCode.Success;
            }

            this.challenge = this.nonce[0];
            (this.corruptedBlocks, this.key) = Bch.DecodeKey(this.challenge, this.helperData);

            if (this.corruptedBlocks != 0)
            {
                return StatusCode.KeyDecodingFailure;
            }

            return StatusCode.Success;
        }

        /// <summary>
        /// Transfers a signed message to the tag.
        /// 
        /// Note: The authenticate command must be sent before executing this method.
        /// </summary>
        public async Task<OperationResponse> SendData(ReaderManager reader, byte[] bytesToSend, bool sendDone = true)
        {
            if (this.key == null)
            {
                throw new Exception("An authenticate command is required before data can be sent.");
            }

            byte[] data;
            if (this.mode.HasFlag(Mode.AesCmac))
            {
                data = this.AppendAesCmac(this.AddPacketLengthAndPad(bytesToSend), this.key, this.nonce);
            }
            else if (this.mode.HasFlag(Mode.Crc64))
            {
                data = this.AppendCrc(this.AddPacketLengthAndPad(bytesToSend));
            }
            else if (this.mode.HasFlag(Mode.BlakeHash))
            {
                data = this.mode.HasFlag(Mode.IpProtect) ?
                    this.EncryptAndSignPacket(bytesToSend, this.key, this.nonce) :
                    this.SignPacket(bytesToSend, this.key, this.nonce);
            }
            else
            {
                data = this.AddPacketLengthAndPad(bytesToSend);
            }

            // If the tag should perform decryption after each packed we reduce chunk size to 
            // ensure that each block corresponds to exactly 1 AES block (16 bytes)
            // Note: this only applies to IP protect mode
            var bytesPerChunk = this.mode.HasFlag(Mode.DecryptAfterEachPacket) ? 16 : 64;

            var response = await reader.SendData("0B", data, bytesPerChunk, 0x00, TimeSpan.FromSeconds(20.0));
            if (!response.IsSuccess())
            {
                return response;
            }

            if (sendDone)
            {
                response = await reader.SendControl("0B", TagState.Done, 0, 0x00, TimeSpan.FromSeconds(10.0));
            }

            return response;
        }

        public byte[] GetHash(byte[] bytesToSend)
        {
            var data = this.AddPacketLengthAndPad(bytesToSend);
            if (this.CrcMode())
            {
                return Crc64.GenerateCrc64(data);
            }

            return AesCmac.GenerateAesCmac(this.key, this.nonce, data);
        }

        private byte[] AddPacketLengthAndPad(byte[] data)
        {
            var lengthBytes = new byte[] { (byte)(data.Length & 0xFF), (byte)(data.Length >> 8) };
            return lengthBytes.Concat(data).Concat(new byte[CalculatePadding(data.Length, 2)]).ToArray();
        }

        /// <summary>
        /// Appends the hash of the message (with appropriate padding) for verification, then
        /// encrypts and returns the result.
        /// </summary>
        private byte[] EncryptAndSignPacket(byte[] data, byte[] key, byte[] nonce)
        {
            var contentLength = 2 + data.Length + this.HashLength();

            // Store the actual length of the data in the payload, since the actual length will be 
            // unknown due to the added padding.
            var lengthBytes = new byte[] { (byte)(data.Length & 0xFF), (byte)(data.Length >> 8) };
            var bytesWithLength = lengthBytes.Concat(data);

            // Pad bytes with zeros to fit our AES block size, and append a hash of the data
            var paddedBytes = bytesWithLength.Concat(new byte[CalculatePadding(contentLength, 16)]).ToArray();
            var hash = this.HashBytes(paddedBytes, nonce);
            var bytesWithHash = paddedBytes.Concat(hash).ToArray();

            return Aes.EncryptBytes(bytesWithHash, key);
        }

        /// <summary>
        /// Signs a packet by computing its hash (based on the currently configured mode) an then
        /// encrypting it using AES with the provided key.
        /// 
        /// The signature is appended to the message (with appropriate padding) and returned.
        /// </summary>
        private byte[] SignPacket(byte[] data, byte[] key, byte[] nonce)
        {
            // Store the actual length of the data in the payload, since the actual length will be 
            // unknown due to the added padding.
            var lengthBytes = new byte[] { (byte)(data.Length & 0xFF), (byte)(data.Length >> 8) };
            var bytesWithLength = lengthBytes.Concat(data).Concat(new byte[CalculatePadding(data.Length, 2)]).ToArray();

            // In NoIpProtect mode, we only encrypt the hash portion of the message
            var hash = this.HashBytes(bytesWithLength, nonce);
            var encryptedHash = Aes.EncryptBytes(new byte[CalculatePadding(hash.Length, 16)].Concat(hash).ToArray(), key);

            return bytesWithLength.Concat(encryptedHash).ToArray();
        }

        private byte[] AppendAesCmac(byte[] data, byte[] key, byte[] nonce)
        {
            var mac = AesCmac.GenerateAesCmac(key, nonce, data);
            return data.Concat(mac).ToArray();
        }

        private byte[] AppendCrc(byte[] data)
        {
            return data.Concat(Crc64.GenerateCrc64(data)).ToArray();
        }

        private static int CalculatePadding(int length, int alignment)
        {
            return (alignment - length % alignment) % alignment;
        }

        private int HashLength()
        {
            if (this.mode.HasFlag(Mode.BlakeHash) && this.mode.HasFlag(Mode.HashLength256))
            {
                return 32;
            }
            else if (this.mode.HasFlag(Mode.BlakeHash))
            {
                return 16;
            }

            return 8;
        }

        private byte[] HashBytes(byte[] bytes, byte[] nonce)
        {
            if (this.mode.HasFlag(Mode.BlakeHash) && this.mode.HasFlag(Mode.HashLength256))
            {
                return Blake2s.Hash(nonce, bytes, 32);
            }
            else if (this.mode.HasFlag(Mode.BlakeHash))
            {
                return Blake2s.Hash(nonce, bytes, 16);
            }

            return Speck.Hash(BitConverter.ToUInt64(nonce, 0), bytes);
        }

        public bool CrcMode()
        {
            return this.mode.HasFlag(Mode.Crc64);
        }
    }

    static class Aes
    {
        public static byte[] EncryptBytes(byte[] bytes, byte[] key)
        {
            using (var algorithm = new RijndaelManaged())
            {
                algorithm.BlockSize = 128;
                algorithm.KeySize = 128;
                algorithm.Padding = PaddingMode.Zeros;
                // DANGER: using AES in this mode is weak in Ip protect mode since repeated blocks
                // will be the same after encryption. (Since there is only one block in NoIpProtect
                // mode, this is not an issue).
                algorithm.Mode = CipherMode.ECB;

                var encryptor = algorithm.CreateEncryptor(key, new byte[16]);
                using (var memoryStream = new MemoryStream())
                {
                    using (var cryptoStream = new CryptoStream(memoryStream, encryptor, CryptoStreamMode.Write))
                    {
                        cryptoStream.Write(bytes, 0, bytes.Length);
                        cryptoStream.Flush();
                        return memoryStream.ToArray();
                    }
                }
            }
        }

        public static byte[] DecryptBytes(byte[] bytes, byte[] key)
        {
            using (var algorithm = new RijndaelManaged())
            {
                algorithm.BlockSize = 128;
                algorithm.KeySize = 128;
                algorithm.Padding = PaddingMode.Zeros;
                algorithm.Mode = CipherMode.ECB;

                var decryptor = algorithm.CreateDecryptor(key, new byte[16]);
                using (var memoryStream = new MemoryStream())
                {
                    using (var cryptoStream = new CryptoStream(memoryStream, decryptor, CryptoStreamMode.Write))
                    {
                        cryptoStream.Write(bytes, 0, bytes.Length);
                        cryptoStream.Flush();
                        return memoryStream.ToArray();
                    }
                }
            }
        }
    }
}
