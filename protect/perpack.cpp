#include <iostream>
#include <fstream>
#include <vector>
#include <libelf.h>
#include <gelf.h>
#include <unistd.h>
#include <sys/wait.h>   // Для работы с процессами и их статусом
#include <sys/stat.h>   // Для работы с файловыми режимами
#include <fcntl.h>      // Для работы с файловыми дескрипторами
#include <map>
#include <cstring>
#include <algorithm>    // Для работы с алгоритмами - std::search

#include <iomanip>      // Для вывода в hex формате

#include "elf_tables.h"

// g++ -o perpack ./perpack.cpp ./elf_tables.h ./elf_tables.cpp -lelf
//  ./perpack cpu_test 0x5020 0x5028 8
// objdump -D ./output_cpu_test | grep prev
// objdump -D ./cpu_test | grep prev
//
// .. Тут надо бы структуру с полями символов
//


// Инициализация библиотеки libelf
void initialize_libelf() {
    // Проверяет, была ли библиотека libelf инициализирована
    if (elf_version(EV_CURRENT) == EV_NONE) {
        std::cerr << "ELF library initialization failed: " << elf_errmsg(-1) << std::endl;
        exit(EXIT_FAILURE);     // Завершает выполнение программы с кодом ошибки
    }
}

// Чтение ELF-файла
void read_elf_file(const std::string &file_path, std::vector<char> &buffer) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    file.seekg(0, std::ios::end);               // Перемещает указатель чтения в конец файла
    buffer.resize(file.tellg());                // Изменяет размер буфера в зависимости от размера файла
    file.seekg(0, std::ios::beg);               // Перемещает указатель чтения в начало файла
    file.read(buffer.data(), buffer.size());    // Читает данные из файла в буфер
}

// Записывает содержимое буфера в ELF-файл
void write_elf_file(const std::string &file_path, const std::vector<char> &buffer) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    // Записывает данные из буфера в файл
    file.write(buffer.data(), buffer.size());
}

// Ищет адрес в буфере и возвращает его смещение
/*bool find_symbol_by_address(const std::vector<char> &buffer, size_t address, size_t &offset, size_t size) {
    auto it = std::search(buffer.begin(), buffer.end(), reinterpret_cast<const char*>(&address), reinterpret_cast<const char*>(&address) + sizeof(address));    // Ищет адрес в буфере
    if (it != buffer.end()) {
        offset = std::distance(buffer.begin(), it);     // Вычисляет смещение найденного адреса
        std::cout << "offset! : " << offset << std::endl;
        //offset = address;
        return true;
    }
    return false;
}*/

// Меняет местами байты двух последовательностей в буфере
/*void swap_byte_sequences(std::vector<char> &buffer, size_t address1, size_t address2, size_t size) {
    size_t offset1, offset2;
    
    // Находит смещения для двух адресов
    if (find_symbol_by_address(buffer, address1, offset1, size) && find_symbol_by_address(buffer, address2, offset2, size)) {
        std::cout << "addr1 = " << address1 << "; addr2 = " << address2 << "; offset1 = " << offset1 << "; offset2 = " << offset2 << std::endl;
        // Сохраняет байты первой последовательности во временный буфер
        std::vector<char> temp(buffer.begin() + offset1, buffer.begin() + offset1 + size);
        // Копирует байты из второй последовательности на место первой
        std::copy(buffer.begin() + offset2, buffer.begin() + offset2 + size, buffer.begin() + offset1);
        // Копирует байты из временного буфера на место второй последовательности
        std::copy(temp.begin(), temp.end(), buffer.begin() + offset2);
    } else {
        std::cerr << "One or both addresses not found in the buffer." << std::endl;
    }
}*/

