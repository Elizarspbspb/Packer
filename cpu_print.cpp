#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>  // Для sleep
  
  // g++ -fPIE -pie -o cpu ./cpu.cpp -std=c++17 -pthread
  
  
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

    std::cout << "prevIdle: " << prevIdle << std::endl;
    
    prevTotal = total;
    prevIdle = idle;

    if (totalDelta == 0) {
        currentUsage = 0.0;
        deltaUsage = 0.0;
        return;
    }

    std::cout << "idleDelta: " << idleDelta << std::endl;
    std::cout << "totalDelta: " << totalDelta << std::endl;
    
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
