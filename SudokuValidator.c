#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <omp.h>

#define N 9

int sudoku[N][N];

// Función para cargar el Sudoku desde un archivo con formato de línea única
void cargar_sudoku(const char *archivo) {
    FILE *fp = fopen(archivo, "r");
    if (!fp) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }

    char buffer[N * N + 1];  // Buffer para leer la línea completa (+1 para '\0')
    if (fscanf(fp, "%81s", buffer) != 1) {  // Lee toda la línea como un string
        perror("Error al leer el archivo");
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    // Convertir el string en la matriz sudoku[9][9]
    for (int i = 0; i < N * N; i++) {
        sudoku[i / N][i % N] = buffer[i] - '0';  // Convertir char a int
    }
}

// Validar fila
bool validar_fila(int fila) {
    bool numeros[N] = {false};
    for (int j = 0; j < N; j++) {
        int num = sudoku[fila][j] - 1;
        if (num < 0 || num >= N || numeros[num]) return false;
        numeros[num] = true;
    }
    return true;
}

// Validar columna
bool validar_columna(int columna) {
    bool numeros[N] = {false};
    for (int i = 0; i < N; i++) {
        int num = sudoku[i][columna] - 1;
        if (num < 0 || num >= N || numeros[num]) return false;
        numeros[num] = true;
    }
    return true;
}

// Validar subcuadrícula 3x3
bool validar_cuadrante(int inicioFila, int inicioColumna) {
    bool numeros[N] = {false};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int num = sudoku[inicioFila + i][inicioColumna + j] - 1;
            if (num < 0 || num >= N || numeros[num]) return false;
            numeros[num] = true;
        }
    }
    return true;
}

// Hilo para validar columnas
void* thread_validar_columnas(void* arg) {
    for (int j = 0; j < N; j++) {
        if (!validar_columna(j)) {
            pthread_exit((void*)0);
        }
    }
    pthread_exit((void*)1);
}

// Validación con OpenMP para filas y subcuadrículas
bool validar_sudoku_parallel() {
    int valido = 1;

    #pragma omp parallel for reduction(&&:valido)
    for (int i = 0; i < N; i++) {
        if (!validar_fila(i)) valido = 0;
    }

    #pragma omp parallel for collapse(2) reduction(&&:valido)
    for (int i = 0; i < N; i += 3) {
        for (int j = 0; j < N; j += 3) {
            if (!validar_cuadrante(i, j)) valido = 0;
        }
    }

    return valido;
}

// Función para imprimir el Sudoku (para depuración)
void imprimir_sudoku() {
    printf("Sudoku cargado:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%d ", sudoku[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <archivo_sudoku>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    cargar_sudoku(argv[1]);
    imprimir_sudoku();  // Verifica si el sudoku se cargó correctamente

    // Crear un proceso hijo con fork para ejecutar `ps`
    pid_t pid = fork();
    if (pid == 0) { // Proceso hijo
        execlp("ps", "ps", "-lLf", NULL);
        perror("Error al ejecutar ps");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Proceso padre
        wait(NULL);
    } else {
        perror("Error al crear el proceso hijo");
    }

    // Crear hilo para validar columnas
    pthread_t hilo;
    void* resultado;
    pthread_create(&hilo, NULL, thread_validar_columnas, NULL);
    pthread_join(hilo, &resultado);

    if (resultado == 0) {
        printf("Sudoku inválido (error en columnas).\n");
        return EXIT_FAILURE;
    }

    // Validar filas y subcuadrículas
    if (!validar_sudoku_parallel()) {
        printf("Sudoku inválido (error en filas o subcuadrículas).\n");
        return EXIT_FAILURE;
    }

    printf("Sudoku válido.\n");
    return EXIT_SUCCESS;
}