// Обновляет таблицу символов ELF-файла
/*void update_symbol_table(Elf *e, size_t address1, size_t address2) {
    Elf_Scn *scn = nullptr;     // Объявляет переменную для секций
    GElf_Shdr shdr;             // Объявляет переменную для заголовка секции

    // Итерация по секциям ELF-файла
    while ((scn = elf_nextscn(e, scn)) != nullptr) {
        // Получает заголовок секции. Проверяет наличие ошибок
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            std::cerr << "gelf_getshdr() failed: " << elf_errmsg(-1) << std::endl;
            return;
        }

        // Проверяет, является ли секция таблицей символов
        if (shdr.sh_type == SHT_SYMTAB) {
            // Получает данные из секции
            Elf_Data *data = elf_getdata(scn, nullptr);
            if (!data) {
                std::cerr << "elf_getdata() failed: " << elf_errmsg(-1) << std::endl;
                return;
            }

            // Вычисляет количество символов
            size_t symbol_count = shdr.sh_size / shdr.sh_entsize;
            // Итерация по символам в таблице
            for (size_t i = 0; i < symbol_count; ++i) {
                GElf_Sym sym;   // Объявляет переменную для символа
                if (gelf_getsym(data, i, &sym) != &sym) {   // Получает символ из таблицы
                    std::cerr << "gelf_getsym() failed: " << elf_errmsg(-1) << std::endl;
                    return;
                }

                // Проверяет, совпадает ли адрес символа с первым адресом
                if (sym.st_value == address1) {
                    sym.st_value = address2;            // Меняет адрес символа на второй
                    gelf_update_sym(data, i, &sym);     // Обновляет символ в таблице
                } else if (sym.st_value == address2) {  // Проверяет, совпадает ли адрес символа со вторым адресом
                    sym.st_value = address1;            // Меняет адрес символа на первый
                    gelf_update_sym(data, i, &sym);     // Обновляет символ в таблице
                }
            }
        }
    }
} */

// Функция запуска ELF-файла
void execute_elf(const std::string &file_path) {
    pid_t pid = fork();     // Создает новый процесс с помощью fork

    if (pid == 0) {     // Child process
        execl(file_path.c_str(), file_path.c_str(), (char *) nullptr);
        std::cerr << "Failed to execute file: " << file_path << std::endl;
        exit(EXIT_FAILURE);     // Выполняет ELF-файл
    } else if (pid < 0) {
        std::cerr << "Fork failed." << std::endl;
    } else {            // Parent process
        int status;
        waitpid(pid, &status, 0);   // Ожидает завершения дочернего процесса и получает его статус
        if (WIFEXITED(status)) {
            std::cout << "Process exited with status: " << WEXITSTATUS(status) << std::endl;
        } else {
            std::cerr << "Process did not exit normally." << std::endl;
        }
    }
}

// Функция установки прав на выполнение
void set_execute_permissions(const std::string &file_path) {
    // Устанавливает права на выполнение (0755)
    if (chmod(file_path.c_str(), 0755) != 0) {
        std::cerr << "Failed to set execute permissions on file: " << file_path << std::endl;
    }
}

// Генерация нового имени файла
std::string generate_output_filename(const std::string &input_file) {
    size_t dot_pos = input_file.rfind('.');
    if (dot_pos == std::string::npos) {
        return "output_" + input_file;
    } else {
        return "output_" + input_file.substr(0, dot_pos) + input_file.substr(dot_pos);
    }
}


