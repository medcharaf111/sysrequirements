#include "dxtextmake.h"
#include <thread>
#include <chrono>

using namespace std;
int main() {
    this_thread::sleep_for(chrono::seconds(2));
    dxdiag();
    fstream file;
    string filename="dxdiag_output.txt";
    file.open(filename.c_str());
    string pro_name;
    string line;
    int linereach = 223;
    int linelim = 0;

    while (getline(file, line) && linelim < linereach) {
        linelim++;
        cout << line << "\n" ;
    };

    file.close();

   
    }

