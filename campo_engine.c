#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bomba.h"

void print_json(int max, void *ptr, int num_bombas, int flags_placed, int status, int last_r, int last_c)
{
    casa(*matrix)[max] = ptr; // Cast seguro para VLA baseado no tamanho atual

    printf("{\"max\": %d, \"flagsLeft\": %d, \"status\": \"%s\", \"cells\": [",
           max, num_bombas - flags_placed,
           status == 1 ? "won" : (status == -1 ? "lost" : "playing"));

    for (int i = 0; i < max; i++)
    {
        printf("[");
        for (int j = 0; j < max; j++)
        {
            int revealed = (matrix[i][j].flag == 1) ? 1 : 0;

            // Se o jogo for perdido, revela todas as bombas automaticamente
            if (status == -1 && matrix[i][j].boolean == 1)
            {
                revealed = 1;
            }

            int flagged = (matrix[i][j].flag == -1) ? 1 : 0;
            int bomb = matrix[i][j].boolean;
            int count = matrix[i][j].bombs;

            // Marca especificamente a bomba que estourou
            int exploded = (status == -1 && i == last_r && j == last_c && bomb) ? 1 : 0;

            printf("{\"revealed\": %s, \"flagged\": %s, \"bomb\": %s, \"count\": %d, \"exploded\": %s}",
                   revealed ? "true" : "false",
                   flagged ? "true" : "false",
                   bomb ? "true" : "false",
                   count,
                   exploded ? "true" : "false");

            if (j < max - 1)
                printf(",");
        }
        printf("]");
        if (i < max - 1)
            printf(",");
    }
    printf("]}\n");

    // O fflush limpa o buffer e garante que o Node.js receba a linha imediatamente
    fflush(stdout);
}

void print_hint_json(int tamanho, Vertice caminho[])
{
    printf("{\"type\": \"hint\", \"path\": [");
    for (int i = 0; i < tamanho; i++)
    {
        printf("{\"r\": %d, \"c\": %d}", caminho[i].linha, caminho[i].coluna);
        if (i < tamanho - 1)
            printf(",");
    }
    printf("]}\n");
    fflush(stdout);
}

int main()
{
    char cmd[256];
    int max = 10, num_bombas = 10;
    int flags_placed = 0;
    int status = 0; // 0 = jogando, 1 = venceu, -1 = perdeu
    int last_r = -1, last_c = -1;

    void *matriz_ptr = NULL;

    srand(time(NULL));

    // Loop infinito lendo as instruções do server.js via stdin
    while (fgets(cmd, sizeof(cmd), stdin))
    {
        if (strncmp(cmd, "QUIT", 4) == 0)
        {
            break;
        }
        else if (strncmp(cmd, "START", 5) == 0)
        {
            int level = 1;
            sscanf(cmd, "START %d", &level);

            if (level == 1)
            {
                max = 10;
                num_bombas = 10;
            }
            else if (level == 2)
            {
                max = 15;
                num_bombas = 35;
            }
            else
            {
                max = 20;
                num_bombas = 90;
            }

            if (matriz_ptr)
                free(matriz_ptr);
            matriz_ptr = malloc(max * max * sizeof(casa));
            casa(*matriz)[max] = matriz_ptr;

            for (int i = 0; i < max; i++)
            {
                for (int j = 0; j < max; j++)
                {
                    matriz[i][j].boolean = 0;
                    matriz[i][j].bombs = 0;
                    matriz[i][j].flag = 0;
                }
            }

            gerar_bombas(max, matriz, num_bombas);
            contador_bombs(max, matriz);

            flags_placed = 0;
            status = 0;
            last_r = -1;
            last_c = -1;

            print_json(max, matriz_ptr, num_bombas, flags_placed, status, last_r, last_c);
        }
        else if (strncmp(cmd, "OPEN", 4) == 0)
        {
            int r, c;
            sscanf(cmd, "OPEN %d %d", &r, &c);
            casa(*matriz)[max] = matriz_ptr;

            if (status == 0 && r >= 0 && r < max && c >= 0 && c < max)
            {
                if (matriz[r][c].flag == 0)
                {
                    if (matriz[r][c].boolean == 1)
                    {
                        status = -1; // Achou bomba = Game Over
                        last_r = r;
                        last_c = c;
                    }
                    else
                    {
                        if (matriz[r][c].bombs == 0)
                        {
                            printa_ou_n(max, matriz, r, c); // Usa sua função recursiva
                        }
                        else
                        {
                            matriz[r][c].flag = 1;
                        }
                        if (vitoria(max, matriz, num_bombas))
                        { // Usa sua função de vitória
                            status = 1;
                        }
                    }
                }
            }
            print_json(max, matriz_ptr, num_bombas, flags_placed, status, last_r, last_c);
        }
        else if (strncmp(cmd, "FLAG", 4) == 0)
        {
            int r, c;
            sscanf(cmd, "FLAG %d %d", &r, &c);
            casa(*matriz)[max] = matriz_ptr;

            if (status == 0 && r >= 0 && r < max && c >= 0 && c < max)
            {
                if (matriz[r][c].flag == -1)
                {
                    matriz[r][c].flag = 0; // Remove bandeira
                    flags_placed--;
                }
                else if (matriz[r][c].flag == 0)
                {
                    matriz[r][c].flag = -1; // Coloca bandeira
                    flags_placed++;
                }

                if (vitoria(max, matriz, num_bombas))
                {
                    status = 1;
                }
            }
            print_json(max, matriz_ptr, num_bombas, flags_placed, status, last_r, last_c);
        }
        else if (strncmp(cmd, "HINT", 4) == 0)
        {
            int r, c;
            sscanf(cmd, "HINT %d %d", &r, &c);

            if (status == 0 && matriz_ptr != NULL && r >= 0 && r < max && c >= 0 && c < max)
            {
                casa(*matriz)[max] = matriz_ptr;
                Vertice inicio = {r, c};
                Vertice caminho[400];
                int tamanho = buscar_caminho_bomba_bfs(max, matriz, inicio, caminho);
                print_hint_json(tamanho, caminho);
            }
            else
            {
                printf("{\"error\": \"Movimento invalido ou jogo encerrado\"}\n");
                fflush(stdout);
            }
        }
        else if (strncmp(cmd, "STATE", 5) == 0)
        {
            if (matriz_ptr)
            {
                print_json(max, matriz_ptr, num_bombas, flags_placed, status, last_r, last_c);
            }
            else
            {
                printf("{\"error\": \"Jogo nao inicializado\"}\n");
                fflush(stdout);
            }
        }
    }

    if (matriz_ptr)
        free(matriz_ptr);
    return 0;
}