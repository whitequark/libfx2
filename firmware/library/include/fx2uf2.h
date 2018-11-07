#ifndef FX2UF2_H
#define FX2UF2_H

#include <stdbool.h>
#include <stdint.h>

/// Configuration of USB UF2 interface.
struct uf2_configuration {
  /// Total number of sectors on the virtual USB Mass Storage device. The optimal number depends
  /// on the size of the firmware, but in general, 65536 sectors (32 MiB) is a reasonable size.
  uint32_t total_sectors;

  /// Contents of the INFO_UF2.TXT file on the virtual USB Mass Storage device. Should be at
  /// most one sector (512 bytes) long.
  __code const char *info_uf2_txt;

  /// Contents of the INDEX.HTM file on the virtual USB Mass Storage device. Should be at
  /// most one sector (512 bytes) long.
  __code const char *index_htm;

  /// Size of the firmware. Currently rounded up to the nearest UF2 payload size, 256,
  /// but this should not be relied on.
  uint32_t firmware_size;

  /// Firmware read function. This function should accept any ``address`` and ``length``
  /// arguments that read bytes between ``0`` and ``firmware_size`` (after rounding).
  /// It should return ``true`` if the firmware could be read, and ``false`` otherwise.
  bool (*firmware_read )(uint32_t address, __xdata uint8_t *data, uint16_t length) __reentrant;

  /// Firmware write function. This function is passed the UF2 blocks directly without
  /// further verification of ``address`` and ``length`` against ``firmware_size``.
  /// It should return ``true`` if the firmware could be written, and ``false`` otherwise.
  bool (*firmware_write)(uint32_t address, __xdata uint8_t *data, uint16_t length) __reentrant;
};

typedef __code const struct uf2_configuration
  uf2_configuration_c;

#ifndef DOXYGEN
bool uf2_scsi_command (uint8_t lun, __xdata uint8_t *command, uint8_t length) __reentrant;
bool uf2_scsi_data_out(uint8_t lun, __xdata const uint8_t *data, uint16_t length) __reentrant;
bool uf2_scsi_data_in (uint8_t lun, __xdata uint8_t *data, uint16_t length) __reentrant;

bool uf2_fat_read (uint32_t lba, __xdata uint8_t *data);
bool uf2_fat_write(uint32_t lba, __xdata const uint8_t *data);
#endif

/**
 * The global UF2 configuration, defined in the application code.
 * It only makes sense for a single device to expose a single UF2 interface at a time.
 */
extern uf2_configuration_c uf2_config;

#endif
