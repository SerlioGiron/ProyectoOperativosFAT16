#ifndef FAT16_H
#define FAT16_H

#include <cstdint>
#include <string>
#include <cstdio>

// Define PartitionTable struct
struct PartitionTable {
    // Añade los campos de PartitionTable aquí
    uint8_t boot_flag;
    uint8_t start_chs[3];
    uint8_t partition_type;
    uint8_t end_chs[3];
    uint32_t start_lba;
    uint32_t size_in_sectors;
};

// Define Fat16BootSector struct
struct Fat16BootSector {
    // Añade los campos de Fat16BootSector aquí
    uint8_t jump_code[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t number_of_fats;
    uint16_t root_dir_entries;
    uint16_t total_sectors;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;
    // ... otros campos ...
};

// Define Fat16Entry struct
struct Fat16Entry {
    // Añade los campos de Fat16Entry aquí
    char filename[8];
    char ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
    // ... otros campos ...
};

class FAT16 {
public:
    FAT16(const std::string &filename);
    void ls();
    void cat(const std::string &filename);

private:
    void print_file_info(struct Fat16Entry *entry);
    void fat16_read_file(FILE *in, FILE *out, uint32_t fat_start, uint32_t data_start, uint32_t cluster_size, uint16_t cluster, uint32_t file_size);

    struct PartitionTable pt[4];
    struct Fat16BootSector bs;
    // Otros miembros privados ...
};

#endif // FAT16_H
