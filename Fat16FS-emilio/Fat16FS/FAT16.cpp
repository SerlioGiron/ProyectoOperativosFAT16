#include "FAT16.h"

FAT16::FAT16(const string &imagePath) {
    currentPartition = -1;
    rootDirOffset = 0;
    currentDirCluster = 0;
    imageFile = fopen(imagePath.c_str(), "rb");
    if (!imageFile) {
        cerr << "Unable to open image file." << endl;
        return;
    }

    fseek(imageFile, 0x1BE, SEEK_SET);
    fread(pt, sizeof(PartitionTable), 4, imageFile);

    for(int i = 0; i < 4; i++) {        
        if(pt[i].partition_type == 4 || pt[i].partition_type == 6 || pt[i].partition_type == 14) {
            //cout << "FAT16 filesystem found from partition " << i << endl;
            currentPartition = i;
            break;
        }
    }

    if(currentPartition == -1) {
        cout << "No FAT16 filesystem found, exiting..." << endl;
        return;
    }

    fseek(imageFile, 512 * pt[currentPartition].start_sector, SEEK_SET);
    fread(&bs, sizeof(Fat16BootSector), 1, imageFile);

    //cout << "Now at 0x" << hex << ftell(imageFile) << ", sector size " << bs.sector_size << ", FAT size " << bs.fat_size_sectors << " sectors, " << bs.number_of_fats << " FATs" << endl << dec;

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
        cerr << "Error: Imagen no abierta." << endl;
        return;
    }

    uint32_t cluster_size = bs.sectors_per_cluster * bs.sector_size;
    uint32_t data_start = rootDirOffset + (bs.root_dir_entries * sizeof(Fat16Entry));

    // Calcula el offset del directorio actual basado en currentDirCluster
    uint32_t dir_offset = (currentDirCluster == 0) ? rootDirOffset : data_start + (currentDirCluster - 2) * cluster_size;

    fseek(imageFile, dir_offset, SEEK_SET);

    Fat16Entry entry;
    for(int i = 0; i < bs.root_dir_entries || currentDirCluster != 0; i++) {
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

void FAT16::print_file_info(Fat16Entry * entry) {
    switch(entry->filename[0]) {
        case 0x00:
            return;
        case 0xE5:
            printf("Deleted file: [?%.7s.%.3s]\n", entry->filename+1, entry->ext);
            return;
        case 0x05:
            printf("File starting with 0xE5: [%c%.7s.%.3s]\n", 0xE5, entry->filename+1, entry->ext);
            break;
        case 0x2E:
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

void FAT16::cat(const string &fileName) {
    if (!imageFile) {
        cerr << "Error: Imagen no abierta." << endl;
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

    int i, j;
    for (i = 0; i < 8 && i < fileName.size() && fileName[i] != '.'; i++)
        filename[i] = toupper(fileName[i]);
    if (i < fileName.size() && fileName[i] == '.') {
        for (j = 1; j <= 3 && (i + j) < fileName.size(); j++)
            file_ext[j - 1] = toupper(fileName[i + j]);
    }

    for (i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, imageFile);
        if (entry.filename[0] == 0x00) break;
        if (memcmp(entry.filename, filename, 8) == 0 && memcmp(entry.ext, file_ext, 3) == 0) {
             if (entry.attributes & 0x10) {
                cout << "Error: '" << fileName << "' es un directorio, no un archivo." << endl;
                return;
            }
            fat16_read_file(imageFile, stdout, fat_start, data_start, bs.sectors_per_cluster * bs.sector_size, entry.starting_cluster, entry.file_size);
            return;
        }
    }

    cout << "Archivo no encontrado: " << fileName << endl;
}

void FAT16::fat16_read_file(FILE *in, FILE *out, uint32_t fat_start, uint32_t data_start, uint32_t cluster_size, uint16_t cluster, uint32_t file_size) {
    unsigned char buffer[4096];
    size_t bytes_read, bytes_to_read, file_left = file_size, cluster_left = cluster_size;

    fseek(in, data_start + cluster_size * (cluster - 2), SEEK_SET);

    while(file_left > 0 && cluster != 0xFFFF) {
        bytes_to_read = sizeof(buffer);

        if(bytes_to_read > file_left)
            bytes_to_read = file_left;
        if(bytes_to_read > cluster_left)
            bytes_to_read = cluster_left;

        bytes_read = fread(buffer, 1, bytes_to_read, in);
        fwrite(buffer, 1, bytes_read, out);

        cluster_left -= bytes_read;
        file_left -= bytes_read;

        if(cluster_left == 0) {
            fseek(in, fat_start + cluster * 2, SEEK_SET);
            fread(&cluster, 2, 1, in);

            fseek(in, data_start + cluster_size * (cluster - 2), SEEK_SET);
            cluster_left = cluster_size;
        }
    }
}

void FAT16::mkdir(const string & dirName) {
    if (!imageFile) {
        cerr << "Error: Imagen no abierta." << endl;
        return;
    }

    char dirname[9];
    fill_n(dirname, 9, ' ');
    for (size_t i = 0; i < min(dirName.size(), sizeof(dirname) - 1); ++i) {
        dirname[i] = toupper(dirName[i]);
    }

    // Encontrar un espacio libre en el directorio actual para una nueva entrada
    bool freeSpace = false;
    uint32_t dirOffset;
    if (currentDirCluster == 0) {
        dirOffset = rootDirOffset;
    } else {
        uint32_t clusterSize = bs.sectors_per_cluster * bs.sector_size;
        uint32_t dataStart = (bs.reserved_sectors + bs.number_of_fats * bs.fat_size_sectors) * bs.sector_size;
        dirOffset = dataStart + (currentDirCluster - 2) * clusterSize;
    }

    fseek(imageFile, dirOffset, SEEK_SET);
    Fat16Entry entry, freeEntry;
    uint32_t freeEntryPosition = -1;

    // Leer entradas de directorio hasta encontrar una vacía
    while (fread(&entry, sizeof(Fat16Entry), 1, imageFile) == 1) {
        if (entry.filename[0] == 0x00 || entry.filename[0] == 0xE5) {
            freeEntry = entry;
            freeSpace = true;
            freeEntryPosition = ftell(imageFile) - sizeof(Fat16Entry);
            break;
        }
    }

    if(freeSpace && freeEntryPosition != -1) {
        uint16_t freeCluster = 0;

        uint32_t fatStart = bs.reserved_sectors * bs.sector_size;
        uint16_t clusterValue;
        uint32_t fatOffset;

        fseek(imageFile, fatStart, SEEK_SET);

        // Busca un cluster libre
        for (uint16_t cluster = 2; cluster < bs.total_sectors_long / bs.sectors_per_cluster; ++cluster) {
            fatOffset = cluster * 2;
            fseek(imageFile, fatStart + fatOffset, SEEK_SET);
            fread(&clusterValue, sizeof(clusterValue), 1, imageFile);

            if (clusterValue == 0x0000) {
                freeCluster = cluster;
            }
        }

        if(freeCluster == 0) {
            cerr << "No se encontro ningun cluster disponible" << endl;
            return;
        }

        // Actualiza la entrada de directorio
        memcpy(freeEntry.filename, dirname, 8);
        memset(freeEntry.ext, ' ', 3);
        freeEntry.attributes = 0x10;
        freeEntry.starting_cluster = freeCluster;
        freeEntry.file_size = 0;
        
        // Escribe la entrada acttualizada en la FAT
        fseek(imageFile, -static_cast<long>(sizeof(Fat16Entry)), SEEK_CUR);
        fwrite(&freeEntry, sizeof(Fat16Entry), 1, imageFile);

        // Actualiza la FAT
        uint32_t fatStartA = bs.reserved_sectors * bs.sector_size;
        uint16_t eofMarker = 0xFFFF;
        uint32_t fatOffsetA = freeCluster * 2;

        fseek(imageFile, freeEntryPosition, SEEK_SET);
        fwrite(&freeEntry, sizeof(Fat16Entry), 1, imageFile);
        fflush(imageFile);

        // Si hay múltiples copias de la FAT, actualízalas todas
        for (int i = 1; i < bs.number_of_fats; i++) {
            fseek(imageFile, fatStartA + (bs.fat_size_sectors * bs.sector_size * i) + fatOffsetA, SEEK_SET);
            fwrite(&eofMarker, sizeof(eofMarker), 1, imageFile);
        }

        cout << "Directorio '" << dirName << "' creado." << endl;

    } else {
        cout << "No se encontro espacio libre para el nuevo directorio." << endl;
    }
}

void FAT16::createFile(const string & fileName) {
}

void FAT16::cd(const string &dirName) {
    if (!imageFile) {
        cerr << "Error: Imagen no abierta." << endl;
        return;
    }

    if (dirName == "..") {
        if (!dirHistory.empty()) {
            currentDirCluster = dirHistory.top();
            dirHistory.pop();
            return;
        } else {
            cerr << "Ya estás en el directorio raíz." << endl;
            return;
        }
    }

    uint32_t fat_start = rootDirOffset - bs.root_dir_entries * sizeof(Fat16Entry);
    uint32_t data_start = rootDirOffset + (bs.root_dir_entries * sizeof(Fat16Entry));
    uint32_t cluster_size = bs.sectors_per_cluster * bs.sector_size;

    fseek(imageFile, rootDirOffset, SEEK_SET);
    Fat16Entry entry;
    char dirname[9] = "        ";

    string upperDirName = dirName;
    transform(upperDirName.begin(), upperDirName.end(), upperDirName.begin(), ::toupper);
    strncpy(dirname, upperDirName.c_str(), min(8, static_cast<int>(upperDirName.size())));


    for (int i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, imageFile);
        if (memcmp(entry.filename, dirname, 8) == 0 && (entry.attributes & 0x10)) {
            // Encontrado el directorio
            dirHistory.push(currentDirCluster);
            currentDirCluster = entry.starting_cluster;
            currentPath = dirName;
            return;
        }
    }

    cerr << "Directorio no encontrado: " << dirName << endl;
}
