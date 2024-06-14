import os
import struct
from datetime import datetime

SECTOR_SIZE = 512
ROOT_DIR_OFFSET = 19 * SECTOR_SIZE  # Offset for the root directory in FAT16
ROOT_DIR_SIZE = 32 * 512  # Size of the root directory in FAT16 (32 sectors)
ENTRY_SIZE = 32  # Size of each directory entry in FAT16

def read_sector(image_data, sector_number):
    offset = sector_number * SECTOR_SIZE
    return image_data[offset:offset + SECTOR_SIZE]

def read_entry(entry_data):
    filename = entry_data[:8].decode('ascii').strip()
    ext = entry_data[8:11].decode('ascii').strip()
    attr = entry_data[11]
    cluster_low = struct.unpack('<H', entry_data[26:28])[0]
    filesize = struct.unpack('<I', entry_data[28:32])[0]
    return filename, ext, attr, cluster_low, filesize

def ls_l(image_data):
    root_dir_data = image_data[ROOT_DIR_OFFSET:ROOT_DIR_OFFSET + ROOT_DIR_SIZE]
    for i in range(0, len(root_dir_data), ENTRY_SIZE):
        entry_data = root_dir_data[i:i + ENTRY_SIZE]
        if entry_data[0] == 0x00:  # No more entries
            break
        if entry_data[0] == 0xE5:  # Deleted file
            continue
        filename, ext, attr, cluster_low, filesize = read_entry(entry_data)
        if filename:
            full_name = f"{filename}.{ext}" if ext else filename
            print(f"{full_name} - Attr: {attr} - Cluster: {cluster_low} - Size: {filesize}")
            
def get_file_data(image_data, cluster, filesize):
    data = []
    cluster_size = 512 * 1  # FAT16 usually

# Función para leer la imagen FAT16
def read_image(image_path):
    with open(image_path, 'rb') as f:
        return f.read()

# Read an entry in the directory
def read_entry(entry_data):
    filename = entry_data[0:8].decode('ascii').strip()
    ext = entry_data[8:11].decode('ascii').strip()
    attr = entry_data[11]
    cluster_low = struct.unpack('<H', entry_data[26:28])[0]
    filesize = struct.unpack('<I', entry_data[28:32])[0]
    return filename, ext, attr, cluster_low, filesize

# Convert attributes byte to a string representation
def parse_attributes(attr):
    attributes = ""
    if attr & 0x01:
        attributes += "R"  # Read-only
    if attr & 0x02:
        attributes += "H"  # Hidden
    if attr & 0x04:
        attributes += "S"  # System
    if attr & 0x08:
        attributes += "V"  # Volume label
    if attr & 0x10:
        attributes += "D"  # Directory
    if attr & 0x20:
        attributes += "A"  # Archive
    return attributes

# Convert the date and time fields to a human-readable format
def parse_date_time(entry_data):
    raw_date = struct.unpack('<H', entry_data[24:26])[0]
    year = ((raw_date & 0xFE00) >> 9) + 1980
    month = (raw_date & 0x1E0) >> 5
    day = raw_date & 0x1F

    raw_time = struct.unpack('<H', entry_data[22:24])[0]
    hour = (raw_time & 0xF800) >> 11
    minute = (raw_time & 0x7E0) >> 5
    second = (raw_time & 0x1F) * 2

    return f"{year}-{month:02}-{day:02} {hour:02}:{minute:02}:{second:02}"

def ls_l(image_data):
    root_dir_data = image_data[ROOT_DIR_OFFSET:ROOT_DIR_OFFSET + ROOT_DIR_SIZE]
    for i in range(0, len(root_dir_data), ENTRY_SIZE):
        entry_data = root_dir_data[i:i + ENTRY_SIZE]
        if entry_data[0] == 0x00:  # No more entries
            break
        if entry_data[0] == 0xE5:  # Deleted file
            continue
        filename, ext, attr, cluster_low, filesize = read_entry(entry_data)
        attributes = parse_attributes(attr)
        date_time = parse_date_time(entry_data)
        full_name = filename if not ext else f"{filename}.{ext}"
        print(f"{full_name:12} {attributes:6} {filesize:10} {date_time}")

def cat_file(image_data, filename):
    root_dir_data = image_data[ROOT_DIR_OFFSET:ROOT_DIR_OFFSET + ROOT_DIR_SIZE]
    for i in range(0, len(root_dir_data), ENTRY_SIZE):
        entry_data = root_dir_data[i:i + ENTRY_SIZE]
        if entry_data[0] == 0x00:  # No more entries
            break
        if entry_data[0] == 0xE5:  # Deleted file
            continue
        entry_filename, ext, attr, cluster_low, filesize = read_entry(entry_data)
        full_name = entry_filename if not ext else f"{entry_filename}.{ext}"
        if full_name == filename and not (attr & 0x10):  # Check if it's a file (not a directory)
            print_file_contents(image_data, cluster_low, filesize)
            return
    print("File not found")

