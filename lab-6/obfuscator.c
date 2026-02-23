#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

typedef struct {
    int remove_comments;
    int remove_whitespace;
    int rename_variables;
    int rename_functions;
    bool is_numeric;
    bool is_string;
    int shuffle_functions;
    int add_junk_variables;
    int add_junk_functions;
    int add_junk_loops;
    char variable_prefix[32];
    char function_prefix[32];
    int junk_var_count;
    int junk_func_count;
    int junk_loop_count;
} Config;

typedef struct {
    char old_name[64];
    char new_name[64];
    char type[8]; // var или func
} Identifier;

typedef struct {
    char *text;
    int is_main;
} Function;

char* remove_comments(const char *input);
char* remove_whitespace(const char *input);
void generate_new_name(char *output, int counter, Config *config, const char *type);
int is_word_boundary(char c);
char* replace_whole_word(const char *text, const char *old_word, const char *new_word);
int is_standard_function(const char *name);
int find_identifiers(const char *input, Identifier *identifiers, int max_count);
char* rename_identifiers(const char *input, Config *config);
char* add_junk_variables(const char *input, int count);
char* add_junk_functions(const char *input, int count);
char* add_junk_loops(const char *input, int count);
char* extract_prototype(const char *func_text);
char* create_prototypes(Function *functions, int count);
int extract_functions(const char *input, Function *functions, int max_count);
char* shuffle_functions(const char *input);
char* generate_random_string(int length);
void trim(char *str);
void parse_config_line(char *line, Config *config);
Config load_config(const char *filename);
FILE* open_and_valid(const char* filename);
char* file_to_buffer(FILE* file);
void write_file(const char *filename, const char *content);

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "execute error: obfuscator input.c output.c config.txt");
        abort();
    }

    const char *input = argv[1];
    const char *output = argv[2];
    const char *config = argv[3];

    FILE* input_file = open_and_valid(input);

    char* code = file_to_buffer(input_file); // массив текста input.c
    Config cur_config = load_config(config); // структура с параметрами обфускации

    srand(time(NULL));

    char *temp;

    // удаление комментариев
    if (cur_config.remove_comments) {
        temp = remove_comments(code);
        free(code);
        code = temp;
    }

    // удаление лишних пробелов
    if (cur_config.remove_whitespace) {
        temp = remove_whitespace(code);
        free(code);
        code = temp;
    }
    
    // переименование переменных и функций - todo
    if (cur_config.rename_variables || cur_config.rename_functions) {
        temp = rename_identifiers(code, &cur_config);
        free(code);
        code = temp;
    }

    // добавление мусорных переменных
    if (cur_config.add_junk_variables) {
        temp = add_junk_variables(code, cur_config.junk_var_count);
        free(code);
        code = temp;
    }

    // добавление мусорных функций
    if (cur_config.add_junk_functions) {
        temp = add_junk_functions(code, cur_config.junk_func_count);
        free(code);
        code = temp;
    }

    // добавление мусорных циклов
    if (cur_config.add_junk_loops) {
        temp = add_junk_loops(code, cur_config.junk_loop_count);
        free(code);
        code = temp;
    }

    // перемешивание функций
    if (cur_config.shuffle_functions) {
        temp = shuffle_functions(code);
        free(code);
        code = temp;
    }

    write_file(output, code);
    free(code);
    fclose(input_file);

    return 0;
}