// Функция для обмена адресов в инструкциях
void swap_addresses_in_text_section(const std::string &filename, size_t old_addr1, size_t old_addr2) {
    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);

    if (!file) {
        std::cerr << "Failed to open the file!" << std::endl;
        return;
    }

    // Поиск старых адресов в секции .text и их замена
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* file_data = new char[file_size];
    file.read(file_data, file_size);

    // Буферы для старых и новых адресов
    char buffer_old1[sizeof(size_t)], buffer_old2[sizeof(size_t)];
    std::memcpy(buffer_old1, &old_addr1, sizeof(size_t));
    std::memcpy(buffer_old2, &old_addr2, sizeof(size_t));

    // Цикл для поиска и замены адресов
    for (size_t i = 0; i < file_size - sizeof(size_t); ++i) {
        // Если нашли первый адрес, заменяем его на второй
        if (std::memcmp(file_data + i, buffer_old1, sizeof(size_t)) == 0) {
            file.seekp(i, std::ios::beg);
            file.write(reinterpret_cast<char*>(&old_addr2), sizeof(size_t));
            std::cout << "Replaced address " << std::hex << old_addr1 << " with " << old_addr2 << " at offset: " << i << std::endl;
        }
        // Если нашли второй адрес, заменяем его на первый
        else if (std::memcmp(file_data + i, buffer_old2, sizeof(size_t)) == 0) {
            file.seekp(i, std::ios::beg);
            file.write(reinterpret_cast<char*>(&old_addr1), sizeof(size_t));
            std::cout << "Replaced address " << std::hex << old_addr2 << " with " << old_addr1 << " at offset: " << i << std::endl;
        }
    }

    delete[] file_data;
    file.close();
}

// Функция для обмена значениями по адресам
//void swap_values_in_data_section(const char* filename, uintptr_t addr1, uintptr_t addr2, size_t size) {
void swap_values_in_data_section(const std::string &filename, size_t addr1, size_t addr2, size_t size) {
    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open the file!" << std::endl;
        return;
    }

    char* buffer1 = new char[size];
    char* buffer2 = new char[size];

    // Чтение значений по адресам
    file.seekg(addr1, std::ios::beg);
    file.read(buffer1, size);

    file.seekg(addr2, std::ios::beg);
    file.read(buffer2, size);

    // Запись значений в обратном порядке
    file.seekp(addr1, std::ios::beg);
    file.write(buffer2, size);

    file.seekp(addr2, std::ios::beg);
    file.write(buffer1, size);

    delete[] buffer1;
    delete[] buffer2;
    file.close();
}




//uintptr_t get_file_offset_by_virtual_address(const char* filename, uintptr_t virt_addr) {
size_t get_file_offset_by_virtual_address(const std::string &filename, size_t virt_addr) {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 0;
    }

    if (elf_version(EV_CURRENT) == EV_NONE) {
        std::cerr << "ELF library initialization failed." << std::endl;
        close(fd);
        return 0;
    }

    Elf* elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) {
        std::cerr << "Failed to open ELF descriptor." << std::endl;
        close(fd);
        return 0;
    }

    // Проходим по всем программным заголовкам
    size_t num_phdrs;
    if (elf_getphdrnum(elf, &num_phdrs) != 0) {
        std::cerr << "Failed to get number of program headers." << std::endl;
        elf_end(elf);
        close(fd);
        return 0;
    }

    for (size_t i = 0; i < num_phdrs; i++) {
        GElf_Phdr phdr;
        if (!gelf_getphdr(elf, i, &phdr)) {
            std::cerr << "Failed to get program header " << i << std::endl;
            continue;
        }

        // Проверяем, входит ли виртуальный адрес в этот сегмент
        if (virt_addr >= phdr.p_vaddr && virt_addr < phdr.p_vaddr + phdr.p_memsz) {
            uintptr_t offset = phdr.p_offset + (virt_addr - phdr.p_vaddr);
            elf_end(elf);
            close(fd);
            return offset; // Возвращаем смещение в файле
        }
    }

    elf_end(elf);
    close(fd);

    std::cerr << "Virtual address " << std::hex << virt_addr << " not found in any segment." << std::endl;
    return 0;
}


