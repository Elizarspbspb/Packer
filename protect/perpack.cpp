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

#include "elf_tables.h"

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
bool find_symbol_by_address(const std::vector<char> &buffer, size_t address, size_t &offset, size_t size) {
    auto it = std::search(buffer.begin(), buffer.end(), reinterpret_cast<const char*>(&address), reinterpret_cast<const char*>(&address) + sizeof(address));    // Ищет адрес в буфере
    if (it != buffer.end()) {
        offset = std::distance(buffer.begin(), it);     // Вычисляет смещение найденного адреса
        return true;
    }
    return false;
}

// Меняет местами байты двух последовательностей в буфере
void swap_byte_sequences(std::vector<char> &buffer, size_t address1, size_t address2, size_t size) {
    size_t offset1, offset2;
    
    // Находит смещения для двух адресов
    if (find_symbol_by_address(buffer, address1, offset1, size) && find_symbol_by_address(buffer, address2, offset2, size)) {
        // Сохраняет байты первой последовательности во временный буфер
        std::vector<char> temp(buffer.begin() + offset1, buffer.begin() + offset1 + size);
        // Копирует байты из второй последовательности на место первой
        std::copy(buffer.begin() + offset2, buffer.begin() + offset2 + size, buffer.begin() + offset1);
        // Копирует байты из временного буфера на место второй последовательности
        std::copy(temp.begin(), temp.end(), buffer.begin() + offset2);
    } else {
        std::cerr << "One or both addresses not found in the buffer." << std::endl;
    }
}

// Обновляет таблицу символов ELF-файла
void update_symbol_table(Elf *e, size_t address1, size_t address2) {
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
}

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

// Основная функция для обработки ELF-файла
void process_elf_file(const std::string &input_file, size_t address1, size_t address2, size_t size) {
    std::string output_file = generate_output_filename(input_file);
    std::vector<char> buffer;               // Буфер для хранения содержимого ELF-файла
    read_elf_file(input_file, buffer);      // Читает ELF-файл в буфер

    // Меняет байты местами в буфере
    swap_byte_sequences(buffer, address1, address2, size);

    // Записывает измененный буфер в новый ELF-файл
    write_elf_file(output_file, buffer);

    // Открывает файл для чтения и записи
    int fd = open(output_file.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open file: " << output_file << std::endl;
        return;
    }

    initialize_libelf();    // Инициализирует библиотеку libelf

    // Открывает ELF-файл для чтения и записи
    Elf *e = elf_begin(fd, ELF_C_RDWR, nullptr);
    if (!e) {
        std::cerr << "elf_begin() failed: " << elf_errmsg(-1) << std::endl;
        close(fd);
        return;
    }

    // Обновляет таблицу символов
    update_symbol_table(e, address1, address2);

    elf_end(e);     // Закрывает ELF-файл
    close(fd);      // Закрывает файловый дескриптор
    
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

    // Печать таблиц символов и строк
    print_symbol_table(input_file);
    print_string_table(input_file);
    
    process_elf_file(input_file, address1, address2, size);    // Вызывает функцию обработки ELF-файла

    return EXIT_SUCCESS;        // Завершает программу успешно
}
