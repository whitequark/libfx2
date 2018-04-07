import sys
import os
import io
import re
import collections
import argparse
import textwrap
import usb1

from . import VID_CYPRESS, PID_FX2, FX2Device, FX2DeviceError


class VID_PID(collections.namedtuple("VID_PID", "vid pid")):
    @classmethod
    def parse(cls, s):
        match = re.match(r"^([0-9a-f]{1,4}):([0-9a-f]{1,4})$", s, re.I)
        if not match:
            raise ValueError("{} is not a VID:PID pair".format(s))
        vid = int(match.group(1), 16)
        pid = int(match.group(2), 16)
        return cls(vid, pid)

    def __str__(self):
        return "{:04x}:{:04x}".format(self.vid, self.pid)


class TextHelpFormatter(argparse.HelpFormatter):
    def _fill_text(self, text, width, indent):
        def filler(match):
            text = match[0]

            list_match = re.match(r"(\s*)\*", text)
            if list_match:
                return text

            text = textwrap.fill(text, width,
                                 initial_indent=indent,
                                 subsequent_indent=indent)

            text = re.sub(r"(\w-) (\w)", r"\1\2", text)
            text = text + (match[2] or "")
            return text

        text = textwrap.dedent(text).strip()
        return re.sub(r"((?!\n\n)(?!\n\s+\*).)+(\n*)?", filler, text, flags=re.S)


def get_argparser():
    def vid_pid(arg):
        try:
            return VID_PID.parse(arg)
        except ValueError:
            raise argparse.ArgumentTypeError("{} is not a VID:PID pair".format(arg))

    def int_with_base(arg):
        try:
            return int(arg, 0)
        except ValueError:
            raise argparse.ArgumentTypeError("{} is not an integer".format(arg))

    parser = argparse.ArgumentParser(
        formatter_class=TextHelpFormatter,
        description="""
        Cypress FX2/FX2LP bootloader tool

        This tool can read and write data in three formats:

          * hex: contiguous hexadecimal dump with non-significant whitespace
          * bin (.bin extension): contiguous binary
          * ihex (.ihex, .ihx, .hex extensions): discontiguous Intel HEX records

        By default, the format is automatically determined from the file extension,
        and if reading from standard input or writing to standard output, whether
        the stream is a terminal (human-readable hexadecimal is used) or not
        (binary is used).
        """)
    parser.add_argument(
        "-d", "--device", type=vid_pid, default=(VID_CYPRESS, PID_FX2),
        help="device VID:PID pair")
    parser.add_argument(
        "-F", "--format", choices=["hex", "bin", "ihex", "auto"], default="auto",
        help="data input/output format")
    parser.add_argument(
        "-S", "--stage2", metavar="FILENAME", type=argparse.FileType("rb"),
        help="load the specified second stage bootloader before any further action")
    parser.add_argument(
        "-B", "--bootloader", action="store_true",
        help="load the second stage bootloader provided with fx2tool")

    subparsers = parser.add_subparsers(dest="action", metavar="COMMAND")
    subparsers.required = True

    p_load = subparsers.add_parser(
        "load", formatter_class=TextHelpFormatter,
        help="load and run firmware",
        description="Loads firmware into on-chip code memory and runs it.")
    p_load.add_argument(
        "firmware", metavar="FIRMWARE", type=argparse.FileType("rb"),
        help="read firmware from the specified file")

    def add_read_args(parser):
        parser.add_argument(
            "-f", "--file", metavar="FILENAME", type=argparse.FileType("wb"), default="-",
            help="write data to the specified file")
        parser.add_argument(
            "address", metavar="ADDRESS", type=int_with_base,
            help="starting address")
        parser.add_argument(
            "length", metavar="LENGTH", type=int_with_base,
            help="amount of bytes to read")

    def add_write_args(parser):
        parser.add_argument(
            "-a", "--offset", metavar="ADDRESS", type=int_with_base, default=0,
            help="starting address")
        group = parser.add_mutually_exclusive_group(required=True)
        group.add_argument(
            "-f", "--file", metavar="FILENAME", type=argparse.FileType("rb"),
            help="read data from the specified file")
        group.add_argument(
            "-d", "--data", metavar="DATA", type=str,
            help="hexadecimal bytes to write")

    ram_note = textwrap.dedent("""
    \"External\" RAM means on-chip RAM external to the 8051 core, i.e. RAM where
    __code or __xdata objects are placed by the toolchain. Internal RAM of the 8051
    is not accessible by this tool.
    """)

    p_read_ram = subparsers.add_parser("read_ram",
        formatter_class=TextHelpFormatter,
        help="read data from \"external\" (__xdata) RAM",
        description="Reads data from on-chip \"external\" RAM.\n" + ram_note)
    add_read_args(p_read_ram)

    p_write_ram = subparsers.add_parser("write_ram",
        formatter_class=TextHelpFormatter,
        help="write data to \"external\" (__xdata) RAM",
        description="Writes data to on-chip \"external\" RAM.\n" + ram_note)
    add_write_args(p_write_ram)

    def add_eeprom_args(parser):
        parser.add_argument(
            "-W", "--address-width", metavar="WIDTH", type=int, choices=[1, 2], default=2,
            help="EEPROM address width in bytes")

    bootloader_note = textwrap.dedent("""
    An appropriate second stage bootloader must be loaded for this command to work,
    see the --stage2 option. The format of the bootloader firmware file is auto-detected.
    """)

    p_read_eeprom = subparsers.add_parser("read_eeprom",
        formatter_class=TextHelpFormatter,
        help="read data from boot EEPROM",
        description="Reads data from boot EEPROM.\n" + bootloader_note)
    add_eeprom_args(p_read_eeprom)
    add_read_args(p_read_eeprom)

    p_write_eeprom = subparsers.add_parser("write_eeprom",
        formatter_class=TextHelpFormatter,
        help="write data to boot EEPROM",
        description="Writes data to boot EEPROM.\n" + bootloader_note)
    add_eeprom_args(p_write_eeprom)
    add_write_args(p_write_eeprom)

    return parser


