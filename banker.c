#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

int number_of_resources;        /* quantidade de tipos de recurso (vem do argc) */
int number_of_customers = 0;    /* quantidade de clientes (linhas do customer.txt) */
int *available = NULL;          /* instancias disponiveis de cada recurso */
int **maximum = NULL;           /* maximum[cliente][recurso]: demanda maxima */
int **allocation = NULL;        /* allocation[cliente][recurso]: quanto o cliente tem agora */
int **need = NULL;              /* need[cliente][recurso]: quanto ainda pode pedir (maximum - allocation) */

/* Um comando lido do commands.txt */
typedef struct {
    char type;       /* 'Q' = RQ (pedido), 'L' = RL (liberacao), '*' = mostrar estado */
    int customer;    /* cliente do comando (nao usado no '*') */
    int *amounts;    /* quantidade de cada recurso (NULL no '*') */
} Command;

Command *commands = NULL;       /* lista de comandos, na ordem do arquivo */
int number_of_commands = 0;

/* Converte texto em inteiro >= 0. Retorna 1 se deu certo, 0 se o texto e invalido. */
int parse_int(const char *text, int *value) {
    char *end;
    long n;

    if (text == NULL || !isdigit((unsigned char) text[0])) return 0;
    errno = 0;
    n = strtol(text, &end, 10);
    if (errno != 0 || *end != '\0' || n > INT_MAX) return 0;
    *value = (int) n;
    return 1;
}

/* Le uma linha inteira do arquivo, de qualquer tamanho, sem o \n (e sem o \r do Windows).
   Retorna a linha (que deve ser liberada com free) ou NULL no fim do arquivo. */
char *read_line(FILE *file) {
    int capacity = 64, length = 0, c;
    char *line = malloc(capacity);

    if (line == NULL) return NULL;
    while ((c = fgetc(file)) != EOF && c != '\n') {
        if (length + 1 >= capacity) {
            char *bigger = realloc(line, capacity * 2);
            if (bigger == NULL) { free(line); return NULL; }
            line = bigger;
            capacity *= 2;
        }
        line[length++] = (char) c;
    }
    if (c == EOF && length == 0) { free(line); return NULL; }
    if (length > 0 && line[length - 1] == '\r') length--;
    line[length] = '\0';
    return line;
}

/* Libera toda a memoria alocada pelo programa. */
void free_all(void) {
    int i;

    for (i = 0; i < number_of_customers; i++) {
        free(maximum[i]);
        if (allocation != NULL) free(allocation[i]);
        if (need != NULL) free(need[i]);
    }
    free(maximum);
    free(allocation);
    free(need);
    for (i = 0; i < number_of_commands; i++) free(commands[i].amounts);
    free(commands);
    free(available);
}

/* Separa a linha "7,5,3" nas virgulas e guarda os numeros em values.
   Retorna a quantidade de colunas da linha, ou -1 se algum campo nao for um numero valido.
   So guarda os primeiros "limit" valores, mas conta todas as colunas. */
int parse_customer_line(char *line, int *values, int limit) {
    int count = 0, value, last = 0;
    char *field = line, *comma;

    while (!last) {
        comma = strchr(field, ',');
        if (comma != NULL) *comma = '\0';
        else last = 1;
        if (!parse_int(field, &value)) return -1;
        if (count < limit) values[count] = value;
        count++;
        field = comma + 1;
    }
    return count;
}

/* Le o customer.txt e preenche a matriz maximum.
   Retorna 1 se deu certo; se der erro, imprime a mensagem e retorna 0. */
