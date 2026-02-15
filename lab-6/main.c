#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PASSWORD_FILE "password.txt"
#define MAX_LEN 64
#define XOR_KEY 0x41

int check_password(); // функция для сравнения вводимого пароля с истинным (п.1)
void coding_password(); // функция читает зашифрованный пароль и дешифрует его (п.8)

int main(void) {
    coding_password();
    if (!check_password()) { // cbnz w0, LBB0_2
        fprintf(stderr, "access denied!"); // LBB0_1
        abort();
    }
    fprintf(stdout, "access allowed!"); // LBB0_2
    return 0;
}

int check_password() {
    char input[MAX_LEN] = {0};

    printf("enter password: ");
    if (fgets(input, MAX_LEN, stdin) == NULL) {
        fprintf(stderr, "reading input error");
        return 0;
    }
    
    size_t input_len = strlen(input);
    if (input_len > 0 && input[input_len - 1] == '\n') {
        input[input_len - 1] = '\0';
        input_len--;
    }

    for (int i = 0; i < input_len; i++) {
        input[i] ^= XOR_KEY;
    }

    FILE* file = fopen(PASSWORD_FILE, "r");
    if (file == NULL) {
        fprintf(stderr, "opening file error");
        abort();
    }
    char authentic_password[MAX_LEN] = {0};
    if (fgets(authentic_password, MAX_LEN, file) == NULL) {
        fprintf(stderr, "reading file error");
        return 0;
    }
    size_t auth_len = strlen(authentic_password);
    if (auth_len > 0 && authentic_password[auth_len - 1] == '\n') {
        authentic_password[auth_len - 1] = '\0';
        auth_len--;
    }
    return strncmp(authentic_password, input, auth_len) == 0 && input_len == auth_len;
}

void coding_password() {
    FILE* file = fopen(PASSWORD_FILE, "r");
    if (file == NULL) {
        fprintf(stderr, "opening file error");
        abort();
    }
    char buffer[MAX_LEN] = {0};
    int len = fread(buffer, 1, MAX_LEN - 1, file);
    buffer[len] = '\0';
    fclose(file);
    
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }

    file = fopen(PASSWORD_FILE, "w");
    if (file == NULL) {
        fprintf(stderr, "opening file error");
        abort();
    }

    for (int i = 0; i < len; i++) {
        fprintf(file, "%c", buffer[i] ^ XOR_KEY);
    }
    fclose(file);
}
