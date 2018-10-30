#include <fx2lib.h>
#include <string.h>
#include "uf2.h"
#include "fat.h"

#define UF2_MAGIC_START_0           0x0A324655
#define UF2_MAGIC_START_1           0x9E5D5157
#define UF2_MAGIC_END               0x0AB16F30

#define UF2_FLAG_NOT_MAIN_FLASH     0x00000001
#define UF2_FLAG_FILE_CONTAINER     0x00001000
#define UF2_FLAG_FAMILY_ID_PRESENT  0x00002000

#define UF2_FAMILY_CYPRESS_FX2      0x5a18069b

struct uf2_block {
  uint32_t magic_start_0;
  uint32_t magic_start_1;
  uint32_t flags;
  uint32_t target_addr;
  uint32_t payload_size;
  uint32_t block_no;
  uint32_t num_blocks;
  union {
    uint32_t file_size;
    uint32_t family_id;
  };
  uint8_t  data[476];
  uint32_t magic_end;
};

#define BYTES_PER_SECTOR  512
#define FAT_OFFSET        1
#define FAT_SECTORS       (uf2_config.total_sectors >> 8)
#define ROOT_OFFSET       (FAT_OFFSET + FAT_SECTORS)
#define ROOT_SECTORS      1
#define ROOT_ENTRIES      (ROOT_SECTORS << 4)
#define DATA_OFFSET       (ROOT_OFFSET + ROOT_SECTORS)
#define DATA_SECTORS      (uf2_config.total_sectors - DATA_OFFSET)

#define FIRMWARE_BLOCKS   (uf2_config.firmware_size >> 7)
#define FIRMWARE_SECTORS  (uf2_config.firmware_size >> 8)

enum clusters {
  CLUSTER_MEDIA_DESCRIPTOR,
  CLUSTER_EOF_MARKER,
  CLUSTER_INFO_UF2_TXT,
  CLUSTER_INDEX_HTM,
  CLUSTER_STATUS_TXT,

  CLUSTER_CURRENT_UF2       = 0x0100,

  CLUSTER_LAST              = 0xfff8
};

static void fill_entry(__xdata struct fat_directory_entry *entry,
                       __code const char *filename_ext, bool read_only,
                       uint16_t first_cluster, uint32_t size) {
  xmemcpy(entry->filename_ext, (__xdata void *)filename_ext, 11);
  entry->read_only = read_only;
  entry->timestamp.year  = 89;
  entry->timestamp.month = 4;
  entry->timestamp.day   = 20;
  entry->timestamp.hours = 8;
  entry->timestamp.minutes = 4;
  entry->timestamp.secondsx2 = 1;
  entry->first_cluster = first_cluster;
  entry->size = size;
}

static __code const char *status_txt =
  "Blocks accepted: 0x????\n"
  "       rejected: 0x????\n"
  "        ignored: 0x????\n"
  "        flashed: 0x????\n"
  "         failed: 0x????\n"
  "\n"
  "Key:\n"
  "- accepted: valid magic and parameters\n"
  "- rejected: valid magic but not parameters\n"
  "-  ignored: invalid magic (usually filesystem metadata)\n"
  "-  flashed: successfully written to NVM\n"
  "-   failed: timeout writing to NVM\n";

static __code const char *hex =
  "0123456789ABCDEF";

static uint16_t stat_accepted;
static uint16_t stat_rejected;
static uint16_t stat_ignored;
static uint16_t stat_flashed;
static uint16_t stat_failed;

static void fill_hex(__xdata uint8_t *buffer, uint16_t stat) {
  buffer[0] = hex[(stat >> 12) & 0xf];
  buffer[1] = hex[(stat >>  8) & 0xf];
  buffer[2] = hex[(stat >>  4) & 0xf];
  buffer[3] = hex[(stat >>  0) & 0xf];
}

static void fill_status(__xdata uint8_t *buffer) {
  xmemcpy(buffer, (__xdata void *)status_txt, strlen(status_txt));
  fill_hex(&buffer[24 * 0 + 19], stat_accepted);
  fill_hex(&buffer[24 * 1 + 19], stat_rejected);
  fill_hex(&buffer[24 * 2 + 19], stat_ignored);
  fill_hex(&buffer[24 * 3 + 19], stat_flashed);
  fill_hex(&buffer[24 * 4 + 19], stat_failed);
}

