import os

# Ruta de la imagen de disco FAT16
DISK_IMAGE = "ruta/a/tu/imagen.img"

# Tamaño de un sector en bytes
SECTOR_SIZE = 512

raiz = "D:/"

def cat(file_path):
    print("entro a cat")
    with open(file_path, 'r') as file:
        content = file.read()
    print(content)
    
def mkdir(dir_path):
    print("entro a mkdir")
    try:
        os.makedirs(dir_path)
        print(f"Directory {dir_path} created successfully.")
    except FileExistsError:
        print(f"Directory {dir_path} already exists.")
    except Exception as e:
        print(f"An error occurred: {e}")
    
def create_file(file_path):
    print("entro a create_file")
    file_path = os.path.join(raiz, file_path)
    print(file_path)
    # Crear el archivo y escribir contenido en él
    with open(file_path, 'w') as file:
        file.write("Contenido del archivo")

    # Verificar que el archivo se creó correctamente
    if os.path.exists(file_path):
        print(f"El archivo {file_path} se ha creado correctamente.")
    else:
        print(f"No se pudo crear el archivo {file_path}.")
    
def change_dir(dir):
    print("entro a change_dir")

def ls_l():
    print("entro a ls_l")

def main():
    current_dir = raiz
    while True:
        command = input(":> ")
        parts = command.split()
        if len(parts) == 0:
            continue
        if parts[0] == "ls" and parts[1] == "-l" and len(parts) == 2:
            ls_l()
        elif parts[0] == "cat" and len(parts) == 2:
            file_path = os.path.join(current_dir, parts[1])
            cat(file_path)
        elif parts[0] == "mkdir" and len(parts) == 2:
            dir_path = os.path.join(current_dir, parts[1])
            mkdir(dir_path)
        elif parts[0] == "cat" and len(parts) == 3 and parts[1] == ">":
            create_file(parts[2])
        elif parts[0] == "cd" and len(parts) == 2:
            change_dir(parts[1])
        else:
            print("Comando no reconocido")

if __name__ == "__main__":
    main()
