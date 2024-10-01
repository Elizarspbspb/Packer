/*#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>  // Для sleep
    
static unsigned long long prevTotal = 0, prevIdle = 0;
    
// Функция для получения загрузки процессора из /proc/stat
void getCpuUsage(double& currentUsage, double& deltaUsage) {
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) {
        std::cerr << "Failed to open /proc/stat" << std::endl;
        currentUsage = -1.0;
        deltaUsage = -1.0;
        return;
    }

    std::string line;
    std::getline(statFile, line);
    statFile.close();

    std::istringstream iss(line);
    std::string cpu;
    unsigned long long user, nice, system, idle;
    if (!(iss >> cpu >> user >> nice >> system >> idle)) {
        std::cerr << "Error reading /proc/stat" << std::endl;
        currentUsage = -1.0;
        deltaUsage = -1.0;
        return;
    }

    unsigned long long total = user + nice + system + idle;
    unsigned long long totalDelta = total - prevTotal;
    unsigned long long idleDelta = idle - prevIdle;

    prevTotal = total;
    prevIdle = idle;

    if (totalDelta == 0) {
        currentUsage = 0.0;
        deltaUsage = 0.0;
        return;
    }

    currentUsage = 100.0 * (1.0 - static_cast<double>(idleDelta) / totalDelta);

    static double prevUsage = 0.0;
    deltaUsage = currentUsage - prevUsage;
    prevUsage = currentUsage;
}

int main() {
    while (true) {
        double currentUsage, deltaUsage;
        getCpuUsage(currentUsage, deltaUsage);

        if (currentUsage >= 0) {
            std::cout << "Current CPU Usage: " << currentUsage << "% \t || ";
            std::cout << "CPU Usage Change: " << deltaUsage << "%" << std::endl;
        } else {
            std::cerr << "Error retrieving CPU usage" << std::endl;
        }

        sleep(1);  // Пауза на 1 секунду
    }
    return 0;
}
*/



#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>  // Для sleep
#include <cmath>     // Для sqrt

// g++ -fPIE -pie -o cpu ./cpu.cpp -std=c++17 -pthread
// sudo ./pid 11601  0x70177afa6de0  0x7f0ffffffffc7d81   (0x7f7ffffffffc7d81 - загруженно максимально)


static unsigned long long Total = 10000, Idle = 2000;
//static unsigned long long prevTotal = 10000, prevIdle = 2000;
static long long prevTotal = 10000, prevIdle = 2000;
static double prevUsage = 10.0;

// Функция для получения загрузки процессора из /proc/stat
void getCpuUsage(double& currentUsage, double& deltaUsage) {
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) {
        std::cerr << "Failed to open /proc/stat" << std::endl;
        currentUsage = -1.0;
        deltaUsage = -1.0;
        return;
    }

    std::string line;
    std::getline(statFile, line);
    statFile.close();

    std::istringstream iss(line);
    std::string cpu;
    unsigned long long user, nice, system, idle;
    if (!(iss >> cpu >> user >> nice >> system >> idle)) {
        std::cerr << "Error reading /proc/stat" << std::endl;
        currentUsage = -1.0;
        deltaUsage = -1.0;
        return;
    }

    unsigned long long total = user + nice + system + idle;
    unsigned long long totalDelta = total - Total;
    unsigned long long idleDelta = idle - Idle;

    Total = total;
    Idle = idle;

    if (totalDelta == 0) {
        currentUsage = 0.0;
        deltaUsage = 0.0;
        return;
    }

    currentUsage = 100.0 * (1.0 - static_cast<double>(idleDelta) / totalDelta);

    deltaUsage = currentUsage - prevUsage;
    prevUsage = currentUsage;
}

int main() {
    static unsigned long long cycleCount = 0; // Статическая переменная для подсчета циклов

    while (true) {
        double currentUsage, deltaUsage;
        getCpuUsage(currentUsage, deltaUsage);

        if (currentUsage >= 0) {
            std::cout << "Current CPU Usage: " << currentUsage << "% \t || ";
            std::cout << "CPU Usage Change: " << deltaUsage << "%" << std::endl;
        } else {
            std::cerr << "Error retrieving CPU usage" << std::endl;
        }

        // Используем статическую переменную для демонстрации логики
        cycleCount++;
        if (cycleCount % 10 == 0) {
            std::cout << "Cycle count reached " << cycleCount << ", simulating CPU load..." << std::endl;
            for (int i = 0; i < 100000000; ++i) {
                double dummy = sqrt(i); // Симуляция нагрузки на процессор
            }
            std::cout << "Simulation complete." << std::endl;
        }

        sleep(1);  // Пауза на 1 секунду
        std::cout << " ------------------- ... " << prevTotal - prevIdle << " sec." << std::endl;
    }
    return 0;
}