bool uf2_fat_read(uint16_t lba, __xdata uint8_t *data) {
  if(lba == 0) {
    // Boot sector.
    __xdata struct fat16_boot_sector *boot =
      (__xdata struct fat16_boot_sector *)data;
    xmemclr(data, BYTES_PER_SECTOR);

    xmemcpy(boot->oem_name_version, (__xdata void *)"MSWIN4.1",     8);
    boot->bytes_per_sector    = BYTES_PER_SECTOR;
    boot->sectors_per_cluster = 1;
    boot->reserved_sectors    = FAT_OFFSET;
    boot->fat_copies          = 1;
    boot->root_entries        = ROOT_ENTRIES;
    boot->media_descriptor    = 0xf8;
    boot->sectors_per_fat     = FAT_SECTORS;
    boot->total_sectors       = uf2_config.total_sectors;
    boot->extended_signature  = 0x29;
    xmemcpy(boot->volume_label,     (__xdata void *)"CYPRESS UF2",  11);
    xmemcpy(boot->filesystem_type,  (__xdata void *)"FAT16   ",     8);
    boot->signature[0]        = 0x55;
    boot->signature[1]        = 0xaa;

    return true;
  }

  if(lba >= FAT_OFFSET && lba < FAT_OFFSET + FAT_SECTORS) {
    // File allocation table.
    uint16_t offset = (lba - FAT_OFFSET) << 8;
    __xdata uint16_t *next_cluster = (__xdata uint16_t *)data;
    uint16_t index;
    xmemclr(data, BYTES_PER_SECTOR);

    if(offset == 0) {
      next_cluster[CLUSTER_MEDIA_DESCRIPTOR]  = 0xfff8;
      next_cluster[CLUSTER_EOF_MARKER]        = CLUSTER_LAST;
      next_cluster[CLUSTER_INFO_UF2_TXT]      = CLUSTER_LAST;
      next_cluster[CLUSTER_INDEX_HTM]         = CLUSTER_LAST;
      next_cluster[CLUSTER_STATUS_TXT]        = CLUSTER_LAST;
    }

    if(offset >= CLUSTER_CURRENT_UF2 &&
       offset <= CLUSTER_CURRENT_UF2 + FIRMWARE_SECTORS + 256) {
      for(index = offset; index < CLUSTER_CURRENT_UF2 + FIRMWARE_SECTORS; index++) {
        if(index == CLUSTER_CURRENT_UF2 + FIRMWARE_SECTORS - 1) {
          next_cluster[index - offset] = CLUSTER_LAST;
        } else {
          next_cluster[index - offset] = index + 1;
        }
      }
    }

    return true;
  }

  if(lba >= ROOT_OFFSET && lba < ROOT_OFFSET + ROOT_SECTORS) {
    // Root directory.
    uint16_t offset = (lba - ROOT_OFFSET) >> 4;
    __xdata struct fat_directory_entry *root_entry = (__xdata struct fat_directory_entry *)data;
    xmemclr(data, BYTES_PER_SECTOR);

    if(offset == 0) {
      fill_entry(&root_entry[0], "INFO_UF2TXT",
                 /*read_only=*/true,
                 /*first_cluster=*/CLUSTER_INFO_UF2_TXT,
                 /*size=*/strlen(uf2_config.info_uf2_txt));
      fill_entry(&root_entry[1], "INDEX   HTM",
                 /*read_only=*/true,
                 /*first_cluster=*/CLUSTER_INDEX_HTM,
                 /*size=*/strlen(uf2_config.index_htm));
      fill_entry(&root_entry[2], "STATUS  TXT",
                 /*read_only=*/true,
                 /*first_cluster=*/CLUSTER_STATUS_TXT,
                 /*size=*/strlen(status_txt));
      fill_entry(&root_entry[3], "CURRENT UF2",
                 /*read_only=*/false,
                 /*first_cluster=*/CLUSTER_CURRENT_UF2,
                 /*size=*/uf2_config.firmware_size * 2);
    }

    return true;
  }

  if(lba >= DATA_OFFSET && lba < DATA_OFFSET + DATA_SECTORS) {
    // Data clusters.
    __xdata uint16_t cluster = (lba - DATA_OFFSET);
    cluster += 2;
    // If the above is rewritten as (lba - DATA_OFFSET) + 2, the result is wrong on sdcc 3.7.0.
    // Compiler bug?

    if(cluster == CLUSTER_INFO_UF2_TXT) {
      xmemcpy(data, (__xdata void *)uf2_config.info_uf2_txt,
              strlen(uf2_config.info_uf2_txt));
    } else if(cluster == CLUSTER_INDEX_HTM) {
      xmemcpy(data, (__xdata void *)uf2_config.index_htm,
              strlen(uf2_config.index_htm));
    } else if(cluster == CLUSTER_STATUS_TXT) {
      fill_status(data);
    } else if(cluster >= CLUSTER_CURRENT_UF2 &&
              cluster < CLUSTER_CURRENT_UF2 + FIRMWARE_SECTORS) {
      __xdata struct uf2_block *block = (__xdata struct uf2_block *)data;
      xmemclr(data, BYTES_PER_SECTOR);

      block->magic_start_0 = UF2_MAGIC_START_0;
      block->magic_start_1 = UF2_MAGIC_START_1;
      block->flags         = UF2_FLAG_FAMILY_ID_PRESENT;
      block->block_no      = cluster - CLUSTER_CURRENT_UF2;
      block->num_blocks    = FIRMWARE_BLOCKS;
      block->payload_size  = 256;
      block->target_addr   = block->block_no * block->payload_size;
      block->family_id     = UF2_FAMILY_CYPRESS_FX2;
      block->magic_end     = UF2_MAGIC_END;

      if(!uf2_config.firmware_read(block->target_addr, (__xdata uint8_t *)&block->data,
                                   block->payload_size))
        return false;
    } else {
      xmemclr(data, BYTES_PER_SECTOR);
    }

    return true;
  }

  return false;
}

bool uf2_fat_write(uint16_t lba, __xdata const uint8_t *data) {
  if(lba >= DATA_OFFSET && lba < DATA_OFFSET + DATA_SECTORS) {
    __xdata const struct uf2_block *block = (__xdata struct uf2_block *)data;
    if(!(block->magic_start_0 == UF2_MAGIC_START_0 &&
         block->magic_start_1 == UF2_MAGIC_START_1 &&
         block->magic_end     == UF2_MAGIC_END)) {
      stat_ignored++;
      return true;
    }

    if(!(block->flags     == UF2_FLAG_FAMILY_ID_PRESENT &&
         block->family_id == UF2_FAMILY_CYPRESS_FX2)) {
      stat_rejected++;
      return true;
    }

    stat_accepted++;

    if(uf2_config.firmware_write(block->target_addr, (__xdata uint8_t *)&block->data,
                                 block->payload_size)) {
      stat_flashed++;
    } else {
      stat_failed++;
    }
    return true;
  }

  stat_ignored++;
  return true;
}
