import os

# Ruta de la imagen de disco FAT16
DISK_IMAGE = "ruta/a/tu/imagen.img"

# Tamaño de un sector en bytes
SECTOR_SIZE = 512

def cat():
    print("entro a cat")
    
def mkdir():
    print("entro a mkdir")
    
def create_file():
    print("entro a create_file")
    
def change_dir():
    print("entro a change_dir")

def ls_l():
    print("entro a ls_l")

def main():
    current_dir = "/"
    while True:
        command = input(":> ")
        parts = command.split()
        if len(parts) == 0:
            continue
        if parts[0] == "ls" and parts[1] == "-l" and len(parts) == 2:
            ls_l()
        elif parts[0] == "cat" and len(parts) == 2:
            cat(parts[1])
        elif parts[0] == "mkdir" and len(parts) == 2:
            mkdir(parts[1])
        elif parts[0] == "cat" and len(parts) == 3 and parts[1] == ">":
            create_file(parts[2])
        elif parts[0] == "cd" and len(parts) == 2:
            change_dir(parts[1])
        else:
            print("Comando no reconocido")

if __name__ == "__main__":
    main()
