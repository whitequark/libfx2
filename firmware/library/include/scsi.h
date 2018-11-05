#ifndef SCSI_H
#define SCSI_H

#include <stdint.h>

// Command TEST UNIT READY

struct scsi_test_unit_ready {
  uint8_t  __reserved0[4];
  uint8_t  control;
};

// Command REQUEST SENSE

struct scsi_request_sense {
  uint8_t  desc:1;
  uint8_t  __reserved0:7;
  uint8_t  __reserved1[2];
  uint8_t  allocation_length;
  uint8_t  control;
};

// Command INQUIRY

struct scsi_inquiry {
  uint8_t  evpd:1;
  uint8_t  page_code;
  uint16_t allocation_length;
  uint8_t  control;
};

struct scsi_inquiry_data {
  uint8_t  peripheral_device_type:5;
  uint8_t  peripheral_qualifier:3;
  uint8_t  __reserved0:7;
  uint8_t  rmb:1;
  uint8_t  version;
  uint8_t  response_data_format:4;
  uint8_t  hi_sup:1;
  uint8_t  norm_aca:1;
  uint8_t  __obsolete1:2;
  uint8_t  additional_length;
  uint8_t  protect:1;
  uint8_t  __reserved2:2;
  uint8_t  _3pc:1;
  uint8_t  tpgs:2;
  uint8_t  acc:1;
  uint8_t  sccs:1;
  uint8_t  addr16:1;
  uint8_t  __obsolete3:2;
  uint8_t  m_chngr:1;
  uint8_t  multi_p:1;
  uint8_t  vs:1;
  uint8_t  enc_serv:1;
  uint8_t  b_que:1;
  uint8_t  vs2:1;
  uint8_t  cmd_que:1;
  uint8_t  __obsolete4:1;
  uint8_t  linked:1;
  uint8_t  sync:1;
  uint8_t  wbus16:1;
  uint8_t  __obsolete5:2;
  uint8_t  t10_vendor_identification[8];
  uint8_t  product_identification[16];
  uint8_t  product_revision_level[4];
};

// Command PREVENT ALLOW MEDIUM REMOVAL

struct scsi_prevent_allow_medium_removal {
  uint8_t  __reserved0[3];
  uint8_t  prevent:1;
  uint8_t  __reserved1:7;
  uint8_t  control;
};

// Command READ CAPACITY

struct scsi_read_capacity {
  uint8_t  __reserved0[8];
  uint8_t  control;
};

struct scsi_read_capacity_data {
  uint32_t returned_logical_block_address;
  uint32_t block_length_in_bytes;
};

// Command READ (10)

struct scsi_read_10 {
  uint8_t  __reserved0;
  uint32_t logical_block_address;
  uint8_t  __reserved1;
  uint16_t transfer_length;
  uint8_t  control;
};

// Command WRITE (10)

struct scsi_write_10 {
  uint8_t  __reserved0:3;
  uint8_t  fua:1;
  uint8_t  __reserved1:4;
  uint32_t logical_block_address;
  uint8_t  __reserved2;
  uint16_t transfer_length;
  uint8_t  control;
};

// Command descriptor

enum scsi_operation_code {
  SCSI_OPERATION_TEST_UNIT_READY  = 0x00,
  SCSI_OPERATION_REQUEST_SENSE    = 0x03,
  SCSI_OPERATION_INQUIRY          = 0x12,
  SCSI_OPERATION_MODE_SENSE_6     = 0x1A,
  SCSI_OPERATION_PREVENT_ALLOW_MEDIUM_REMOVAL
                                  = 0x1E,
  SCSI_OPERATION_READ_CAPACITY    = 0x25,
  SCSI_OPERATION_READ_10          = 0x28,
  SCSI_OPERATION_WRITE_10         = 0x2A,
};

struct scsi_command {
  uint8_t op_code;
  union {
    struct scsi_test_unit_ready   test_unit_ready;
    struct scsi_request_sense     request_sense;
    struct scsi_inquiry           inquiry;
    struct scsi_read_capacity     read_capacity;
    struct scsi_read_10           read_10;
    struct scsi_write_10          write_10;
    struct scsi_prevent_allow_medium_removal
                                  prevent_allow_medium_removal;
  };
};

// Sense data

enum scsi_sense_response_code {
  SCSI_SENSE_CURRENT_ERROR_FIXED_FORMAT       = 0x70,
  SCSI_SENSE_DEFERRED_ERROR_FIXED_FORMAT      = 0x71,
  SCSI_SENSE_CURRENT_ERROR_DESCRIPTOR_FORMAT  = 0x72,
  SCSI_SENSE_DEFERRED_ERROR_DESCRIPTOR_FORMAT = 0x73,
};

enum scsi_sense_key {
  SCSI_SENSE_NO_SENSE         = 0x0,
  SCSI_SENSE_RECOVERED_ERROR  = 0x1,
  SCSI_SENSE_NOT_READY        = 0x2,
  SCSI_SENSE_MEDIUM_ERROR     = 0x3,
  SCSI_SENSE_HARDWARE_ERROR   = 0x4,
  SCSI_SENSE_ILLEGAL_REQUEST  = 0x5,
  SCSI_SENSE_UNIT_ATTENTION   = 0x6,
  SCSI_SENSE_DATA_PROTECT     = 0x7,
  SCSI_SENSE_BLANK_CHECK      = 0x8,
  SCSI_SENSE_VENDOR_SPECIFIC  = 0x9,
  SCSI_SENSE_COPY_ABORTED     = 0xA,
  SCSI_SENSE_ABORTED_COMMAND  = 0xB,
};

struct scsi_sense_data_descriptor {
  uint8_t  response_code:7;
  uint8_t  __reserved0:1;
  uint8_t  sense_key:4;
  uint8_t  __reserved1:4;
  uint8_t  additional_sense_code;
  uint8_t  additional_sense_code_qualifier;
  uint8_t  __reserved2[3];
  uint8_t  additional_sense_length;
};

struct scsi_sense_data_fixed {
  uint8_t  response_code:7;
  uint8_t  valid:1;
  uint8_t  __obsolete0;
  uint8_t  sense_key:4;
  uint8_t  __reserved1:1;
  uint8_t  ili:1;
  uint8_t  eom:1;
  uint8_t  filemark:1;
  uint32_t information;
  uint8_t  additional_sense_length;
  uint32_t command_specific_information;
  uint8_t  additional_sense_code;
  uint8_t  additional_sense_code_qualifier;
  uint8_t  field_replaceable_unit_code;
  uint8_t  sense_key_specific:7;
  uint8_t  sksv:1;
  uint8_t  sense_key_specific2[2];
};

#endif