char* remove_comments(const char *input) {
    size_t len = strlen(input);
    char* result = malloc(len + 1);
    if (result == NULL) {
        fprintf(stderr, "allocation memory error");
        return NULL;
    }

    int i = 0; int j = 0; 
    bool in_string = false; bool in_char_literal = false;
    while (i < len) {
        if (input[i] == '"' && (i == 0 || input[i-1] != '\\')) {
            in_string = !in_string;
            result[j++] = input[i++];
            continue;
        }
        if (input[i] == '\'' && (i == 0 || input[i-1] != '\\')) {
            in_char_literal = !in_char_literal;
            result[j++] = input[i++];
            continue;
        }
        if (in_string || in_char_literal) {
            result[j++] = input[i++];
            continue;
        }
        if (input[i] == '/' && input[i+1] == '*') {
            i += 2;
            while (i < len && !(input[i] == '*' && input[i+1] == '/')) {
                i++;
            }
            i += 2; // пропустить */
            continue;
        }
        if (input[i] == '/' && input[i+1] == '/') {
            while (i < len && input[i] != '\n') {
                i++;
            }
            if (i < len && input[i] == '\n') {
                result[j++] = '\n';
                i++;
            }
            continue;
        }
        result[j++] = input[i++];
    }
    result[j] = '\0';
    return result;
}

char* remove_whitespace(const char *input) {
    size_t len = strlen(input);
    char* result = malloc(len + 1);
    if (result == NULL) {
        fprintf(stderr, "allocation memory error");
        return NULL;
    }

    int i = 0; int j = 0;
    int prev_was_space = 0;
    int in_string = 0;
    int in_preprocessor = 0;
    
    while (i < len) {
        if (input[i] == '#' && (i == 0 || input[i-1] == '\n')) {
            in_preprocessor = 1;
        }
        if (in_preprocessor) {
            result[j++] = input[i];
            if (input[i] == '\n') { // \n необходим после директив
                if (i > 0 && input[i-1] == '\\') {
                } else {
                    in_preprocessor = 0;
                    prev_was_space = 1;
                }
            }
            i++;
            continue;
        }
        if (input[i] == '"' && (i == 0 || input[i-1] != '\\')) {
            in_string = !in_string;
            result[j++] = input[i++];
            prev_was_space = 0;
            continue;
        }
        if (in_string) {
            result[j++] = input[i++];
            continue;
        }
        if (input[i] == ' ' || input[i] == '\t' || input[i] == '\n') {
            if (!prev_was_space) {
                result[j++] = ' ';
                prev_was_space = 1;
            }
            i++;
            continue;
        }
        if (input[i] == '\n') {
            result[j++] = ' ';
            i++;
            prev_was_space = 1;
            continue;
        }

        result[j++] = input[i++];
        prev_was_space = 0;
    }
    
    result[j] = '\0';
    return result;
}

void generate_new_name(char *output, int counter, Config *config, const char *type) {
    const char *prefix = (strcmp(type, "var") == 0) ? config->variable_prefix : config->function_prefix;
    if (config->is_numeric && !config->is_string) {
        snprintf(output, 64, "%s%d", prefix, counter);
    }
    else if (!config->is_numeric && config->is_string) {
        char *random = generate_random_string(8);
        snprintf(output, 64, "%s%s", prefix, random);
    }
    else if (config->is_numeric && config->is_string) {
        char *random = generate_random_string(8);
        snprintf(output, 64, "%s%d%s", prefix, counter, random);
    }
    else {
        fprintf(stderr, "wrong params in config file");
        abort();
    }
}

int is_word_boundary(char c) {
    return !isalnum(c) && c != '_';
}

char* replace_whole_word(const char *text, const char *old_word, const char *new_word) {
    size_t text_len = strlen(text);
    size_t old_len = strlen(old_word);
    size_t new_len = strlen(new_word);
    
    size_t result_size = text_len + (text_len * (new_len > old_len ? (new_len - old_len) : 0)) + 1;
    char *result = malloc(result_size);
    if (result == NULL) {
        fprintf(stderr, "allocation memory error");
        return NULL;
    }

    size_t j = 0;
    int in_string = 0;
    for (size_t i = 0; i < text_len; ) {
        if (text[i] == '"' && (i == 0 || text[i-1] != '\\')) {
            in_string = !in_string;
            result[j++] = text[i++];
            continue;
        }
        if (in_string) {
            result[j++] = text[i++];
            continue;
        }
        if (i + old_len <= text_len && strncmp(text + i, old_word, old_len) == 0) {
            char prev = (i > 0) ? text[i-1] : ' ';
            char next = (i + old_len < text_len) ? text[i + old_len] : '\0';
            
            if (is_word_boundary(prev) && is_word_boundary(next)) {
                strcpy(result + j, new_word);
                j += new_len;
                i += old_len;
                continue;
            }
        }
        result[j++] = text[i++];
    }
    
    result[j] = '\0';
    return result;
}