def autodetect_format(file):
    basename, extname = os.path.splitext(file.name)
    if extname in [".hex", ".ihex", ".ihx"]:
        return "ihex"
    elif extname in [".bin"]:
        return "bin"
    elif file.isatty():
        return "hex"
    else:
        raise SystemExit("Specify file format explicitly")


def output_data(fmt, file, data, offset):
    if isinstance(file, io.TextIOWrapper):
        file = file.buffer

    if fmt == "auto":
        fmt = autodetect_format(file)

    if fmt == "bin":
        file.write(data)

    elif fmt == "hex":
        n = 0
        for n, byte in enumerate(data):
            file.write("{:02x}".format(byte).encode())
            if n > 0 and n % 16 == 15:
                file.write(b"\n")
            elif n > 0 and n % 8 == 7:
                file.write(b"  ")
            else:
                file.write(b" ")
        if n % 16 != 15:
            file.write(b"\n")

    elif fmt == "ihex":
        pos = 0
        while pos < len(data):
            recoff  = offset + pos
            recdata = data[pos:pos + 0x10]
            record  = [
                len(recdata),
                (recoff >> 8) & 0xff,
                (recoff >> 0) & 0xff,
                0x00,
                *list(recdata)
            ]
            record.append((~sum(record) + 1) & 0xff)
            file.write(b":")
            file.write(bytes(record).hex().encode())
            file.write(b"\n")
            pos += len(recdata)
        file.write(b":00000001ff\n")


