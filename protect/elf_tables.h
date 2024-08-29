#ifndef ELF_TABLES_H
#define ELF_TABLES_H

#include <iostream>
#include <libelf.h>
#include <gelf.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

// Инициализация библиотеки libelf
void initialize_libelf();

// Функции печати таблиц
void print_symbol_table(const std::string &file_path);
void print_string_table(const std::string &file_path);

#endif // ELF_TABLES_H
