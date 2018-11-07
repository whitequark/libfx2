import math
import sys
import os
import io
import re
import struct
import collections
import argparse
import textwrap
import crcmod
import usb1

from . import VID_CYPRESS, PID_FX2, FX2Config, FX2Device, FX2DeviceError
from .format import input_data, output_data, diff_data


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
    def usb_id(arg):
        if re.match(r"^([0-9a-f]{1,4})$", arg, re.I) and int(arg, 16) not in (0x0000, 0xffff):
            return int(arg, 16)
        else:
            raise argparse.ArgumentTypeError("{} is not an USB ID".format(arg))

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

    def power_of_two(arg):
        size = int_with_base(arg)
        size_log = math.log2(size)
        if int(size_log) != size_log:
            raise argparse.ArgumentTypeError("{} is not a power of 2".format(size))
        return int(size_log)

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
        help="read data from code RAM",
        description="Reads data from on-chip code RAM.\n" + ram_note)
    add_read_args(p_read_ram)

    p_write_ram = subparsers.add_parser("write_ram",
        formatter_class=TextHelpFormatter,
        help="write data to code RAM",
        description="Writes data to on-chip code RAM.\n" + ram_note)
    add_write_args(p_write_ram)

    p_read_xram = subparsers.add_parser("read_xram",
        formatter_class=TextHelpFormatter,
        help="read data from code RAM, external RAM, or registers",
        description="Reads data from RAM using the `movx` instruction.\n" + ram_note)
    add_read_args(p_read_xram)

    p_write_xram = subparsers.add_parser("write_xram",
        formatter_class=TextHelpFormatter,
        help="write data to code RAM, external RAM, or registers",
        description="Writes data to RAM using the `movx` instruction.\n" + ram_note)
    add_write_args(p_write_xram)

    def add_eeprom_args(parser):
        parser.add_argument(
            "-W", "--address-width", metavar="WIDTH", type=int, choices=[1, 2], default=2,
            help="EEPROM address width in bytes")

    def add_eeprom_write_args(parser):
        parser.add_argument(
            "-p", "--page-size", metavar="SIZE", type=power_of_two, default=1,
            help="power-of-two EEPROM page size (default: %(default)d)")

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
    add_eeprom_write_args(p_write_eeprom)

    p_reenumerate = subparsers.add_parser("reenumerate",
        formatter_class=TextHelpFormatter,
        help="re-enumerate",
        description="Simulates device disconnection and reconnection.\n" + bootloader_note)

    def add_program_args(parser):
        parser.add_argument(
            "-V", "--vid", dest="vendor_id", metavar="ID", type=usb_id, default=VID_CYPRESS,
            help="USB vendor ID (default: %(default)04x)")
        parser.add_argument(
            "-P", "--pid", dest="product_id", metavar="ID", type=usb_id, default=PID_FX2,
            help="USB product ID (default: %(default)04x)")
        parser.add_argument(
            "-D", "--did", dest="device_id", metavar="ID", type=usb_id, default=0x0000,
            help="USB device ID (default: %(default)04x)")
        parser.add_argument(
            "-N", "--disconnect", dest="disconnect", default=False, action="store_true",
            help="do not automatically enumerate on startup")
        parser.add_argument(
            "-F", "--fast", dest="i2c_400khz", default=False, action="store_true",
            help="use 400 kHz clock for loading firmware via I2C")

    p_program = subparsers.add_parser("program",
        formatter_class=TextHelpFormatter,
        help="program USB IDs or firmware",
        description="Writes USB VID, PID, and DID, and if specified, firmware, "
        "into boot EEPROM.\n" + bootloader_note)
    add_eeprom_args(p_program)
    add_eeprom_write_args(p_program)
    add_program_args(p_program)
    p_program.add_argument(
        "-f", "--firmware", metavar="FILENAME", type=argparse.FileType("rb"),
        help="read firmware from the specified file")

    p_update = subparsers.add_parser("update",
        formatter_class=TextHelpFormatter,
        help="update USB IDs or firmware",
        description="Writes USB VID, PID, DID, boot options, and if specified, firmware, "
        "into boot EEPROM, without changing any omitted parameters.\n" + bootloader_note)
    add_eeprom_args(p_update)
    add_eeprom_write_args(p_update)
    p_update.add_argument(
        "-V", "--vid", dest="vendor_id", metavar="ID", type=usb_id,
        help="USB vendor ID")
    p_update.add_argument(
        "-P", "--pid", dest="product_id", metavar="ID", type=usb_id,
        help="USB product ID")
    p_update.add_argument(
        "-D", "--did", dest="device_id", metavar="ID", type=usb_id,
        help="USB device ID")
    p_update.add_argument(
        "-N", "--disconnect", default=None, dest="disconnect", action="store_true",
        help="do not automatically enumerate on startup")
    p_update.add_argument(
        "-E", "--no-disconnect", default=None, dest="disconnect", action="store_false",
        help="do automatically enumerate on startup")
    p_update.add_argument(
        "-F", "--fast", default=None, dest="i2c_400khz", action="store_true",
        help="use 400 kHz clock for loading firmware via I2C")
    p_update.add_argument(
        "-S", "--slow", default=None, dest="i2c_400khz", action="store_false",
        help="use 100 kHz clock for loading firmware via I2C")
    g_update_firmware = p_update.add_mutually_exclusive_group()
    g_update_firmware.add_argument(
        "-f", "--firmware", metavar="FILENAME", type=argparse.FileType("rb"),
        help="read firmware from the specified file")
    g_update_firmware.add_argument(
        "-n", "--no-firmware", default=False, action="store_true",
        help="remove any firmware present")

    p_dump = subparsers.add_parser("dump",
        formatter_class=TextHelpFormatter,
        help="read out USB IDs or firmware",
        description="Reads USB VID, PID, DID, boot options, and if present, firmware, "
        "from boot EEPROM.\n" + bootloader_note)
    add_eeprom_args(p_dump)
    p_dump.add_argument(
        "-f", "--firmware", metavar="FILENAME", type=argparse.FileType("wb"),
        help="write firmware to the specified file")

    p_uf2 = subparsers.add_parser("uf2",
        formatter_class=TextHelpFormatter,
        help="prepare UF2 firmware update images",
        description="Assembles USB VID, PID, DID, boot options and firmware "
        "into an image that can be flashed into the boot EEPROM using "
        "the UF2 firmware update protocol.")
    add_program_args(p_uf2)
    p_uf2.add_argument(
        "firmware_file", metavar="FIRMWARE-FILE", type=argparse.FileType("rb"),
        help="read firmware from the specified file")
    p_uf2.add_argument(
        "uf2_file", metavar="UF2-FILE", type=argparse.FileType("wb"),
        help="write UF2 firmware update image to the specified file")

    p_dfu = subparsers.add_parser("dfu",
        formatter_class=TextHelpFormatter,
        help="prepare DFU firmware update images",
        description="Assembles USB VID, PID, DID, boot options and firmware "
        "into an image that can be flashed into the boot EEPROM using "
        "the standard Device Firmware Update protocol.")
    add_program_args(p_dfu)
    p_dfu.add_argument(
        "--dfu-pid", dest="dfu_product_id", metavar="ID", type=usb_id,
        help="DFU mode USB product ID (default: firmware product ID)")
    p_dfu.add_argument(
        "firmware_file", metavar="FIRMWARE-FILE", type=argparse.FileType("rb"),
        help="read firmware from the specified file")
    p_dfu.add_argument(
        "dfu_file", metavar="UF2-FILE", type=argparse.FileType("wb"),
        help="write DFU image to the specified file")

    return parser


