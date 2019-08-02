using ELFSharp.ELF;
using ELFSharp.ELF.Segments;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;

namespace SecuCodeApp
{
    /// <summary>
    /// A class for loading firmware files that can be sent using SecuCode
    /// </summary>
    public class MspFirmware
    {
        public List<(UInt32 address, byte[] data)> Segments = new List<(uint address, byte[] data)>();

        /// <summary>
        /// Loads the firmware from the specified path.
        /// 
        /// - Files ending with ".txt" are interpreted as TI Hex encoded files.
        /// - Files ending with either ".elf" or ".out" are interpreted as ELF binaries.
        /// </summary>
        public MspFirmware(string path)
        {
            if (path.EndsWith(".txt"))
            {
                this.ParseTiTextFile(path);
            }
            else if (path.EndsWith(".elf") || path.EndsWith(".out"))
            {
                this.ParseELFFile(path);
            }
            else
            {
                throw new Exception("Unrecognised file format");
            }
        }

        /// <summary>
        /// Returns the total firmware bytes in all segments
        /// </summary>
        public int TotalBytes()
        {
            var total = 0;
            foreach (var (_, data) in this.Segments)
            {
                total += data.Length;
            }

            return total;
        }

        private void ParseELFFile(string path)
        {
            var elf = ELFReader.Load<uint>(path);
            foreach (var segment in elf.Segments)
            {
                if (segment.Type == SegmentType.Load && segment.FileSize != 0)
                {
                    this.Segments.Add((segment.PhysicalAddress, segment.GetMemoryContents()));
                }
            }
        }

        private void ParseTiTextFile(string path)
        {
            List<byte> currentSegment = null;
            UInt32 currentAddress = 0;

            foreach (var line in File.ReadLines(path))
            {
                if (line.StartsWith("q"))
                {
                    break;
                }
                else if (line.StartsWith("@"))
                {
                    if (currentSegment != null)
                    {
                        this.Segments.Add((currentAddress, currentSegment.ToArray()));
                    }

                    currentSegment = new List<byte>();
                    currentAddress = UInt32.Parse(line.Substring(1).Trim(), NumberStyles.HexNumber);
                }
                else
                {
                    var bytes = line.Split(' ')
                        .Select(str => str.Trim())
                        .Where(str => str.Length == 2)
                        .Select(str => byte.Parse(str.Trim(), NumberStyles.HexNumber));

                    currentSegment.AddRange(bytes);
                }
            }

            if (currentSegment != null)
            {
                this.Segments.Add((currentAddress, currentSegment.ToArray()));
            }
        }
    }
}
