#ifndef UF2_H
#define UF2_H

#include <stdbool.h>
#include <stdint.h>

struct uf2_configuration {
  uint16_t total_sectors;
  __code const char *info_uf2_txt;
  __code const char *index_htm;

  uint32_t firmware_size;
  bool (*firmware_read )(uint32_t address, __xdata uint8_t *data, uint16_t length) __reentrant;
  bool (*firmware_write)(uint32_t address, __xdata uint8_t *data, uint16_t length) __reentrant;
};

typedef __code const struct uf2_configuration
  uf2_configuration_c;

extern uf2_configuration_c uf2_config;

bool uf2_scsi_command (uint8_t lun, __xdata uint8_t *command, uint8_t length) __reentrant;
bool uf2_scsi_data_out(uint8_t lun, __xdata const uint8_t *data, uint16_t length) __reentrant;
bool uf2_scsi_data_in (uint8_t lun, __xdata uint8_t *data, uint16_t length) __reentrant;

bool uf2_fat_read (uint16_t lba, __xdata uint8_t *data);
bool uf2_fat_write(uint16_t lba, __xdata const uint8_t *data);

#endif