def read_entire_boot_eeprom(device, address_width):
    # We don't know how large the EEPROM is, so we use a heuristic tailored
    # for the C2 load: if we detect a chunk identical to the first chunk
    # *or* chunk consisting only of erased bytes, we stop.
    addr = 0
    data = bytearray()
    while addr < 0x10000: # never larger than 64k
        chunk = device.read_boot_eeprom(addr, 0x100, address_width)
        if addr == 0:
            first_chunk = chunk
        elif chunk == first_chunk:
            break
        if re.match(rb"\xff{256}", chunk):
            break
        else:
            data += chunk
            addr += len(chunk)
    return data


def main():
    resource_dir = os.path.dirname(os.path.abspath(__file__))
    args = get_argparser().parse_args()

    if args.action in ("uf2", "dfu"):
        device = None
    else:
        try:
            vid, pid = args.device
            device = FX2Device(vid, pid)
        except FX2DeviceError as e:
            raise SystemExit(e)

    try:
        if device is not None:
            if args.bootloader:
                bootloader_ihex = os.path.join(resource_dir, "boot-cypress.ihex")
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

        elif args.action == "read_xram":
            device.cpu_reset(False)
            data = device.read_ext_ram(args.address, args.length)
            output_data(args.file, data, args.format, args.address)

        elif args.action == "write_xram":
            data = input_data(args.file or args.data, args.format, args.offset)
            device.cpu_reset(False)
            for address, chunk in data:
                device.write_ext_ram(address, chunk)

        elif args.action == "read_eeprom":
            device.cpu_reset(False)
            data = device.read_boot_eeprom(args.address, args.length, args.address_width)
            output_data(args.file, data, args.format, args.address)

        elif args.action == "write_eeprom":
            data = input_data(args.file or args.data, args.format, args.offset)
            device.cpu_reset(False)
            for address, chunk in data:
                device.write_boot_eeprom(address, chunk, args.address_width,
                                         chunk_size=min(args.page_size * 4, 64),
                                         page_size=args.page_size)

        elif args.action == "reenumerate":
            device.reenumerate()

        elif args.action == "program":
            device.cpu_reset(False)

            if args.firmware:
                firmware = input_data(args.firmware, args.format)
            else:
                firmware = []

            config = FX2Config(args.vendor_id, args.product_id, args.device_id,
                               args.disconnect, args.i2c_400khz)
            for address, chunk in firmware:
                config.append(address, chunk)
            image = config.encode()

            device.write_boot_eeprom(0, image, args.address_width,
                                     chunk_size=min(args.page_size * 4, 64),
                                     page_size=args.page_size)

            image = device.read_boot_eeprom(0, len(image), args.address_width)
            if FX2Config.decode(image) != config:
                raise SystemExit("Verification failed")

        elif args.action == "update":
            device.cpu_reset(False)

            if args.firmware:
                firmware = input_data(args.firmware, args.format)
            elif args.no_firmware:
                firmware = []
            else:
                firmware = None

            old_image = read_entire_boot_eeprom(device, args.address_width)

            config = FX2Config.decode(old_image)
            if args.vendor_id  is not None:
                config.vendor_id  = args.vendor_id
            if args.product_id is not None:
                config.product_id = args.product_id
            if args.device_id  is not None:
                config.device_id  = args.device_id
            if args.disconnect is not None:
                config.disconnect = args.disconnect
            if args.i2c_400khz is not None:
                config.i2c_400khz = args.i2c_400khz
            if firmware is not None:
                config.firmware = []
                for (addr, chunk) in firmware:
                    config.append(addr, chunk)

            new_image = config.encode()

            for (addr, chunk) in diff_data(old_image, new_image):
                device.write_boot_eeprom(addr, chunk, args.address_width,
                                         chunk_size=min(args.page_size * 4, 64),
                                         page_size=args.page_size)

            new_image = device.read_boot_eeprom(0, len(new_image), args.address_width)
            if FX2Config.decode(new_image) != config:
                raise SystemExit("Verification failed")

        elif args.action == "dump":
            device.cpu_reset(False)

            image = read_entire_boot_eeprom(device, args.address_width)

            config = FX2Config.decode(image)
            if not config:
                print("Device erased")
            else:
                print("USB VID:    {:04x}\n"
                      "    PID:    {:04x}\n"
                      "    DID:    {:04x}\n"
                      "Disconnect: {}\n"
                      "I2C clock:  {}\n"
                      "Firmware:   {}"
                      .format(config.vendor_id,
                              config.product_id,
                              config.device_id,
                              "enabled" if config.disconnect else "disabled",
                              "400 kHz" if config.i2c_400khz else "100 kHz",
                              "present" if len(config.firmware) > 0 else "absent"),
                      file=sys.stderr)

                if args.firmware and len(config.firmware) > 0:
                    output_data(args.firmware, config.firmware, args.format)

        elif args.action == "uf2":
            config = FX2Config(args.vendor_id, args.product_id, args.device_id,
                               args.disconnect, args.i2c_400khz)
            for address, chunk in input_data(args.firmware_file, args.format):
                config.append(address, chunk)
            image = config.encode()

            UF2_MAGIC_START_0           = 0x0A324655
            UF2_MAGIC_START_1           = 0x9E5D5157
            UF2_MAGIC_END               = 0x0AB16F30

            UF2_FLAG_NOT_MAIN_FLASH     = 0x00000001
            UF2_FLAG_FILE_CONTAINER     = 0x00001000
            UF2_FLAG_FAMILY_ID_PRESENT  = 0x00002000

            UF2_FAMILY_CYPRESS_FX2      = 0x5a18069b

            block_size = 256
            num_blocks = (len(image) + block_size - 1) // block_size
            block_no   = 0
            while len(image) > 0:
                uf2_block  = struct.pack("<IIIIIIII",
                    UF2_MAGIC_START_0,
                    UF2_MAGIC_START_1,
                    UF2_FLAG_FAMILY_ID_PRESENT,
                    block_no * block_size,
                    block_size,
                    block_no,
                    num_blocks,
                    UF2_FAMILY_CYPRESS_FX2)
                uf2_block += image[:block_size]
                uf2_block += b"\x00" * (512 - 4 - len(uf2_block))
                uf2_block += struct.pack("<I",
                    UF2_MAGIC_END)
                args.uf2_file.write(uf2_block)

                image     = image[block_size:]
                block_no += 1

        elif args.action == "dfu":
            config = FX2Config(args.vendor_id, args.product_id, args.device_id,
                               args.disconnect, args.i2c_400khz)
            for address, chunk in input_data(args.firmware_file, args.format):
                config.append(address, chunk)
            image = config.encode()

            suffix = struct.pack(">BBBBHHHH",
                struct.calcsize(">LBBBBHHHH"),
                0x44, 0x46, 0x55, # ucDfuSignature
                0x0100, # bcdDFU
                args.vendor_id,
                args.dfu_product_id or args.product_id,
                args.device_id)
            image += bytes(reversed(suffix))

            crc = crcmod.Crc(poly=0x104c11db7, initCrc=0xffffffff)
            crc.update(image)
            image += struct.pack("<L", crc.crcValue)

            args.dfu_file.write(image)

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

    finally:
        if device is not None:
            device.usb_context.close()


if __name__ == "__main__":
    main()
