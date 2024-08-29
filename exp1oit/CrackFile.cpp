#include <iostream>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>

// g++ -o replace_path replace_path.cpp
// sudo ./replace_path <pid> <address> <new_path>


void modify_process_memory(pid_t target_pid, void* target_addr, const char* new_data) {
    // Attach to the target process
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace attach");
        exit(EXIT_FAILURE);
    }

    // Wait for the target process to stop
    waitpid(target_pid, NULL, 0);

    // Calculate the length of the new data
    size_t len = strlen(new_data) + 1; // Include null terminator

    // Write the new data to the target address
    for (size_t i = 0; i < len; i += sizeof(long)) {
        long word;
        memcpy(&word, new_data + i, sizeof(long));
        if (ptrace(PTRACE_POKETEXT, target_pid, (void*)((char*)target_addr + i), word) == -1) {
            perror("ptrace poketext");
            ptrace(PTRACE_DETACH, target_pid, NULL, NULL);
            exit(EXIT_FAILURE);
        }
    }

    // Detach from the target process
    if (ptrace(PTRACE_DETACH, target_pid, NULL, NULL) == -1) {
        perror("ptrace detach");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <pid> <address> <new path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t target_pid = atoi(argv[1]);
    void* target_addr = (void*)strtoul(argv[2], NULL, 0);
    const char* new_data = argv[3];

    // подставновка нужного значения
    modify_process_memory(target_pid, target_addr, new_data);

    printf("Modified process %d memory at address %p with new path \"%s\"\n", target_pid, target_addr, new_data);

    return 0;
}
