#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <fx2lib.h>
#include <fx2uf2.h>
#include <scsi.h>

static enum scsi_op_code current_op;
static enum scsi_sense_key sense_key;
static uint32_t block_index;
static uint32_t blocks_left;

bool uf2_scsi_command(uint8_t lun, __xdata uint8_t *buffer, uint8_t length) __reentrant {
  __xdata struct scsi_command *command = (__xdata struct scsi_command *)buffer;
  lun;

  // Here, we generally ignore the Allocation length field when validating the commands,
  // since this is already handled on the USB MSC BBB level.

         if(command->op_code == SCSI_OPERATION_TEST_UNIT_READY &&
            length >= sizeof(command->op_code) + sizeof(command->test_unit_ready)) {
    goto success;
  } else if(command->op_code == SCSI_OPERATION_REQUEST_SENSE &&
            length >= sizeof(command->op_code) + sizeof(command->request_sense)) {
    if(command->request_sense.desc == 0) {
      goto success;
    }
  } else if(command->op_code == SCSI_OPERATION_INQUIRY &&
            length >= sizeof(command->op_code) + sizeof(command->inquiry)) {
    if(command->inquiry.evpd == 0 && command->inquiry.page_code == 0) {
      goto success;
    }
  } else if(command->op_code == SCSI_OPERATION_PREVENT_ALLOW_MEDIUM_REMOVAL &&
            length >= sizeof(command->op_code) + sizeof(command->prevent_allow_medium_removal)) {
    goto success;
  } else if(command->op_code == SCSI_OPERATION_READ_CAPACITY &&
            length >= sizeof(command->op_code) + sizeof(command->read_capacity)) {
    goto success;
  } else if(command->op_code == SCSI_OPERATION_READ_10 &&
            length >= sizeof(command->op_code) + sizeof(command->read_10)) {
    block_index = bswap32(command->read_10.logical_block_address);
    blocks_left = bswap32(command->read_10.transfer_length);
    goto success;
  } else if(command->op_code == SCSI_OPERATION_WRITE_10 &&
            length >= sizeof(command->op_code) + sizeof(command->write_10)) {
    block_index = bswap32(command->write_10.logical_block_address);
    blocks_left = bswap32(command->write_10.transfer_length);
    goto success;
  }

  current_op = 0;
  sense_key  = SCSI_SENSE_ILLEGAL_REQUEST;
  return false;

success:
  current_op = command->op_code;
  return true;
}

bool uf2_scsi_data_out(uint8_t lun, __xdata uint8_t *buffer, uint16_t length) __reentrant {
  lun;

  if(current_op == SCSI_OPERATION_WRITE_10) {
    if(blocks_left == 0)
      return false;
    if(length != 512)
      return false;
    if(!uf2_fat_write(block_index, buffer))
      return false;

    block_index++;
    blocks_left--;
    if(blocks_left > 0)
      return true;

    goto done;
  }

  return false;

done:
  current_op = 0;
  return true;
}

bool uf2_scsi_data_in(uint8_t lun, __xdata uint8_t *buffer, uint16_t length) __reentrant {
  lun;

  if(current_op == SCSI_OPERATION_REQUEST_SENSE) {
    __xdata struct scsi_sense_data_fixed *data =
      (__xdata struct scsi_sense_data_fixed *)buffer;
    xmemclr(buffer, length);

    data->response_code = SCSI_SENSE_CURRENT_ERROR_FIXED_FORMAT;
    data->sense_key     = sense_key;

    sense_key = SCSI_SENSE_NO_SENSE;
    goto done;
  } else if(current_op == SCSI_OPERATION_INQUIRY) {
    __xdata struct scsi_inquiry_data *data =
      (__xdata struct scsi_inquiry_data *)buffer;
    xmemclr(buffer, length);

    data->additional_length = sizeof(struct scsi_inquiry_data) -
      (offsetof(struct scsi_inquiry_data, additional_length) +
       sizeof(data->additional_length));

    // SBC-2 (Direct access block device).
    // It would be more fitting to use RBC here, but Windows does not understand
    // how to talk to that (even though RBC is basically boneless SBC...)
    data->peripheral_device_type = 0x00;

    // We're a removable device. Without this flag, Windows will refuse to mount
    // the device as a flat filesystem, and will instead demand to have it partitioned.
    data->rmb = true;

    // Our identification.
    xmemcpy(data->t10_vendor_identification, (__xdata void *)"Qi-Hardw",          8);
    xmemcpy(data->product_identification,    (__xdata void *)"Cypress UF2 Boot",  16);
    xmemcpy(data->product_revision_level,    (__xdata void *)"A0  ",              4);

    goto done;
  } else if(current_op == SCSI_OPERATION_READ_CAPACITY) {
    __xdata struct scsi_read_capacity_data *data =
      (__xdata struct scsi_read_capacity_data *)buffer;

    data->returned_logical_block_address  = bswap32(uf2_config.total_sectors - 1);
    data->block_length_in_bytes           = bswap32(512);

    goto done;
  } else if(current_op == SCSI_OPERATION_READ_10) {
    if(blocks_left == 0)
      return false;
    if(length != 512)
      return false;
    if(!uf2_fat_read(block_index, buffer))
      return false;

    block_index++;
    blocks_left--;
    if(blocks_left > 0)
      return true;

    goto done;
  }

  return false;

done:
  current_op = 0;
  return true;
}