def print_file_contents(image_data, start_cluster, filesize):
    cluster_size = 512  # Assume 1 sector per cluster for simplicity
    clusters = get_file_clusters(image_data, start_cluster)
    remaining_size = filesize
    for cluster in clusters:
        cluster_offset = (31 + cluster) * cluster_size
        data_to_read = min(remaining_size, cluster_size)
        file_data = image_data[cluster_offset:cluster_offset + data_to_read]
        print(file_data.decode('ascii'), end='')
        remaining_size -= data_to_read
        if remaining_size <= 0:
            break

def get_file_clusters(image_data, start_cluster):
    clusters = []
    current_cluster = start_cluster
    while current_cluster < 0xFFF8:
        clusters.append(current_cluster)
        current_cluster = get_next_cluster(image_data, current_cluster)
    return clusters

def get_next_cluster(image_data, cluster):
    fat_offset = SECTOR_SIZE
    fat_data = image_data[fat_offset:fat_offset + 9 * SECTOR_SIZE]
    next_cluster = struct.unpack('<H', fat_data[cluster * 2:cluster * 2 + 2])[0]
    return next_cluster


def mkdir_dir(image_data, dirname):
    root_dir_data = bytearray(image_data[ROOT_DIR_OFFSET:ROOT_DIR_OFFSET + ROOT_DIR_SIZE])
    empty_entry_found = False
    for i in range(0, len(root_dir_data), ENTRY_SIZE):
        entry_data = root_dir_data[i:i + ENTRY_SIZE]
        if entry_data[0] == 0x00 or entry_data[0] == 0xE5:  # Empty or deleted entry
            empty_entry_found = True
            entry_data = bytearray(ENTRY_SIZE)
            entry_data[:8] = dirname.ljust(8).encode('ascii')
            entry_data[11] = 0x10  # Directory attribute
            
            # Find a free cluster for the new directory
            cluster_low = find_free_cluster(image_data)
            if cluster_low is None:
                print("No free clusters available")
                return
            
            # Mark the cluster as used in the FAT table
            mark_cluster_used(image_data, cluster_low)

            entry_data[26:28] = struct.pack('<H', cluster_low)
            entry_data[28:32] = struct.pack('<I', 0)  # Directory size is 0

            # Write the entry back
            root_dir_data[i:i + ENTRY_SIZE] = entry_data
            # Update the image data
            image_data[ROOT_DIR_OFFSET:ROOT_DIR_OFFSET + ROOT_DIR_SIZE] = root_dir_data
            
            # Initialize the new directory with '.' and '..' entries
            initialize_directory(image_data, cluster_low)
            print(f"Directory '{dirname}' created")
            return
    if not empty_entry_found:
        print("No space to create directory")