int load_customers(void) {
    FILE *file = fopen("customer.txt", "r");
    char *line;
    int columns;

    if (file == NULL) {
        printf("Fail to read customer.txt\n");
        return 0;
    }

    while ((line = read_line(file)) != NULL) {
        int **bigger;
        int *row;

        if (line[0] == '\0') { free(line); continue; }   /* ignora linha em branco */

        row = malloc(number_of_resources * sizeof(int));
        if (row == NULL) {
            printf("Error: could not allocate memory\n");
            free(line); fclose(file);
            return 0;
        }

        columns = parse_customer_line(line, row, number_of_resources);
        free(line);

        if (columns != number_of_resources) {
            if (columns == -1) printf("Fail to read customer.txt\n");
            else printf("Incompatibility between customer.txt and command line\n");
            free(row); fclose(file);
            return 0;
        }

        bigger = realloc(maximum, (number_of_customers + 1) * sizeof(int *));
        if (bigger == NULL) {
            printf("Error: could not allocate memory\n");
            free(row); fclose(file);
            return 0;
        }
        maximum = bigger;
        maximum[number_of_customers++] = row;
    }
    fclose(file);

    if (number_of_customers == 0) {   /* arquivo vazio */
        printf("Fail to read customer.txt\n");
        return 0;
    }
    return 1;
}

/* Le o commands.txt e preenche a lista commands, sem executar nada ainda.
   Retorna 1 se deu certo; se der erro, imprime a mensagem e retorna 0. */
int load_commands(void) {
    FILE *file = fopen("commands.txt", "r");
    char *line, *token;

    if (file == NULL) {
        printf("Fail to read commands.txt\n");
        return 0;
    }

    while ((line = read_line(file)) != NULL) {
        Command command;
        Command *bigger;
        int count = 0, value;
        int error = 0;   /* 0 = ok, 1 = formato, 2 = qtd. de recursos, 3 = cliente inexistente */

        command.type = '*';
        command.customer = 0;
        command.amounts = NULL;

        token = strtok(line, " \t");
        if (token == NULL) { free(line); continue; }   /* ignora linha em branco */

        if (strcmp(token, "*") == 0) {
            if (strtok(NULL, " \t") != NULL) error = 1;   /* nada pode vir depois do * */
        } else if (strcmp(token, "RQ") == 0 || strcmp(token, "RL") == 0) {
            command.type = token[1];   /* 'Q' ou 'L' */
            command.amounts = malloc(number_of_resources * sizeof(int));
            if (command.amounts == NULL) {
                printf("Error: could not allocate memory\n");
                free(line); fclose(file);
                return 0;
            }

            token = strtok(NULL, " \t");   /* numero do cliente */
            if (!parse_int(token, &command.customer)) error = 1;

            while (!error && (token = strtok(NULL, " \t")) != NULL) {   /* recursos */
                if (!parse_int(token, &value)) error = 1;
                else {
                    if (count < number_of_resources) command.amounts[count] = value;
                    count++;
                }
            }

            if (!error && count != number_of_resources) error = 2;
            if (!error && command.customer >= number_of_customers) error = 3;
        } else {
            error = 1;   /* comando desconhecido */
        }

        if (error) {
            if (error == 1) printf("Fail to read commands.txt\n");
            else if (error == 2) printf("Incompatibility between commands.txt and command line\n");
            else printf("Error: customer %d in commands.txt does not exist\n", command.customer);
            free(command.amounts); free(line); fclose(file);
            return 0;
        }
        free(line);

        bigger = realloc(commands, (number_of_commands + 1) * sizeof(Command));
        if (bigger == NULL) {
            printf("Error: could not allocate memory\n");
            free(command.amounts); fclose(file);
            return 0;
        }
        commands = bigger;
        commands[number_of_commands++] = command;
    }
    fclose(file);
    return 1;
}

/* Cria as matrizes allocation (tudo zero) e need (igual a maximum, pois nada foi alocado).
   Retorna 1 se deu certo, 0 se faltou memoria. */
