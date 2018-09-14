#include <ux00boot/ux00boot.c>

int load_spiflash_gpt_partition(spi_ctrl* spictrl, void* dst, const gpt_guid* partition_type_guid) { return _load_spiflash_gpt_partition(spictrl, dst, partition_type_guid); }
int load_mmap_gpt_partition(const void* gpt_base, void* payload_dest, const gpt_guid* partition_type_guid) { return _load_mmap_gpt_partition(gpt_base, payload_dest, partition_type_guid); }
int load_sd_gpt_partition(spi_ctrl* spictrl, void* dst, const gpt_guid* partition_type_guid) { return _load_sd_gpt_partition(spictrl, dst, partition_type_guid); }
void ux00boot_fail(long code, int trap) { _ux00boot_fail(code, trap); }
void ux00boot_load_gpt_partition(void* dst, const gpt_guid* partition_type_guid, unsigned int peripheral_input_khz) { return _ux00boot_load_gpt_partition(dst, partition_type_guid, peripheral_input_khz); }
