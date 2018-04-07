import usb1


__all__ = ['FX2Device', 'FX2DeviceError']


VID_CYPRESS = 0x04B4
PID_FX2     = 0x8613

CMD_RW_RAM       = 0xA0
CMD_RW_EEPROM_SB = 0xA2
CMD_RENUM        = 0xA8
CMD_RW_EEPROM_DB = 0xA9


class FX2DeviceError(Exception):
    """An exception raised on a communication error."""


class FX2Device:
    """
    A Cypress FX2 series device.

    The initializer of this class locates the device by provided VID:PID pair,
    or raises a :exc:`FX2DeviceError`.
    """
    def __init__(self, vid=VID_CYPRESS, pid=PID_FX2):
        self._timeout = 1000
        self._context = usb1.USBContext()
        self._device = self._context.openByVendorIDAndProductID(vid, pid)
        if self._device is None:
            raise FX2DeviceError("Device {:04x}:{:04x} not found".format(vid, pid))
        self._device.setAutoDetachKernelDriver(True)

        self._eeprom_size = None

    def _control_read(self, request_type, request, value, index, length,
                      timeout=None):
        if timeout is None:
            timeout = self._timeout
        return self._device.controlRead(request_type, request, value, index, length, timeout)

    def _control_write(self, request_type, request, value, index, data,
                      timeout=None):
        if timeout is None:
            timeout = self._timeout
        self._device.controlWrite(request_type, request, value, index, data, timeout)

    def read_ram(self, addr, length):
        """
        Read ``length`` bytes at ``addr`` from internal RAM.
        Note that not all memory can be addressed this way; consult the TRM.
        """
        if addr & 1: # unaligned
            return self._control_read(usb1.REQUEST_TYPE_VENDOR,
                                      CMD_RW_RAM, addr, 0, length + 1)[1:]
        else:
            return self._control_read(usb1.REQUEST_TYPE_VENDOR,
                                      CMD_RW_RAM, addr, 0, length)

    def write_ram(self, addr, data):
        """
        Write ``data`` to ``addr`` to internal RAM.
        Note that not all memory can be addressed this way; consult the TRM.
        """
        self._control_write(usb1.REQUEST_TYPE_VENDOR, CMD_RW_RAM, addr, 0, data)

    def cpu_reset(self, is_reset):
        """Bring CPU in or out of reset."""
        self.write_ram(0xE600, [1 if is_reset else 0])

    @staticmethod
    def _eeprom_cmd(addr_width):
        if addr_width == 1:
            return CMD_RW_EEPROM_SB
        elif addr_width == 2:
            return CMD_RW_EEPROM_DB
        else:
            raise ValueError("Address width {addr_width} is not supported"
                             .format(addr_width=addr_width))

    def read_eeprom(self, addr, length, addr_width):
        """Read ``length`` bytes at ``addr`` from boot EEPROM."""
        return self._control_read(usb1.REQUEST_TYPE_VENDOR,
                                  self._eeprom_cmd(addr_width), addr, 0, length)

    def write_eeprom(self, addr, data, addr_width):
        """Write ``data`` to ``addr`` in boot EEPROM."""
        self._control_write(usb1.REQUEST_TYPE_VENDOR,
                            self._eeprom_cmd(addr_width), addr, 0, data)

    def reenumerate(self):
        """Trigger re-enumeration."""
        self._control_write(usb1.REQUEST_TYPE_VENDOR, CMD_RENUM, 0, 0, [])