int create_state(void) {
    int i, j;

    allocation = calloc(number_of_customers, sizeof(int *));
    need = calloc(number_of_customers, sizeof(int *));
    if (allocation == NULL || need == NULL) return 0;

    for (i = 0; i < number_of_customers; i++) {
        allocation[i] = calloc(number_of_resources, sizeof(int));
        need[i] = malloc(number_of_resources * sizeof(int));
        if (allocation[i] == NULL || need[i] == NULL) return 0;
        for (j = 0; j < number_of_resources; j++) need[i][j] = maximum[i][j];
    }
    return 1;
}

/* Algoritmo de seguranca: verifica se existe uma ordem em que todos os clientes
   conseguem terminar. Retorna 1 (seguro), 0 (inseguro) ou -1 (faltou memoria). */
int is_safe(void) {
    int *work = malloc(number_of_resources * sizeof(int));
    int *finish = calloc(number_of_customers, sizeof(int));
    int i, j, progress = 1, safe = 1;

    if (work == NULL || finish == NULL) { free(work); free(finish); return -1; }

    for (j = 0; j < number_of_resources; j++) work[j] = available[j];

    while (progress) {
        progress = 0;
        for (i = 0; i < number_of_customers; i++) {
            int can_finish = 1;

            if (finish[i]) continue;
            for (j = 0; j < number_of_resources; j++)
                if (need[i][j] > work[j]) can_finish = 0;

            if (can_finish) {   /* o cliente i termina e devolve tudo o que tem */
                for (j = 0; j < number_of_resources; j++) work[j] += allocation[i][j];
                finish[i] = 1;
                progress = 1;
            }
        }
    }

    for (i = 0; i < number_of_customers; i++)
        if (!finish[i]) safe = 0;

    free(work);
    free(finish);
    return safe;
}

/* Escreve " a b c" (um espaco antes de cada numero). */
void print_values(FILE *out, const int *values) {
    int j;

    for (j = 0; j < number_of_resources; j++) fprintf(out, " %d", values[j]);
}

/* Comando RQ: tenta alocar os recursos pedidos. Retorna 0 so se faltar memoria. */
int request_resources(FILE *out, int customer, const int *request) {
    int j, safe;

    /* 1) o pedido nao pode passar do que o cliente ainda tem direito (need) */
    for (j = 0; j < number_of_resources; j++) {
        if (request[j] > need[customer][j]) {
            fprintf(out, "The customer %d request", customer);
            print_values(out, request);
            fprintf(out, " was denied because exceed its maximum allocation\n");
            return 1;
        }
    }

    /* 2) precisa existir recurso disponivel */
    for (j = 0; j < number_of_resources; j++) {
        if (request[j] > available[j]) {
            fprintf(out, "The resources");
            print_values(out, available);
            fprintf(out, " are not enough to customer %d request", customer);
            print_values(out, request);
            fprintf(out, " \n");
            return 1;
        }
    }

    /* 3) aloca "de mentira" e testa se o estado continua seguro */
    for (j = 0; j < number_of_resources; j++) {
        available[j] -= request[j];
        allocation[customer][j] += request[j];
        need[customer][j] -= request[j];
    }

    safe = is_safe();
    if (safe == 1) {
        fprintf(out, "Allocate to customer %d the resources", customer);
        print_values(out, request);
        fprintf(out, "\n");
        return 1;
    }

    /* inseguro: desfaz a alocacao */
    for (j = 0; j < number_of_resources; j++) {
        available[j] += request[j];
        allocation[customer][j] -= request[j];
        need[customer][j] += request[j];
    }
    if (safe == -1) return 0;

    fprintf(out, "The customer %d request", customer);
    print_values(out, request);
    fprintf(out, " was denied because result in an unsafe state\n");
    return 1;
}

/* Comando RL: devolve recursos do cliente para o sistema. */
void release_resources(FILE *out, int customer, const int *release) {
    int j;

    /* o cliente nao pode devolver mais do que tem */
    for (j = 0; j < number_of_resources; j++) {
        if (release[j] > allocation[customer][j]) {
            fprintf(out, "The customer %d released", customer);
            print_values(out, release);
            fprintf(out, " was denied because exceed its maximum allocation\n");
            return;
        }
    }

    for (j = 0; j < number_of_resources; j++) {
        available[j] += release[j];
        allocation[customer][j] -= release[j];
        need[customer][j] += release[j];
    }
    fprintf(out, "Release from customer %d the resources", customer);
    print_values(out, release);
    fprintf(out, "\n");
}

