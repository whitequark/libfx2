#ifndef FAT_H
#define FAT_H

struct fat16_boot_sector {
  uint8_t  jump_to_bootstrap[3];
  uint8_t  oem_name_version[8];
  uint16_t bytes_per_sector;
  uint8_t  sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t  fat_copies;
  uint16_t root_entries;
  uint16_t _reserved0;
  uint8_t  media_descriptor;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors;
  uint8_t  logical_drive_number;
  uint8_t  dirty;
  uint8_t  extended_signature;
  uint32_t serial_number;
  uint8_t  volume_label[11];
  uint8_t  filesystem_type[8];
  uint8_t  bootstrap[448];
  uint8_t  signature[2];
};

struct fat_directory_entry {
  uint8_t  filename_ext[11];
  uint8_t  read_only:1;
  uint8_t  hidden:1;
  uint8_t  system:1;
  uint8_t  volume_label:1;
  uint8_t  subdirectory:1;
  uint8_t  archive:1;
  uint8_t  _reserved0:2;
  uint8_t  _reserved1[8];
  struct {
    uint16_t secondsx2:5;
    uint16_t minutes:6;
    uint16_t hours:5;
    uint16_t day:5;
    uint16_t month:4;
    uint16_t year:7; // since 1980
  } timestamp;
  uint16_t first_cluster;
  uint32_t size;
};

#endif
