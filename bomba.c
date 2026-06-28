#include "bomba.h"

static const int DIRECOES_ARESTAS[8][2] = {
    {-1, -1}, {-1, 0}, {-1, 1},
    { 0, -1},          { 0, 1},
    { 1, -1}, { 1, 0}, { 1, 1}
};

int vertice_valido(int MAX, Vertice vertice) {
    return vertice.linha >= 0 && vertice.linha < MAX &&
           vertice.coluna >= 0 && vertice.coluna < MAX;
}

int obter_arestas_do_vertice(int MAX, Vertice origem, Aresta arestas[8]) {
    int total_arestas = 0;

    for (int i = 0; i < 8; i++) {
        Vertice destino = {
            origem.linha + DIRECOES_ARESTAS[i][0],
            origem.coluna + DIRECOES_ARESTAS[i][1]
        };

        if (vertice_valido(MAX, destino)) {
            arestas[total_arestas].origem = origem;
            arestas[total_arestas].destino = destino;
            total_arestas++;
        }
    }

    return total_arestas;
}

void gerar_bombas(int max, casa matrix[max][max], int num_bombas) {
    int bombas_colocadas = 0;
    while (bombas_colocadas < num_bombas) {
        int i = rand() % max;
        int j = rand() % max;
        if (matrix[i][j].boolean == 0) {
            matrix[i][j].boolean = 1;
            bombas_colocadas++;
        }
    }
}

void contador_bombs(int MAX, casa matrix[MAX][MAX]) {
    for (int pi = 0; pi < MAX; pi++) {
        for (int pj = 0; pj < MAX; pj++) {
            Vertice vertice_atual = {pi, pj};
            Aresta arestas[8];
            int total_arestas = obter_arestas_do_vertice(MAX, vertice_atual, arestas);

            matrix[pi][pj].bombs = 0;
            for (int i = 0; i < total_arestas; i++) {
                Vertice vizinho = arestas[i].destino;

                if (matrix[vizinho.linha][vizinho.coluna].boolean == 1) {
                    matrix[pi][pj].bombs++;
                }
            }
        }
    }
}

void printa_ou_n(int MAX, casa matrix[MAX][MAX], int pi, int pj){
    Vertice vertice_atual = {pi, pj};
    Aresta arestas[8];
    int total_arestas = obter_arestas_do_vertice(MAX, vertice_atual, arestas);

    if (matrix[pi][pj].flag != 0) {
        return;
    }
    matrix[pi][pj].flag = 1;

    for (int i = 0; i < total_arestas; i++) {
        Vertice vizinho = arestas[i].destino;

        if (matrix[vizinho.linha][vizinho.coluna].bombs == 0) {
            printa_ou_n(MAX, matrix, vizinho.linha, vizinho.coluna);
        }
        else {
            matrix[vizinho.linha][vizinho.coluna].flag = 1;
        }
    }
}

int vitoria(int MAX, casa matrix[MAX][MAX], int num_bombas){
    int casas_abertas = 0;
    int bandeira_certa = 0;

    for (int i = 0; i < MAX; i++) {
        for (int j = 0; j < MAX; j++) {
            if (matrix[i][j].flag == 1) {
                casas_abertas++;
            }
            if (matrix[i][j].flag == -1 && matrix[i][j].boolean == 1) {
                bandeira_certa++;
            }
        }
    }

    if (casas_abertas == MAX * MAX - num_bombas || bandeira_certa == num_bombas) {
        return 1;
    }
    return 0;
}

void inicializar_fila(Fila *f) {
    f->inicio = 0;
    f->fim = 0;
}

void enfileirar(Fila *f, Vertice v) {
    f->dados[f->fim] = v;
    f->fim++;
}

Vertice desenfileirar(Fila *f) {
    Vertice v = f->dados[f->inicio];
    f->inicio++;
    return v;
}

int fila_vazia(Fila *f) {
    return f->inicio == f->fim;
}