/* Quantos caracteres a linha "a b c " ocupa (cada numero seguido de um espaco). */
int row_width(const int *values) {
    int j, width = 0;

    for (j = 0; j < number_of_resources; j++) width += snprintf(NULL, 0, "%d ", values[j]);
    return width;
}

/* Largura de uma coluna da tabela: a maior linha da matriz ou o titulo, o que for maior. */
int column_width(int **matrix, const char *title) {
    int i, width = (int) strlen(title);

    for (i = 0; i < number_of_customers; i++)
        if (row_width(matrix[i]) > width) width = row_width(matrix[i]);
    return width;
}

/* Escreve "a b c " e completa com espacos ate a largura da coluna. */
void print_cell(FILE *out, const int *values, int width) {
    int j, written = 0;

    for (j = 0; j < number_of_resources; j++) written += fprintf(out, "%d ", values[j]);
    for (; written < width; written++) fputc(' ', out);
}

/* Comando *: escreve a tabela MAXIMUM | ALLOCATION | NEED e a linha AVAILABLE. */
void print_state(FILE *out) {
    int i, j;
    int width_max = column_width(maximum, "MAXIMUM");
    int width_alloc = column_width(allocation, "ALLOCATION");

    fprintf(out, "%-*s   |   %-*s  |   NEED\n", width_max, "MAXIMUM", width_alloc, "ALLOCATION");
    for (i = 0; i < number_of_customers; i++) {
        print_cell(out, maximum[i], width_max);
        fprintf(out, "   |   ");
        print_cell(out, allocation[i], width_alloc);
        fprintf(out, "  |   ");
        print_cell(out, need[i], 0);
        fprintf(out, "\n");
    }
    fprintf(out, "AVAILABLE ");
    for (j = 0; j < number_of_resources; j++) fprintf(out, "%d ", available[j]);
    fprintf(out, "\n");
}

/* Executa todos os comandos, na ordem, escrevendo o resultado em result.txt.
   Retorna 1 se deu certo; se der erro, imprime a mensagem e retorna 0. */
int run_commands(void) {
    FILE *out = fopen("result.txt", "w");
    int i, ok = 1;

    if (out == NULL) {
        printf("Fail to create result.txt\n");
        return 0;
    }

    for (i = 0; i < number_of_commands && ok; i++) {
        if (commands[i].type == 'Q') ok = request_resources(out, commands[i].customer, commands[i].amounts);
        else if (commands[i].type == 'L') release_resources(out, commands[i].customer, commands[i].amounts);
        else print_state(out);
    }

    fclose(out);
    if (!ok) printf("Error: could not allocate memory\n");
    return ok;
}

int main(int argc, char *argv[]) {
    int i;

    if (argc < 2) {
        printf("Error: inform the number of instances of each resource. Example: ./banker 10 5 7\n");
        return 1;
    }

    number_of_resources = argc - 1;
    available = malloc(number_of_resources * sizeof(int));
    if (available == NULL) {
        printf("Error: could not allocate memory\n");
        return 1;
    }

    for (i = 0; i < number_of_resources; i++) {
        if (!parse_int(argv[i + 1], &available[i])) {
            printf("Error: invalid argument \"%s\". Use only non-negative integers\n", argv[i + 1]);
            free_all();
            return 1;
        }
    }

    if (!load_customers() || !load_commands()) {
        free_all();
        return 1;
    }

    if (!create_state()) {
        printf("Error: could not allocate memory\n");
        free_all();
        return 1;
    }

    if (!run_commands()) {
        free_all();
        return 1;
    }

    free_all();
    return 0;
}
