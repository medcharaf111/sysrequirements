#pragma once 
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdexcept>



void dxdiag()
{

    const std::string filename = "dxdiag_output.txt";

    try {
        std::ifstream file(filename, std::ios::in);
        if (file.is_open()) {
            std::cout << filename << " dxdiag exists. terminating ..\n";
            file.close();
        } else {
            std::cout << filename << " not found. Creating...\n";
            int result = std::system("dxdiag /t dxdiag_output.txt");
            if (result == 0) {
                std::cout << "dxdiag_output.txt created successfully.\n";
            } else {
                throw std::runtime_error("Failed to create dxdiag output.");
            }
        }
    } catch (const std::ios_base::failure& e) {
        std::cerr << "File error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "An unknown error occurred.\n";
    };
}
