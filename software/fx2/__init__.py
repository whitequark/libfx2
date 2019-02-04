import struct
import usb1
import select
from . import poll_wrapper


__all__ = ["FX2Config", "FX2Device", "FX2DeviceError"]


VID_CYPRESS = 0x04B4
PID_FX2     = 0x8613

CFG_DISCON  = 1 << 6
CFG_400KHZ  = 1 << 0

REG_CPUCS   = 0xE600

REQ_RAM        = 0xA0
REQ_EEPROM_SB  = 0xA2
REQ_EXT_RAM    = 0xA3
REQ_RENUMERATE = 0xA8
REQ_EEPROM_DB  = 0xA9
REQ_PAGE_SIZE  = 0xB0


class FX2Config:
    """
    Cypress FX2 EEPROM configuration data.

    vendor_id : int
        USB vendor ID, 16 bits.
    product_id : int
        USB product ID, 16 bits.
    device_id : int
        USB device ID, 16 bits in binary-coded decimal.
    disconnect : bool
        If ``True``, do not enumerate on startup.
        If ``False``, enumerate as a default FX2 device with specified VID/PID/DID.
    i2c_400khz : bool
        If ``True``, use 400 kHz I2C clock to read firmware from EEPROM.
        If ``False``, use 100 kHz clock.
    firmware : list
        Firmware to be loaded into on-chip program/data RAM.
        If empty, "C0 load" is used; if not, "C2 load" is used, and the command
        to bring the CPU out of reset is inserted automatically.
    """
    def __init__(self, vendor_id=VID_CYPRESS, product_id=PID_FX2, device_id=0x0000,
                 disconnect=False, i2c_400khz=False):
        self.vendor_id  = vendor_id
        self.product_id = product_id
        self.device_id  = device_id
        self.disconnect = disconnect
        self.i2c_400khz = i2c_400khz
        self.firmware   = []

    def append(self, addr, chunk):
        """
        Append a command to load ``chunk`` at ``addr`` into on-chip program/data RAM
        on device startup.
        """
        first, last = addr, addr + len(chunk)
        assert (first >= 0x0000 and last <= 0x4000 or
                first >= 0xE000 and last <= 0xE200)

        while len(chunk) > 0:
            # Split into 1023-byte chunks, since the hardware is not able to handle
            # length field over 10 bit.
            self.firmware.append((addr, bytes(chunk[:1023])))
            addr += 1023
            chunk = chunk[1023:]

    def encode(self, max_size=None):
        """
        Convert configuration to an image that can be loaded into an EEPROM.

        Returns the EEPROM image if the resulting configuration is smaller than ``max_size``,
        or raises :class:`ValueError` if it is not.
        """
        data = bytearray()
        data.append(0xC0 if len(self.firmware) == 0 else 0xC2)

        # Windows wouldn't enumerate a device with these VID/PID.
        assert (self.vendor_id not in (0x0000, 0xFFFF) and
                self.product_id not in (0x0000, 0xFFFF))
        data += struct.pack("<HHH", self.vendor_id, self.product_id, self.device_id)

        data.append((CFG_DISCON if self.disconnect else 0) |
                    (CFG_400KHZ if self.i2c_400khz else 0))

        if len(self.firmware) > 0:
            for (addr, chunk) in self.firmware:
                assert len(chunk) > 0 and len(chunk) < 1024
                data += struct.pack(">HH", len(chunk), addr)
                data += chunk

            data += struct.pack(">HHb", 0x8001, REG_CPUCS, 0)

        if max_size is not None and len(data) > max_size:
            raise ValueError("configuration image exceeds maximum size")

        return data

    @classmethod
    def decode(cls, data, partial=False):
        """
        Parse configuration from an image loaded from an EEPROM.

        Returns ``None`` if the EEPROM image is empty (the EEPROM was erased),
        :class:`FX2Config` if it contains a valid configuration, or
        raises :class:`ValueError` if it does not.

        If ``partial`` is ``True``, only requires any data records present
        to be complete; if it is ``False``, it is an error unless the image
        contains the final data record.
        """
        if data[0] == 0xFF:
            return None
        elif data[0] == 0xC0:
            has_firmware = False
        elif data[0] == 0xC2:
            has_firmware = True
        else:
            raise ValueError("Invalid load command")

        vendor_id, product_id, device_id, config_byte = \
            struct.unpack_from("<HHHb", data, offset=1)
        if (vendor_id in (0x0000, 0xFFFF) or
                product_id in (0x0000, 0xFFFF)):
            raise ValueError("Invalid VID/PID")

        disconnect = (config_byte & CFG_DISCON) != 0
        i2c_400khz = (config_byte & CFG_400KHZ) != 0

        config = cls(vendor_id, product_id, device_id, disconnect, i2c_400khz)

        if has_firmware:
            offset = 8
            while not partial or offset < len(data):
                if offset + 4 > len(data):
                    raise ValueError("Truncated data record")

                length, addr = struct.unpack_from(">HH", data, offset)
                offset += 4

                if length & 0x8000:
                    break

                if offset + length > len(data):
                    raise ValueError("Truncated data record")

                config.firmware.append((addr, data[offset:offset + length]))
                offset += length

        return config

    def __eq__(self, other):
        return (self.vendor_id == other.vendor_id and
                self.product_id == other.product_id and
                self.device_id == other.device_id and
                self.disconnect == other.disconnect and
                self.i2c_400khz == other.i2c_400khz and
                self.firmware == other.firmware)

    def __ne__(self, other):
        return not (self == other)


