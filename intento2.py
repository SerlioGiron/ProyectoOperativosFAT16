import os
import time

# Ruta de la imagen de disco FAT16
DISK_IMAGE = "ruta/a/tu/imagen.img"

# Tamaño de un sector en bytes
SECTOR_SIZE = 512

raiz = "D:/"

def cat(file_path):
    # print("entro a cat")
    with open(file_path, 'r') as file:
        content = file.read()
    print(content)
    
def mkdir(dir_path):
    # print("entro a mkdir")
    try:
        os.makedirs(dir_path)
        print(f"Directory {dir_path} created successfully.")
    except FileExistsError:
        print(f"Directory {dir_path} already exists.")
    except Exception as e:
        print(f"An error occurred: {e}")
    
def create_file(file_path):
    # print("entro a create_file")
    # print(file_path)
    contenido = input()
    # Crear el archivo y escribir contenido en él
    with open(file_path, 'w') as file:
        file.write(contenido)
    # Verificar que el archivo se creó correctamente
    if os.path.exists(file_path):
        print(f"El archivo {file_path} se ha creado correctamente.")
    else:
        print(f"No se pudo crear el archivo {file_path}.")
    
def change_dir(dir_path):
    try:
        # Change the current working directory to dir_path
        os.chdir(dir_path)
        print(f"Changed directory to {dir_path}")
    except FileNotFoundError:
        # This exception is raised if the directory does not exist
        print(f"Directory {dir_path} does not exist.")
    except NotADirectoryError:
        # This exception is raised if dir_path is not a directory
        print(f"{dir_path} is not a directory.")
    except PermissionError:
        # This exception is raised if you do not have permission to change to the directory
        print(f"Permission denied to change to directory {dir_path}.")
    except Exception as e:
        # Catch any other exceptions and print the error message
        print(f"An error occurred: {e}")
    
    return dir_path

def ls_l(dir_path):
    # print("entro a ls_l")
    # print(f"Directory: {dir_path}")
    # Check if the provided path is a directory
    if not os.path.isdir(dir_path):
        print(f"The path {dir_path} is not a directory.")
        return

    # Get the list of all files and directories in the specified directory
    for entry in os.listdir(dir_path):
        full_path = os.path.join(dir_path, entry)  # Get the full path of the entry
        if os.path.isfile(full_path) or os.path.isdir(full_path):
            # Get metadata
            info = os.stat(full_path)
            size = info.st_size  # Size in bytes
            last_modified = time.ctime(info.st_mtime)  # Last modified time
            mode = info.st_mode  # Mode (permissions)
            entry_type = "Directory" if os.path.isdir(full_path) else "File"
            print(f"{entry}\tType: {entry_type}\tSize: {size} bytes\tLast Modified: {last_modified}\tMode: {mode}")

def main():
    current_dir = raiz
    while True:
        command = input(":> ")
        parts = command.split()
        if len(parts) == 0:
            continue
        if parts[0] == "ls" and parts[1] == "-l" and len(parts) == 2:
            ls_l(current_dir)
        elif parts[0] == "cat" and len(parts) == 2:
            file_path = os.path.join(current_dir, parts[1])
            cat(file_path)
        elif parts[0] == "mkdir" and len(parts) == 2:
            dir_path = os.path.join(current_dir, parts[1])
            mkdir(dir_path)
        elif parts[0] == "cat" and len(parts) == 3 and parts[1] == ">":
            file_path = os.path.join(current_dir, parts[2])
            create_file(file_path)
        elif parts[0] == "cd" and len(parts) == 2:
            new_dir = os.path.join(current_dir, parts[1])
            current_dir = change_dir(new_dir)
        else:
            print("Comando no reconocido")

if __name__ == "__main__":
    main()
