#ifndef FAT16_H
#define FAT16_H

#include <iostream>
#include <fstream>
#include <string>
#include <stack>

#include "FAT16structs.h"

using namespace std;

class FAT16 {
public:
    FAT16(const string &imagePath);
    ~FAT16();
    void ls();
    void cat(const string &fileName);
    void mkdir(const string &dirName);
    void createFile(const string &fileName);
    void cd(const string &dirName);

private:
    FILE* imageFile;
    PartitionTable pt[4];
    Fat16BootSector bs;
    Fat16Entry entry;
    int currentPartition;
    long rootDirOffset;
    uint16_t currentDirCluster;
    string currentPath;
    stack<uint16_t> dirHistory;

    void print_file_info(Fat16Entry *entry);
    void fat16_read_file(FILE * in, FILE * out, uint32_t fat_start, uint32_t data_start, uint32_t cluster_size, uint16_t cluster, uint32_t file_size);
};

#endif // FAT16_H
