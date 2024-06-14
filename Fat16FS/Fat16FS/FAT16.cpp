#include "FAT16.h"
#include <cstring>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <ctime>
#include <cstdio>

FAT16::FAT16(const std::string &imagePath) {
    currentPartition = -1;
    rootDirOffset = 0;
    currentDirCluster = 0;
    imageFile = fopen(imagePath.c_str(), "r+b");
    if (!imageFile) {
        std::cerr << "Error al abrir la imagen." << std::endl;
        return;
    }

    // Leer el boot sector
    fseek(imageFile, 0x1BE, SEEK_SET);
    fread(pt, sizeof(PartitionTable), 4, imageFile);

    for (int i = 0; i < 4; i++) {
        if (pt[i].partition_type == 4 || pt[i].partition_type == 6 || pt[i].partition_type == 14) {
            currentPartition = i;
            break;
        }
    }

    if (currentPartition == -1) {
        std::cout << "No se encontró un FAT16 filesystem." << std::endl;
        return;
    }

    fseek(imageFile, 512 * pt[currentPartition].start_sector, SEEK_SET);
    fread(&bs, sizeof(Fat16BootSector), 1, imageFile);

    // Calcula el offset al directorio raíz
    rootDirOffset = ftell(imageFile) + (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size;
}

FAT16::~FAT16() {
    if (imageFile) {
        fclose(imageFile);
    }
}

void FAT16::ls() {
    if (!imageFile) {
        std::cerr << "Error: Imagen no abierta." << std::endl;
        return;
    }

    uint32_t cluster_size = bs.sectors_per_cluster * bs.sector_size;
    uint32_t data_start = rootDirOffset + (bs.root_dir_entries * sizeof(Fat16Entry));

    // Calcula el offset del directorio actual basado en currentDirCluster
    uint32_t dir_offset = (currentDirCluster == 0) ? rootDirOffset : data_start + (currentDirCluster - 2) * cluster_size;

    fseek(imageFile, dir_offset, SEEK_SET);

    Fat16Entry entry;
    for (int i = 0; i < bs.root_dir_entries || currentDirCluster != 0; i++) {
        if (fread(&entry, sizeof(entry), 1, imageFile) < 1) {
            break;
        }
        if (entry.filename[0] == 0x00 || entry.filename[0] == 0xE5) {
            continue;
        }
        if (memcmp(entry.filename, ".       ", 8) == 0 || memcmp(entry.filename, "..      ", 8) == 0) {
            continue;
        }
        print_file_info(&entry);
    }
}

void FAT16::print_file_info(Fat16Entry *entry) {
    switch (entry->attributes) {
        case 0x00:
            return;
        case 0xE5:
            printf("Deleted file: [?%.7s.%.3s]\n", entry->filename + 1, entry->ext);
            return;
        case 0x05:
            printf("File starting with 0xE5: [%c%.7s.%.3s]\n", 0xE5, entry->filename + 1, entry->ext);
            break;
        case 0x10:
            printf("Directory: [%.8s.%.3s]\n", entry->filename, entry->ext);
            break;
        default:
            printf("File: [%.8s.%.3s]\n", entry->filename, entry->ext);
    }

    printf("  Modified: %04d-%02d-%02d %02d:%02d.%02d    Start: [%04X]    Size: %d\n",
           1980 + (entry->modify_date >> 9), (entry->modify_date >> 5) & 0xF, entry->modify_date & 0x1F,
           (entry->modify_time >> 11), (entry->modify_time >> 5) & 0x3F, entry->modify_time & 0x1F,
           entry->starting_cluster, entry->file_size);
}

void FAT16::cat(const std::string &fileName) {
    if (!imageFile) {
        std::cerr << "Error: Imagen no abierta." << std::endl;
        return;
    }

    uint32_t fat_start = rootDirOffset - bs.root_dir_entries * sizeof(Fat16Entry);
    uint32_t data_start = rootDirOffset + (bs.root_dir_entries * sizeof(Fat16Entry));
    uint32_t cluster_size = bs.sectors_per_cluster * bs.sector_size;

    // Calcula el offset del directorio actual
    uint32_t dir_offset = (currentDirCluster == 0) ? rootDirOffset : data_start + (currentDirCluster - 2) * cluster_size;

    fseek(imageFile, dir_offset, SEEK_SET);
    Fat16Entry entry;
    char filename[9] = "        ", file_ext[4] = "   ";

    // Pasa el fileName a mayus
    int i, j;
    for (i = 0; i < 8 && i < fileName.size() && fileName[i] != '.'; i++)
        filename[i] = toupper(fileName[i]);
    if (i < fileName.size() && fileName[i] == '.') {
        for (j = 1; j <= 3 && (i + j) < fileName.size(); j++)
            file_ext[j - 1] = toupper(fileName[i + j]);
    }

    // Busca el archivo para imprimir sus datos
    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, imageFile);
        if (entry.filename[0] == 0x00) break;
        if (memcmp(entry.filename, filename, 8) == 0 && memcmp(entry.ext, file_ext, 3) == 0) {
            if (entry.attributes & 0x10) {
                std::cout << "Error: '" << fileName << "' es un directorio, no un archivo." << std::endl;
                return;
            }
            fat16_read_file(imageFile, stdout, fat_start, data_start, bs.sectors_per_cluster * bs.sector_size, entry.starting_cluster, entry.file_size);
            return;
        }
    }

    std::cout << "Archivo no encontrado: " << fileName << std::endl;
}

void FAT16::fat16_read_file(FILE *in, FILE *out, uint32_t fat_start, uint32_t data_start, uint32_t cluster_size, uint16_t cluster, uint32_t file_size) {
    unsigned char buffer[4096];
    size_t bytes_read, bytes_to_read, file_left = file_size, cluster_left = cluster_size;

    fseek(in, data_start + cluster_size * (cluster - 2), SEEK_SET);

    // Lee todo el archivo o hasta alcanzar el final del cluster
    while (file_left > 0 && cluster != 0xFFFF) {
        bytes_to_read = sizeof(buffer);

        // Ajusta los tamaños
        if (bytes_to_read > file_left)
            bytes_to_read = file_left;
        if (bytes_to_read > cluster_left)
            bytes_to_read = cluster_left;

        bytes_read = fread(buffer, 1, bytes_to_read, in);
        fwrite(buffer, 1, bytes_read, out);

        cluster_left -= bytes_read;
        file_left -= bytes_read;

        // Si está al final del cluster, se mueve al siguiente
        if (cluster_left == 0) {
            fseek(in, fat_start + cluster * 2,