def input_data(fmt, file, data, offset):
    if isinstance(file, io.TextIOWrapper):
        file = file.buffer

    if file is None:
        fmt = "hex"
        data = data.encode()
    else:
        data = file.read()

    if fmt == "auto":
        fmt = autodetect_format(file)

    if fmt == "bin":
        return [(offset, data)]

    elif fmt == "hex":
        try:
            hexdata = re.sub(r"\s*", "", data.decode())
            bindata = bytes.fromhex(hexdata)
        except ValueError as e:
            raise SystemExit("Invalid hexadecimal data")
        return [(offset, bindata)]

    elif fmt == "ihex":
        RE_HDR = re.compile(rb":([0-9a-f]{8})", re.I)
        RE_WS  = re.compile(rb"\s*")

        resoff = 0
        resbuf = []
        res = []

        pos = 0
        while pos < len(data):
            match = RE_HDR.match(data, pos)
            if match is None:
                raise SystemExit("Invalid record header at offset {}".format(pos))
            *rechdr, = bytes.fromhex(match.group(1).decode())
            reclen, recoffh, recoffl, rectype = rechdr

            recdatahex = data[match.end(0):match.end(0)+(reclen+1)*2]
            if len(recdatahex) < (reclen + 1) * 2:
                raise SystemExit("Truncated record at offset {}".format(pos))
            try:
                *recdata, recsum = bytes.fromhex(recdatahex.decode())
            except ValueError:
                raise SystemExit("Invalid record data at offset {}".format(pos))
            if sum(rechdr + recdata + [recsum]) & 0xff != 0:
                raise SystemExit("Invalid record checksum at offset {}".format(pos))
            if rectype not in [0x00, 0x01]:
                raise SystemExit("Unknown record type at offset {}".format(pos))
            if rectype == 0x01:
                break

            recoff = (recoffh << 8) | recoffl
            if resoff + len(resbuf) == recoff:
                resbuf += recdata
            else:
                if len(resbuf) > 0:
                    res.append((offset + resoff, resbuf))
                resoff  = recoff
                resbuf  = recdata

            match = RE_WS.match(data, match.end(0) + len(recdatahex))
            pos = match.end(0)

        if len(resbuf) > 0:
            res.append((offset + resoff, resbuf))

        return res


def load(device, fmt, firmware):
    data = input_data(fmt, firmware, None, 0)
    device.cpu_reset(True)
    for address, chunk in data:
        device.write_ram(address, chunk)
    device.cpu_reset(False)


def main():
    resource_dir = os.path.dirname(os.path.abspath(__file__))
    args = get_argparser().parse_args()

    try:
        vid, pid = args.device
        device = FX2Device(vid, pid)
    except FX2DeviceError as e:
        raise SystemExit(e)

    try:
        if args.bootloader:
            bootloader_ihex = os.path.join(resource_dir, "bootloader.ihex")
            load(device, "auto", open(bootloader_ihex))
        elif args.stage2:
            load(device, "auto", args.stage2)

        if args.action == "load":
            load(device, args.format, args.firmware)

        elif args.action == "read_ram":
            device.cpu_reset(True)
            data = device.read_ram(args.address, args.length)
            output_data(args.format, args.file, data, args.address)

        elif args.action == "write_ram":
            data = input_data(args.format, args.file, args.data, args.offset)
            device.cpu_reset(True)
            for address, chunk in data:
                device.write_ram(address, chunk)

        elif args.action == "read_eeprom":
            device.cpu_reset(False)
            data = device.read_eeprom(args.address, args.length, args.address_width)
            output_data(args.format, args.file, data, args.address)

        elif args.action == "write_eeprom":
            data = input_data(args.format, args.file, args.data, args.offset)
            device.cpu_reset(False)
            for address, chunk in data:
                device.write_eeprom(address, chunk, args.address_width)

    except usb1.USBErrorPipe:
        if args.action in ["read_eeprom", "write_eeprom"]:
            raise SystemExit("Command not acknowledged (wrong address width?)")
        else:
            raise SystemExit("Command not acknowledged")

    except usb1.USBErrorTimeout:
        if args.action in ["read_eeprom", "write_eeprom"]:
            raise SystemExit("Command timeout (bootloader not loaded?)")
        else:
            raise SystemExit("Command timeout")


if __name__ == "__main__":
    main()