int is_standard_function(const char *name) {
    const char *std_funcs[] = {
        "printf", "scanf", "fprintf", "sprintf", "snprintf",
        "malloc", "free", "calloc", "realloc",
        "fopen", "fclose", "fread", "fwrite", "fgets", "fputs",
        "strlen", "strcmp", "strcpy", "strncpy", "strcat",
        "memset", "memcpy", "memmove",
        "atoi", "atof", "strtol",
        "main",
        NULL
    };
    
    for (int i = 0; std_funcs[i] != NULL; i++) {
        if (strcmp(name, std_funcs[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int find_identifiers(const char *input, Identifier *identifiers, int max_count) {
    const char *types[] = {"unsigned int","unsigned char", "int", "char",
         "float", "double", "void", "long", "short", "size_t", NULL};
    int count = 0;
    size_t len = strlen(input);
    
    for (size_t i = 0; i < len && count < max_count; i++) {
        for (int t = 0; types[t] != NULL; t++) {
            size_t type_len = strlen(types[t]);
            if (strncmp(input + i, types[t], type_len) == 0 && is_word_boundary(input[i + type_len])) {
                i += type_len;
                while (i < len && isspace(input[i])) i++;

                if (i >= len || !isalpha(input[i])) continue;
                
                size_t name_start = i;
                while (i < len && (isalnum(input[i]) || input[i] == '_')) {
                    i++;
                }
                
                size_t name_len = i - name_start;
                if (name_len >= 64 || name_len == 0) continue;
                
                char name[64];
                strncpy(name, input + name_start, name_len);
                name[name_len] = '\0';
                
                if (is_standard_function(name)) continue;      
                
                int already_exists = 0;
                for (int k = 0; k < count; k++) {
                    if (strcmp(identifiers[k].old_name, name) == 0) {
                        already_exists = 1;
                        break;
                    }
                }
                if (already_exists) continue;
                
                while (i < len && isspace(input[i])) i++;

                strcpy(identifiers[count].old_name, name);
                if (i < len && input[i] == '(') {
                    strcpy(identifiers[count].type, "func");
                } else {
                    strcpy(identifiers[count].type, "var");
                }
                //printf("found: '%s' (type: %s)\n", name, identifiers[count].type);
                count++;
                break;
            }
        }
    }
    
    return count;
}

char* rename_identifiers(const char *input, Config *config) {
    Identifier identifiers[1000];
    int count = find_identifiers(input, identifiers, 1000);

    int var_counter = 0;
    int func_counter = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(identifiers[i].type, "var") == 0) {
            generate_new_name(identifiers[i].new_name, var_counter++, config, "var");
        } else {
            generate_new_name(identifiers[i].new_name, func_counter++, config, "func");
        }
        //printf("[DEBUG] %s = %s (%s)\n", identifiers[i].old_name, identifiers[i].new_name, identifiers[i].type);
    }

    char *result = strdup(input);
    
    for (int i = 0; i < count; i++) {        
        char *new_result = replace_whole_word(result, identifiers[i].old_name, identifiers[i].new_name);
        if (new_result == NULL) {
            free(result);
            return strdup(input);
        }       
        free(result);
        result = new_result;
    }
    return result;
}

char* add_junk_variables(const char *input, int count) {
    char junk[2048] = {0};
    for (int i = 0; i < count; i++) {
        char line[128];
        snprintf(line, sizeof(line), "int _junk%d=%d; _junk%d++; ", i, i, i);
        strcat(junk, line);
    }
    
    const char *main_pos = strstr(input, "int main");
    if (!main_pos) {
        return strdup(input);
    }
    
    const char *bracket = strchr(main_pos, '{');
    if (!bracket) {
        return strdup(input);
    }
    
    size_t pos = bracket - input + 1;
    
    size_t result_size = strlen(input) + strlen(junk) + 10;
    char *result = malloc(result_size);
    
    strncpy(result, input, pos);
    result[pos] = '\0';   
    strcat(result, junk);
    strcat(result, input + pos);
    
    return result;
}

char* add_junk_functions(const char *input, int count) {
    char junk_defs[4096] = {0};
    char junk_calls[1024] = {0};
    
    for (int i = 0; i < count; i++) {
        char func[256];
        snprintf(func, sizeof(func), " void _dead%d(void) { int _x%d = %d; _x%d *= 2;} ", i, i, i, i);
        strcat(junk_defs, func);
        
        char call[64];
        snprintf(call, sizeof(call), "_dead%d(); ", i);  // ← новое
        strcat(junk_calls, call);
    }
    
    const char *last_include = NULL;
    const char *last_define = NULL;
    const char *p = input;
    
    while ((p = strstr(p, "#include")) != NULL) {
        last_include = p;
        p++;
    }
    
    p = input;
    while ((p = strstr(p, "#define")) != NULL) {
        last_define = p;
        p++;
    }
    
    const char *last_directive = last_include;
    if (last_define && (!last_include || last_define > last_include)) {
        last_directive = last_define;
    }
    
    size_t insert_pos = 0;
    if (last_directive) {
        const char *newline = strchr(last_directive, '\n');
        if (newline) {
            insert_pos = newline - input + 1;
        }
    }

    size_t result_size = strlen(input) + strlen(junk_defs) + strlen(junk_calls) + 10;
    char *temp = malloc(result_size);
    strncpy(temp, input, insert_pos);
    temp[insert_pos] = '\0';
    strcat(temp, junk_defs);
    strcat(temp, input + insert_pos);
    
    const char *main_pos = strstr(temp, "int main");
    if (!main_pos) return temp;
    const char *bracket = strchr(main_pos, '{');
    if (!bracket) return temp;
    
    size_t pos = bracket - temp + 1;
    char *result = malloc(result_size);
    strncpy(result, temp, pos);
    result[pos] = '\0';
    strcat(result, junk_calls);
    strcat(result, temp + pos);
    free(temp);
    
    return result;
}

char* add_junk_loops(const char *input, int count) {
    char junk[4096] = {0};
    for (int i = 0; i < count; i++) {
        char loop[256];
        int iterations = (rand() % 10) + 1;
        snprintf(loop, sizeof(loop),
            "{ int _loop%d = 0; for (int _li%d = 0; _li%d < %d; _li%d++) { _loop%d += _li%d; } } ",
            i, i, i, iterations, i, i, i);
        strcat(junk, loop);
    }

    const char *main_pos = strstr(input, "int main");
    if (!main_pos) {
        return strdup(input);
    }

    const char *bracket = strchr(main_pos, '{');
    if (!bracket) {
        return strdup(input);
    }

    size_t pos = bracket - input + 1;

    size_t result_size = strlen(input) + strlen(junk) + 10;
    char *result = malloc(result_size);

    strncpy(result, input, pos);
    result[pos] = '\0';
    strcat(result, junk);
    strcat(result, input + pos);

    return result;
}

char* extract_prototype(const char *func_text) {
    size_t len = strlen(func_text);
    char *prototype = malloc(len + 10);
    if (prototype == NULL) return NULL;
    
    const char *bracket = strchr(func_text, '{');
    if (!bracket) {
        free(prototype);
        return NULL;
    }
    
    size_t proto_len = bracket - func_text;
    
    strncpy(prototype, func_text, proto_len);
    prototype[proto_len] = '\0';
    
    while (proto_len > 0 && isspace(prototype[proto_len-1])) {
        prototype[--proto_len] = '\0';
    }
    
    strcat(prototype, ";");
    
    return prototype;
}

char* create_prototypes(Function *functions, int count) {
    size_t total_size = 1;
    
    for (int i = 0; i < count; i++) {
        total_size += strlen(functions[i].text) + 10;
    }
    
    char *prototypes = malloc(total_size);
    if (!prototypes) return NULL;
    
    prototypes[0] = '\0';
    
    for (int i = 0; i < count; i++) {
        char *proto = extract_prototype(functions[i].text);
        if (proto && !strstr(proto, "main")) {
            strcat(prototypes, proto);
            free(proto);
        }
    }
    
    strcat(prototypes, "\n");
    return prototypes;
}

char* shuffle_functions(const char *input) {
    size_t len = strlen(input);
    size_t first_func_pos = 0;
    int bracket_level = 0;
    int found_func = 0;
    
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '{') {
            if (bracket_level == 0 && !found_func) {
                first_func_pos = i;
                while (first_func_pos > 0 && 
                       input[first_func_pos-1] != '\n' && 
                       input[first_func_pos-1] != ';' && 
                       input[first_func_pos-1] != '}') {
                    first_func_pos--;
                }
                while (first_func_pos < i && isspace(input[first_func_pos])) {
                    first_func_pos++;
                }
                found_func = 1;
                break;
            }
            bracket_level++;
        }
        else if (input[i] == '}') {
            bracket_level--;
        }
    }
    
    if (!found_func) {
        return strdup(input);
    }
    
    char *header = malloc(first_func_pos + 10);
    if (!header) return strdup(input);
    
    strncpy(header, input, first_func_pos);
    header[first_func_pos] = '\0';
    
    Function functions[100];
    int count = extract_functions(input + first_func_pos, functions, 100);
    
    if (count == 0) {
        free(header);
        return strdup(input);
    }
    
    int main_index = -1;
    for (int i = 0; i < count; i++) {
        if (functions[i].is_main) {
            main_index = i;
            break;
        }
    }
    
    for (int i = 0; i < count - 1; i++) {
        if (i == main_index) continue;
        
        int j;
        do {
            j = rand() % count;
        } while (j == main_index);
        
        Function temp = functions[i];
        functions[i] = functions[j];
        functions[j] = temp;
    }
    
    if (main_index != -1 && main_index != count - 1) {
        Function temp = functions[main_index];
        functions[main_index] = functions[count-1];
        functions[count-1] = temp;
    }
    
    char *prototypes = create_prototypes(functions, count);
    
    size_t total_size = strlen(header) + strlen(prototypes) + 100;
    for (int i = 0; i < count; i++) {
        total_size += strlen(functions[i].text) + 10;
    }
    
    char *result = malloc(total_size);
    if (!result) {
        free(header);
        free(prototypes);
        return strdup(input);
    }
    
    strcpy(result, header); // header (все #include, #define, typedef, глобальные переменные)
    free(header);
    
    strcat(result, prototypes);
    free(prototypes);
    
    for (int i = 0; i < count; i++) {
        strcat(result, functions[i].text);
        free(functions[i].text);
    }
    
    return result;
}

int extract_functions(const char *input, Function *functions, int max_count) {
    int count = 0; // кол-во функций
    int bracket_level = 0; // уровень вложенности
    int in_function = 0;
    size_t func_start = 0;
    size_t len = strlen(input);
    
    for (size_t i = 0; i < len && count < max_count; i++) {
        if (input[i] == '{') {
            if (bracket_level == 0) { // новая функция
                func_start = i;
                while (func_start > 0 && input[func_start-1] != '\n' && 
                       input[func_start-1] != ';' && input[func_start-1] != '}') {
                    func_start--;
                } // пока не дошли до начала фукнции
                while (func_start < i && isspace(input[func_start])) {
                    func_start++;
                }
                in_function = 1;
            }
            bracket_level++; // повышаем уровень вложенности
        }
        else if (input[i] == '}') {
            bracket_level--;
            if (bracket_level == 0 && in_function) { // нашли конец функции
                size_t func_len = i - func_start + 1;
                functions[count].text = malloc(func_len + 1);
                strncpy(functions[count].text, input + func_start, func_len);
                functions[count].text[func_len] = '\0'; 
                
                functions[count].is_main = (strstr(functions[count].text, "main") != NULL);
                
                count++;
                in_function = 0;
            }
        }
    }
    
    return count;
}

char* generate_random_string(int length) {
    char *str = malloc(length + 1);
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    for (int i = 0; i < length; i++) {
        int index = rand() % (sizeof(charset) - 1);
        str[i] = charset[index];
    }
    str[length] = '\0';
    
    return str;
}

void trim(char *str) { // удаление пробелов по бокам
    int len = strlen(str);
    while (len > 0 && isspace(str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
    char *start = str;
    while (*start && isspace(*start)) {
        start++;
    }
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

void parse_config_line(char *line, Config *config) { // парсинг строки вида param=value
    trim(line);

    char *eq = strchr(line, '=');
    if (!eq) {
        return;
    }

    *eq = '\0';
    char *key = line;
    char *value = eq + 1;
    trim(key);
    trim(value);

    if (strcmp(key, "remove_comments") == 0) {
        config->remove_comments = atoi(value);
    }
    else if (strcmp(key, "remove_whitespace") == 0) {
        config->remove_whitespace = atoi(value);
    }
    else if (strcmp(key, "rename_variables") == 0) {
        config->rename_variables = atoi(value);
    }
    else if (strcmp(key, "rename_functions") == 0) {
        config->rename_functions = atoi(value);
    }
    else if (strcmp(key, "is_numeric") == 0) {
        config->is_numeric = atoi(value);
    }
    else if (strcmp(key, "is_string") == 0) {
        config->is_string = atoi(value);
    }
    else if (strcmp(key, "shuffle_functions") == 0) {
        config->shuffle_functions = atoi(value);
    }
    else if (strcmp(key, "add_junk_variables") == 0) {
        config->add_junk_variables = atoi(value);
    }
    else if (strcmp(key, "add_junk_functions") == 0) {
        config->add_junk_functions = atoi(value);
    }
    else if (strcmp(key, "add_junk_loops") == 0) {
        config->add_junk_loops = atoi(value);
    }
    else if (strcmp(key, "variable_prefix") == 0) {
        strncpy(config->variable_prefix, value, 31);
        config->variable_prefix[31] = '\0';
    }
    else if (strcmp(key, "function_prefix") == 0) {
        strncpy(config->function_prefix, value, 31);
        config->function_prefix[31] = '\0';
    }
    else if (strcmp(key, "junk_var_count") == 0) {
        config->junk_var_count = atoi(value);
    }
    else if (strcmp(key, "junk_func_count") == 0) {
        config->junk_func_count = atoi(value);
    }
    else if (strcmp(key, "junk_loop_count") == 0) {
        config->junk_loop_count = atoi(value);
    }
}

Config load_config(const char *filename) { // загрузка конфига в виде структуры
    Config config = {
        .remove_comments = 1,
        .remove_whitespace = 1,
        .rename_variables = 1,
        .rename_functions = 1,
        .is_numeric = 1,
        .is_string = 1,
        .shuffle_functions = 1,
        .add_junk_variables = 0,
        .add_junk_functions = 0,
        .add_junk_loops = 0,
        .variable_prefix = "_v",
        .function_prefix = "_f",
        .junk_var_count = 5,
        .junk_func_count = 3,
        .junk_loop_count = 3
    };

    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "opening config file error, using params by default");
        return config;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        parse_config_line(line, &config);
    }

    fclose(file);
    
    return config;
}

FILE* open_and_valid(const char* filename) { // открытие файла
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "opening %s error", filename);
        abort();
    }
    return file;
}

char* file_to_buffer(FILE* file) { // файл -> буфер чаров
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "allocation memory error");
        fclose(file);
        abort();
    }
    size_t bytes_read = fread(buffer, 1, size, file);
    buffer[bytes_read] = '\0';

    fclose(file);
    
    return buffer;
}

void write_file(const char *filename, const char *content) { // буфер чаров -> файл
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "writing in %s error", filename);
        abort();
    }

    fputs(content, file);
    fclose(file);
}