// Функция для чтения, вывода в hex виде и замены значений по адресам
//void print_and_swap_values(const char* filename, uintptr_t virt_addr1, uintptr_t virt_addr2, size_t size) {
void print_and_swap_values(const std::string &filename, size_t virt_addr1, size_t virt_addr2, size_t size) {
    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);

    if (!file) {
        std::cerr << "Failed to open the file!" << std::endl;
        return;
    }

    // Получаем физические смещения для каждого виртуального адреса
    /*uintptr_t offset1 = get_file_offset_by_virtual_address(filename, virt_addr1);
    uintptr_t offset2 = get_file_offset_by_virtual_address(filename, virt_addr2);*/
    size_t offset1 = get_file_offset_by_virtual_address(filename, virt_addr1);
    size_t offset2 = get_file_offset_by_virtual_address(filename, virt_addr2);

    if (offset1 == 0 || offset2 == 0) {
        std::cerr << "Failed to find offsets for the provided virtual addresses." << std::endl;
        return;
    }

    // Буферы для чтения значений по адресам
    char* buffer1 = new char[size];
    char* buffer2 = new char[size];

    // Чтение значений по смещению 1
    file.seekg(offset1, std::ios::beg);
    file.read(buffer1, size);

    // Чтение значений по смещению 2
    file.seekg(offset2, std::ios::beg);
    file.read(buffer2, size);

    // Вывод значений в hex формате перед заменой
    std::cout << "Value at address " << std::hex << virt_addr1 << " (offset: " << offset1 << "): ";
    for (size_t i = 0; i < size; ++i) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (static_cast<unsigned int>(static_cast<unsigned char>(buffer1[i]))) << " ";
    }
    std::cout << std::endl;

    std::cout << "Value at address " << std::hex << virt_addr2 << " (offset: " << offset2 << "): ";
    for (size_t i = 0; i < size; ++i) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (static_cast<unsigned int>(static_cast<unsigned char>(buffer2[i]))) << " ";
    }
    std::cout << std::endl;

    // Меняем местами значения
    file.seekp(offset1, std::ios::beg);
    file.write(buffer2, size);

    file.seekp(offset2, std::ios::beg);
    file.write(buffer1, size);

    std::cout << "Values swapped." << std::endl;

    // Освобождаем память
    delete[] buffer1;
    delete[] buffer2;

    // Закрываем файл
    file.close();
}


// Функция для чтения, вывода в hex виде и замены значений по адресам
/*void print_and_swap_values(const std::string &filename, size_t addr1, size_t addr2, size_t size) {
    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);

    if (!file) {
        std::cerr << "Failed to open the file!" << std::endl;
        return;
    }

    // Буферы для чтения значений по адресам
    char* buffer1 = new char[size];
    char* buffer2 = new char[size];

    // Чтение значений по адресу 1
    file.seekg(addr1, std::ios::beg);
    file.read(buffer1, size);

    // Чтение значений по адресу 2
    file.seekg(addr2, std::ios::beg);
    file.read(buffer2, size);

    // Вывод значений в hex формате перед заменой
    std::cout << "Value at address " << std::hex << addr1 << ": ";
    for (size_t i = 0; i < size; ++i) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (static_cast<unsigned char>(buffer1[i])) << " ";
    }
    std::cout << std::endl;

    std::cout << "Value at address " << std::hex << addr2 << ": ";
    for (size_t i = 0; i < size; ++i) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (static_cast<unsigned char>(buffer2[i])) << " ";
    }
    std::cout << std::endl;

    // Меняем местами значения
    file.seekp(addr1, std::ios::beg);
    file.write(buffer2, size);

    file.seekp(addr2, std::ios::beg);
    file.write(buffer1, size);

    std::cout << "Values swapped." << std::endl;

    // Освобождаем память
    delete[] buffer1;
    delete[] buffer2;

    // Закрываем файл
    file.close();
} */

