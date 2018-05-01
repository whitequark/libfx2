import usb1


__all__ = ['FX2Device', 'FX2DeviceError']


VID_CYPRESS = 0x04B4
PID_FX2     = 0x8613

REQ_RW_RAM       = 0xA0
REQ_RW_EEPROM_SB = 0xA2
REQ_RENUMERATE   = 0xA8
REQ_RW_EEPROM_DB = 0xA9


class FX2DeviceError(Exception):
    """An exception raised on a communication error."""


class FX2Device:
    """
    A Cypress FX2 series device.

    The initializer of this class locates the device by provided VID:PID pair,
    or raises a :exc:`FX2DeviceError`.
    """
    def __init__(self, vid=VID_CYPRESS, pid=PID_FX2):
        self.timeout = 1000

        self._context = usb1.USBContext()
        try:
            self._device = self._context.openByVendorIDAndProductID(vid, pid)
        except usb1.USBErrorAccess:
            raise FX2DeviceError("Cannot access device {:04x}:{:04x}".format(vid, pid))
        if self._device is None:
            raise FX2DeviceError("Device {:04x}:{:04x} not found".format(vid, pid))
        self._device.setAutoDetachKernelDriver(True)

    def control_read(self, bmRequestType, bRequest, wValue, wIndex, wLength,
                     timeout=None):
        """
        Issue an USB control read request with timeout defaulting to ``self.timeout``.
        """
        if timeout is None:
            timeout = self.timeout
        return self._device.controlRead(bmRequestType, bRequest, wValue, wIndex, wLength, timeout)

    def control_write(self, bmRequestType, bRequest, wValue, wIndex, data,
                      timeout=None):
        """
        Issue an USB control write request with timeout defaulting to ``self.timeout``.
        """
        if timeout is None:
            timeout = self.timeout
        self._device.controlWrite(bmRequestType, bRequest, wValue, wIndex, data, timeout)

    def read_ram(self, addr, length):
        """
        Read ``length`` bytes at ``addr`` from internal RAM.
        Note that not all memory can be addressed this way; consult the TRM.
        """
        if addr & 1: # unaligned
            return self.control_read(usb1.REQUEST_TYPE_VENDOR,
                                     REQ_RW_RAM, addr, 0, length + 1)[1:]
        else:
            return self.control_read(usb1.REQUEST_TYPE_VENDOR,
                                     REQ_RW_RAM, addr, 0, length)

    def write_ram(self, addr, data):
        """
        Write ``data`` to ``addr`` to internal RAM.
        Note that not all memory can be addressed this way; consult the TRM.
        """
        self.control_write(usb1.REQUEST_TYPE_VENDOR, REQ_RW_RAM, addr, 0, data)

    def cpu_reset(self, is_reset):
        """Bring CPU in or out of reset."""
        self.write_ram(0xE600, [1 if is_reset else 0])

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
            return REQ_RW_EEPROM_SB
        elif addr_width == 2:
            return REQ_RW_EEPROM_DB
        else:
            raise ValueError("Address width {addr_width} is not supported"
                             .format(addr_width=addr_width))

    def read_boot_eeprom(self, addr, length, addr_width):
        """
        Read ``length`` bytes at ``addr`` from boot EEPROM.

        Requires the second stage bootloader.
        """
        return self.control_read(usb1.REQUEST_TYPE_VENDOR,
                                 self._eeprom_cmd(addr_width), addr, 0, length)

    def write_boot_eeprom(self, addr, data, addr_width):
        """
        Write ``data`` to ``addr`` in boot EEPROM.

        Requires the second stage bootloader or a compatible firmware.
        """
        self.control_write(usb1.REQUEST_TYPE_VENDOR,
                           self._eeprom_cmd(addr_width), addr, 0, data)

    def reenumerate(self):
        """
        Trigger re-enumeration.

        Requires the second stage bootloader or a compatible firmware.
        """
        self.control_write(usb1.REQUEST_TYPE_VENDOR, REQ_RENUMERATE, 0, 0, [])
