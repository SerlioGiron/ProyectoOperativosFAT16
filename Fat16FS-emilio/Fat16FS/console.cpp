#include <iostream>
#include <sstream>
#include "FAT16.h"

using namespace std;

int main() {
    string imagePath = "test.img"; 
    FAT16 fat16(imagePath);

    string input;
    string command;
    string argument;

    while (true) {
        cout << "FAT16> ";
        getline(cin, input);
        istringstream iss(input);
        iss >> command;

        if (command == "exit") {
            break;
        } else if (command == "ls") {
            cout << endl;
            fat16.ls();
            cout << endl;
        } else if (command == "cat") {
            cout << endl;
            if (!(iss >> argument)) {
                cout << "Error: Nombre de archivo no proporcionado." << endl;
            } else {
                fat16.cat(argument);
            }
            cout << endl;
        } else if (command == "mkdir") {
            iss >> argument;
            fat16.mkdir(argument);
        } else if (command == "create") { // cat > archivo.txt
            iss >> argument;
            fat16.createFile(argument);
        } else if (command == "cd") {
            if (!(iss >> argument)) {
                cout << "Error: Nombre de archivo no proporcionado." << endl;
            } else {
                fat16.cd(argument);
            }
        } else {
            cout << endl << "Comando desconocido." << endl << endl;
        }
    }

    return 0;
}
