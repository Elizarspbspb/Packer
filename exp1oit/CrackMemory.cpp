#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// sudo ./pid 16295 0x72097a96c108 0x00000000003e
// sudo ./pid PID addres value


void modify_process_memory(pid_t target_pid, void* target_addr, long new_data) {
    // Attach to the target process
    // Прикрепляется к целевому процессу с PID
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace attach");
        exit(EXIT_FAILURE);
    }

    // Wait for the target process to stop
    // Ждет, пока целевой процесс остановится. Это необходимо, чтобы убедиться, что процесс готов для изменения памяти.
    waitpid(target_pid, NULL, 0);

    // Modify the memory at the target address
    // Изменяет данные по адресу target_addr в памяти целевого процесса, записывая туда new_data.
    if (ptrace(PTRACE_POKETEXT, target_pid, target_addr, new_data) == -1) {
        perror("ptrace poketext");
        exit(EXIT_FAILURE);
    }

    // Detach from the target process
    // Отсоединяется от целевого процесса, позволяя ему продолжить выполнение.
    if (ptrace(PTRACE_DETACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace detach");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <pid> <address> <new data>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t target_pid = atoi(argv[1]);
    void* target_addr = (void*)strtoul(argv[2], NULL, 0);
    long new_data = strtol(argv[3], NULL, 0);

    // подставновка нужного значения
    modify_process_memory(target_pid, target_addr, new_data);

    printf("Modified process %d memory at address %p with new data 0x%lx\n", target_pid, target_addr, new_data);

    return 0;
}


// подстановка новой функции с помощью нового кода
/*void modify_process_memory_add_new_function(pid_t target_pid, void* target_addr, const char* new_code, size_t code_size) {
    // Attach to the target process
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace attach");
        exit(EXIT_FAILURE);
    }

    // Wait for the target process to stop
    waitpid(target_pid, NULL, 0);

    // Write the new code to the target address
    for (size_t i = 0; i < code_size; i += sizeof(long)) {
        long data = *(long*)(new_code + i);
        if (ptrace(PTRACE_POKETEXT, target_pid, target_addr + i, data) == -1) {
            perror("ptrace poketext");
            exit(EXIT_FAILURE);
        }
    }

    // Detach from the target process
    if (ptrace(PTRACE_DETACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace detach");
        exit(EXIT_FAILURE);
    }
}*/


/*
 // компиляция кода
 // nasm -f bin -o new_function.bin new_function.asm
section .text
    global _start
_start:
    ; Пример новой функции
    ; Добавьте здесь нужный ассемблерный код
    mov rax, 1          ; код системного вызова для выхода
    xor rdi, rdi        ; код возврата 0
    syscall             ; вызов системного вызова
*/
