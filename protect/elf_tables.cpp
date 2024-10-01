#include "elf_tables.h"

void print_symbol_table(const std::string &file_path) {
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    initialize_libelf();

    Elf *e = elf_begin(fd, ELF_C_READ, nullptr);
    if (!e) {
        std::cerr << "elf_begin() failed: " << elf_errmsg(-1) << std::endl;
        close(fd);
        return;
    }

    Elf_Scn *scn = nullptr;
    GElf_Shdr shdr;

    while ((scn = elf_nextscn(e, scn)) != nullptr) {
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            std::cerr << "gelf_getshdr() failed: " << elf_errmsg(-1) << std::endl;
            continue;
        }

        if (shdr.sh_type == SHT_SYMTAB) {
            Elf_Data *data = elf_getdata(scn, nullptr);
            if (!data) {
                std::cerr << "elf_getdata() failed: " << elf_errmsg(-1) << std::endl;
                continue;
            }

            size_t symbol_count = shdr.sh_size / shdr.sh_entsize;
            for (size_t i = 0; i < symbol_count; ++i) {
                GElf_Sym sym;
                if (gelf_getsym(data, i, &sym) != &sym) {
                    std::cerr << "gelf_getsym() failed: " << elf_errmsg(-1) << std::endl;
                    continue;
                }

                char *name = elf_strptr(e, shdr.sh_link, sym.st_name);
                std::cout << "Symbol: " << (name ? name : "")
                          << "\n\tAddress: 0x" << std::hex << sym.st_value
                          << "\n\tSize: " << std::dec << sym.st_size
                          << "\n\tType: " << GELF_ST_TYPE(sym.st_info)
                          << "\n\tBind: " << GELF_ST_BIND(sym.st_info)
                          << "\n\tOther: " << static_cast<int>(sym.st_other)
                          << "\n\tSection Index: " << sym.st_shndx << std::endl;
                // Это будут поля структуры...
            }
        }
    }

    elf_end(e);
    close(fd);
}

void print_string_table(const std::string &file_path) {
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    initialize_libelf();

    Elf *e = elf_begin(fd, ELF_C_READ, nullptr);
    if (!e) {
        std::cerr << "elf_begin() failed: " << elf_errmsg(-1) << std::endl;
        close(fd);
        return;
    }

    Elf_Scn *scn = nullptr;
    GElf_Shdr shdr;

    while ((scn = elf_nextscn(e, scn)) != nullptr) {
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            std::cerr << "gelf_getshdr() failed: " << elf_errmsg(-1) << std::endl;
            continue;
        }

        if (shdr.sh_type == SHT_STRTAB) {
            Elf_Data *data = elf_getdata(scn, nullptr);
            if (!data) {
                std::cerr << "elf_getdata() failed: " << elf_errmsg(-1) << std::endl;
                continue;
            }

            char *strings = static_cast<char*>(data->d_buf);
            size_t size = data->d_size;
            for (size_t i = 0; i < size;) {
                if (strings[i] != '\0') {
                    std::cout << "String: " << &strings[i] 
                              << " Address: 0x" << std::hex << (shdr.sh_addr + i) 
                              << " Size: " << std::dec << strlen(&strings[i]) << std::endl;
                    i += strlen(&strings[i]) + 1;
                } else {
                    ++i;
                }
            }
        }
    }

    elf_end(e);
    close(fd);
}

void print_symbol_8_byte() {
// Смысл функции искать символы одинакового размера ... и менять их между собой
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
Symbol: _ZL9prevUsage
        Address: 0x5030
        Size: 8
        Type: 1
        Bind: 0
        Other: 0
        Section Index: 26*/
}