// Основная функция для обработки ELF-файла
void process_elf_file(const std::string &input_file, size_t addr1, size_t addr2, size_t size) {
//void process_elf_file(const char* input_file, uintptr_t addr1, uintptr_t addr2, size_t size) {
    std::string output_file = generate_output_filename(input_file);
    //const char* output_file = generate_output_filename(input_file);
    std::vector<char> buffer;               // Буфер для хранения содержимого ELF-файла

    // Читаем ELF-файл в буфер
    read_elf_file(input_file, buffer);      // Читает ELF-файл в буфер

    // Записывает измененный буфер в новый ELF-файл
    write_elf_file(output_file, buffer);

    // Тут наверно надо будет делать цикл swap_byte_sequences(buffer, size); для всех символов

    // Показывает HEX и меняем сами значения по адресам
    print_and_swap_values(output_file, addr1, addr2, size);

    // Шаг 1: Поменять значения по адресам
    //swap_values_in_data_section(output_file, addr1, addr2, size);

    // Шаг 2: Поменять адреса в инструкциях
    //swap_addresses_in_text_section(output_file, addr1, addr2);



    // Меняет байты местами в буфере
    //swap_byte_sequences(buffer, address1, address2, size);

    // Записывает измененный буфер в новый ELF-файл
    //write_elf_file(output_file, buffer);

    // Открывает файл для чтения и записи
    /*int fd = open(output_file.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open file: " << output_file << std::endl;
        return;
    }*/

    //initialize_libelf();    // Инициализирует библиотеку libelf

    // Открывает ELF-файл для чтения и записи
    /*Elf *e = elf_begin(fd, ELF_C_RDWR, nullptr);
    if (!e) {
        std::cerr << "elf_begin() failed: " << elf_errmsg(-1) << std::endl;
        close(fd);
        return;
    }

    // Обновляет таблицу символов
    //update_symbol_table(e, address1, address2);

    elf_end(e);     // Закрывает ELF-файл
    close(fd);      // Закрывает файловый дескриптор */


    // Устанавливаем права на выполнение
    set_execute_permissions(output_file);

    // Запускаем выходной файл
    execute_elf(output_file);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <input_elf_file> <address1> <address2> <size>" << std::endl;
        return EXIT_FAILURE;
    }
    
    std::string input_file = argv[1];
    size_t address1 = std::stoul(argv[2], nullptr, 16);     // Получает первый адрес в шестнадцатеричном формате
    size_t address2 = std::stoul(argv[3], nullptr, 16);
    size_t size = std::stoul(argv[4]);
    /*const char* input_file = argv[1];
    uintptr_t address1 = std::stoul(argv[2], nullptr, 16);
    uintptr_t address2 = std::stoul(argv[3], nullptr, 16);
    size_t size = std::stoul(argv[4]);*/


    // Печать таблиц символов и строк
    //print_symbol_table(input_file);
    //print_string_table(input_file);

    // Печать 8 байтных символов
    //print_symbol_8_byte();
    
    process_elf_file(input_file, address1, address2, size);    // Вызывает функцию обработки ELF-файла

    return EXIT_SUCCESS;        // Завершает программу успешно
}

/*int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <filename> <addr1> <addr2> <size>" << std::endl;
        return 1;
    }

    const char* filename = argv[1];
    uintptr_t addr1 = std::stoul(argv[2], nullptr, 16);
    uintptr_t addr2 = std::stoul(argv[3], nullptr, 16);
    size_t size = std::stoul(argv[4]);

    // Шаг 1: Поменять значения по адресам
    swap_values_in_data_section(filename, addr1, addr2, size);

    // Шаг 2: Поменять адреса в инструкциях
    swap_addresses_in_text_section(filename, addr1, addr2);

    return 0;
} */




/*Symbol: _ZL9prevTotal
        Address: 0x5020
        Size: 8
        Type: 1
        Bind: 0
        Other: 0
        Section Index: 26
Symbol: _ZL8prevIdle
        Address: 0x5028
        Size: 8
        Type: 1
        Bind: 0
        Other: 0
        Section Index: 26


Symbol: _ZL9prevTotal
        Address: 0x5028
        Size: 8
        Type: 1
        Bind: 0
        Other: 0
        Section Index: 26
Symbol: _ZL8prevIdle
        Address: 0x5020
        Size: 8
        Type: 1
        Bind: 0
        Other: 0
        Section Index: 26*/