def find_free_cluster(image_data):
    fat_offset = SECTOR_SIZE
    fat_data = image_data[fat_offset:fat_offset + 9 * SECTOR_SIZE]
    for i in range(2, len(fat_data) // 2):
        if struct.unpack('<H', fat_data[i * 2:i * 2 + 2])[0] == 0x0000:
            return i
    raise RuntimeError("No free clusters available.")

def mark_cluster_used(image_data, cluster):
    fat_offset = SECTOR_SIZE
    fat_data = bytearray(image_data[fat_offset:fat_offset + 9 * SECTOR_SIZE])
    fat_data[cluster * 2:cluster * 2 + 2] = struct.pack('<H', 0xFFFF)
    image_data[fat_offset:fat_offset + 9 * SECTOR_SIZE] = fat_data

def initialize_directory(image_data, cluster):
    cluster_size = 512 * 1  # FAT16 usually has 1 sector per cluster
    cluster_offset = (31 + cluster) * cluster_size
    directory_data = bytearray(512)
    
    # '.' entry
    directory_data[0:11] = b'.          '
    directory_data[11] = 0x10  # Directory attribute
    directory_data[26:28] = struct.pack('<H', cluster)
    directory_data[28:32] = struct.pack('<I', 0)
    
    # '..' entry
    directory_data[32:43] = b'..         '
    directory_data[43] = 0x10  # Directory attribute
    directory_data[58:60] = struct.pack('<H', 0)  # Parent directory (root)
    directory_data[60:64] = struct.pack('<I', 0)
    
    image_data[cluster_offset:cluster_offset + 512] = directory_data

def create_file(image_data, filename):
    root_dir_data = image_data[ROOT_DIR_OFFSET:ROOT_DIR_OFFSET + ROOT_DIR_SIZE]

    # Find a free directory entry
    for i in range(0, len(root_dir_data), ENTRY_SIZE):
        entry_data = root_dir_data[i:i + ENTRY_SIZE]
        if entry_data[0] == 0x00 or entry_data[0] == 0xE5:  # Free or deleted entry
            # Prepare the new directory entry
            entry_filename, entry_ext = filename.split('.')
            entry_filename = entry_filename.ljust(8)[:8]
            entry_ext = entry_ext.ljust(3)[:3]
            entry_attr = 0x20  # Archive
            entry_cluster = find_free_cluster(image_data)
            entry_size = 0  # Initially no content
            entry_date, entry_time = get_current_date_time()

            new_entry = struct.pack(
                '<8s3sB10xHHI',
                entry_filename.encode('ascii'),
                entry_ext.encode('ascii'),
                entry_attr,
                entry_time,
                entry_date,
                entry_cluster,
                entry_size
            )

            # Write the new entry to the directory
            root_dir_data = (root_dir_data[:i] + new_entry + root_dir_data[i + ENTRY_SIZE:])
            image_data[ROOT_DIR_OFFSET:ROOT_DIR_OFFSET + ROOT_DIR_SIZE] = root_dir_data

            # Now write the content of the file
            print("Enter the content for the file. Type 'EOF' on a new line to finish.")
            lines = []
            while True:
                line = input()
                if line == "EOF":
                    break
                lines.append(line)
            file_content = "\n".join(lines).encode('ascii')
            write_file_content(image_data, entry_cluster, file_content)

            # Update the file size in the directory entry
            entry_size = len(file_content)
            new_entry = struct.pack(
                '<8s3sB10xHHI',
                entry_filename.encode('ascii'),
                entry_ext.encode('ascii'),
                entry_attr,
                entry_time,
                entry_date,
                entry_cluster,
                entry_size
            )
            root_dir_data = (root_dir_data[:i] + new_entry + root_dir_data[i + ENTRY_SIZE:])
            image_data[ROOT_DIR_OFFSET:ROOT_DIR_OFFSET + ROOT_DIR_SIZE] = root_dir_data

            print(f"File '{filename}' created successfully.")
            return
    print("Error: No free directory entries available.")
    
def get_current_date_time():
    now = datetime.now()
    year = now.year - 1980
    month = now.month
    day = now.day
    hour = now.hour
    minute = now.minute
    second = now.second // 2  # FAT stores seconds divided by 2

    date = (year << 9) | (month << 5) | day
    time = (hour << 11) | (minute << 5) | second
    return date, time


def write_file_content(image_data, start_cluster, content):
    cluster_size = 512  # Assume 1 sector per cluster for simplicity
    clusters_needed = (len(content) + cluster_size - 1) // cluster_size
    clusters = [start_cluster]
    current_cluster = start_cluster
    
    fat_offset = SECTOR_SIZE
    fat_data = image_data[fat_offset:fat_offset + 9 * SECTOR_SIZE]
    
    for _ in range(1, clusters_needed):
        next_cluster = find_free_cluster(image_data)
        clusters.append(next_cluster)
        fat_data[current_cluster * 2:current_cluster * 2 + 2] = struct.pack('<H', next_cluster)
        current_cluster = next_cluster
    
    fat_data[current_cluster * 2:current_cluster * 2 + 2] = struct.pack('<H', 0xFFFF)
    image_data[fat_offset:fat_offset + 9 * SECTOR_SIZE] = fat_data

    remaining_size = len(content)
    for cluster in clusters:
        cluster_offset = (31 + cluster) * cluster_size
        data_to_write = content[:cluster_size]
        image_data[cluster_offset:cluster_offset + len(data_to_write)] = data_to_write
        content = content[cluster_size:]

def initialize_file(image_data, cluster):
    cluster_size = 512 * 1  # FAT16 usually has 1 sector per cluster
    cluster_offset = (31 + cluster) * cluster_size
    file_data = bytearray(512)
    
    image_data[cluster_offset:cluster_offset + 512] = file_data


def cd_dir(image_data, dirname):
    global ROOT_DIR_OFFSET  # This will be updated to point to the new directory
    root_dir_data = image_data[ROOT_DIR_OFFSET:ROOT_DIR_OFFSET + ROOT_DIR_SIZE]
    for i in range(0, len(root_dir_data), ENTRY_SIZE):
        entry_data = root_dir_data[i:i + ENTRY_SIZE]
        if entry_data[0] == 0x00:  # No more entries
            break
        if entry_data[0] == 0xE5:  # Deleted file
            continue
        entry_filename, ext, attr, cluster_low, filesize = read_entry(entry_data)
        if entry_filename.strip() == dirname and attr & 0x10:  # Check if it's a directory
            ROOT_DIR_OFFSET = (31 + cluster_low) * SECTOR_SIZE
            print(f"Changed directory to '{dirname}'")
            return
    print("Directory not found")


def main():
    image_data = bytearray(read_image('test.img'))  # Use bytearray for mutability
    while True:
        command = input(":> ")
        if command.startswith('ls -l'):
            ls_l(image_data)
        elif command.startswith('cat '):
            if command.startswith('cat > '):
                filename = command.split(' ')[2]
                create_file(image_data, filename)
            else:
                filename = command.split(' ')[1]
                cat_file(image_data, filename)
        elif command.startswith('mkdir '):
            dirname = command.split(' ')[1]
            mkdir_dir(image_data, dirname)
        elif command.startswith('cd '):
            dirname = command.split(' ')[1]
            cd_dir(image_data, dirname)
        elif command == 'exit':
            break

if __name__ == '__main__':
    main()
