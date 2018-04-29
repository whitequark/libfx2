import sys
import os
import io
import re
import collections
import argparse
import textwrap
import usb1

from . import VID_CYPRESS, PID_FX2, FX2Device, FX2DeviceError
from .format import input_data, output_data


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
            device.load_ram(input_data(open(bootloader_ihex)))
        elif args.stage2:
            device.load_ram(input_data(args.stage2))

        if args.action == "load":
            device.load_ram(input_data(args.firmware, args.format))

        elif args.action == "read_ram":
            device.cpu_reset(True)
            data = device.read_ram(args.address, args.length)
            output_data(args.file, data, args.format, args.address)

        elif args.action == "write_ram":
            data = input_data(args.file or args.data, args.format, args.offset)
            device.cpu_reset(True)
            for address, chunk in data:
                device.write_ram(address, chunk)

        elif args.action == "read_eeprom":
            device.cpu_reset(False)
            data = device.read_boot_eeprom(args.address, args.length, args.address_width)
            output_data(args.file, data, args.format, args.address)

        elif args.action == "write_eeprom":
            data = input_data(args.file or args.data, args.format, args.offset)
            device.cpu_reset(False)
            for address, chunk in data:
                device.write_boot_eeprom(address, chunk, args.address_width)

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

    except ValueError as e:
        raise SystemExit(str(e))


if __name__ == "__main__":
    main()