class FX2DeviceError(Exception):
    """An exception raised on a communication error."""


class FX2Device:
    """
    A Cypress FX2 series device.

    The initializer of this class locates the device by provided VID:PID pair,
    or raises a :exc:`FX2DeviceError`.

    usb : usb1.USBDeviceHandle
        Raw USB device handle.
    """
    def __init__(self, vendor_id=VID_CYPRESS, product_id=PID_FX2):
        self.timeout = 1000

        try:
            self.usb_context = usb1.USBContext()
            self.usb_poller = None
            self.usb = self.usb_context.openByVendorIDAndProductID(vendor_id, product_id)
        except usb1.USBErrorAccess:
            raise FX2DeviceError("Cannot access device {:04x}:{:04x}"
                                 .format(vendor_id, product_id))
        if self.usb is None:
            raise FX2DeviceError("Device {:04x}:{:04x} not found"
                                 .format(vendor_id, product_id))

        try:
            self.usb.setAutoDetachKernelDriver(True)
        except usb1.USBErrorNotSupported:
            pass

    def control_read(self, request_type, request, value, index, length,
                     timeout=None):
        """
        Issue an USB control read request with timeout defaulting to ``self.timeout``.
        """
        if timeout is None:
            timeout = self.timeout
        return self.usb.controlRead(request_type, request, value, index, length, timeout)

    def control_write(self, request_type, request, value, index, data,
                      timeout=None):
        """
        Issue an USB control write request with timeout defaulting to ``self.timeout``.
        """
        if timeout is None:
            timeout = self.timeout
        self.usb.controlWrite(request_type, request, value, index, data, timeout)

    def bulk_read(self, endpoint, length, timeout=None):
        """
        Issue an USB bulk read request with timeout defaulting to ``self.timeout``.
        """
        if timeout is None:
            timeout = self.timeout
        return self.usb.bulkRead(endpoint, length, timeout)

    def bulk_write(self, endpoint, data, timeout=None):
        """
        Issue an USB bulk write request with timeout defaulting to ``self.timeout``.
        """
        if timeout is None:
            timeout = self.timeout
        self.usb.bulkWrite(endpoint, data, timeout)

    def create_poller(self, poller):
        """
        Integrate waiting for USB transfers into an event loop, by taking an object
        conforming to the Python ``poll`` interface and returning another conforming
        to the same interface.

        Note that if ``create_poller`` is called more than once, events will only be
        delivered to the last returned poller instance.
        """
        return usb1.USBPoller(self.usb_context, poll_wrapper.wrap_poller_for_libusb(poller))

    def get_poller(self, lower=lambda: select.poll()):
        """
        Return a poller instance associated with this device, creating it using
        ``create_poller(lower())`` if necessary.
        """
        if self.usb_poller is None:
            self.usb_poller = self.create_poller(lower())
        return self.usb_poller

    def poll(self, timeout=None):
        """
        Wait for USB transfers, as well as any other events registered on the poller
        returned by ``get_poller()``, with timeout defaulting to ``self.timeout``.
        """
        if timeout is None:
            timeout = self.timeout
        return self.get_poller().poll(timeout)

    def read_ram(self, addr, length):
        """
        Read ``length`` bytes at ``addr`` from internal RAM.
        Note that not all memory can be addressed this way; consult the TRM.
        """
        data = bytearray()
        while length > 0:
            chunk_length = min(length, 4096)
            if addr & 1: # unaligned
                data += self.control_read(usb1.REQUEST_TYPE_VENDOR,
                                          REQ_RAM, addr, 0, chunk_length + 1)[1:]
            else:
                data += self.control_read(usb1.REQUEST_TYPE_VENDOR,
                                          REQ_RAM, addr, 0, chunk_length)
            addr += chunk_length
            length -= chunk_length
        return data

    def write_ram(self, addr, data):
        """
        Write ``data`` to ``addr`` to internal RAM.
        Note that not all memory can be addressed this way; consult the TRM.
        """
        while len(data) > 0:
            chunk_length = min(len(data), 4096)
            self.control_write(usb1.REQUEST_TYPE_VENDOR, REQ_RAM, addr, 0, data[:chunk_length])
            addr += chunk_length
            data = data[chunk_length:]

    def cpu_reset(self, is_reset):
        """Bring CPU in or out of reset."""
        self.write_ram(REG_CPUCS, [1 if is_reset else 0])

    def load_ram(self, chunks):
        """
        Write ``chunks``, a list of ``(address, data)`` pairs, to internal RAM,
        and start the CPU core. See also ``write_ram``.
        """
        self.cpu_reset(True)
        for address, data in chunks:
            self.write_ram(address, data)
        self.cpu_reset(False)

    @staticmethod
    def _eeprom_cmd(addr_width):
        if addr_width == 1:
            return REQ_EEPROM_SB
        elif addr_width == 2:
            return REQ_EEPROM_DB
        else:
            raise ValueError("Address width {addr_width} is not supported"
                             .format(addr_width=addr_width))

    def read_boot_eeprom(self, addr, length, addr_width, chunk_size=0x100):
        """
        Read ``length`` bytes at ``addr`` from boot EEPROM in ``chunk_size`` chunks.

        Requires the second stage bootloader.
        """
        data = bytearray()
        while length > 0:
            chunk_length = min(length, chunk_size)
            data += self.control_read(usb1.REQUEST_TYPE_VENDOR,
                                      self._eeprom_cmd(addr_width), addr, 0, chunk_length)
            addr += chunk_length
            length -= chunk_length
        return data

    def write_boot_eeprom(self, addr, data, addr_width, chunk_size=0x10, page_size=0):
        """
        Write ``data`` to ``addr`` in boot EEPROM that has ``2 ** page_size`` byte pages
        in ``chunk_size`` chunks.

        Writing EEPROM is much slower than reading; for best performance, specify  ``page_size``
        per EEPROM datasheet, and set ``chunk_size`` to a small multiple of ``2 ** page_size``.
        Otherwise, timeouts may occur.

        Requires the second stage bootloader or a compatible firmware.
        """
        self.control_write(usb1.REQUEST_TYPE_VENDOR, REQ_PAGE_SIZE, page_size, 0, [])
        while len(data) > 0:
            chunk_length = min(len(data), chunk_size)
            self.control_write(usb1.REQUEST_TYPE_VENDOR,
                               self._eeprom_cmd(addr_width), addr, 0, data[:chunk_length])
            addr += chunk_length
            data = data[chunk_length:]

    def read_ext_ram(self, addr, length):
        """
        Read ``length`` bytes at ``addr`` in RAM using the ``movx`` instruction.
        Unlike ``read_ram``, this can access the entire address space.

        Requires the second stage bootloader.
        """
        return self.control_read(usb1.REQUEST_TYPE_VENDOR, REQ_EXT_RAM, addr, 0, length)

    def write_ext_ram(self, addr, data):
        """
        Write ``data`` to ``addr`` in RAM using the ``movx`` instruction.
        Unlike ``write_ram``, this can access the entire address space.

        Requires the second stage bootloader or a compatible firmware.
        """
        self.control_write(usb1.REQUEST_TYPE_VENDOR, REQ_EXT_RAM, addr, 0, data)

    def reenumerate(self):
        """
        Trigger re-enumeration.

        Requires the second stage bootloader or a compatible firmware.
        """
        self.control_write(usb1.REQUEST_TYPE_VENDOR, REQ_RENUMERATE, 0, 0, [])
