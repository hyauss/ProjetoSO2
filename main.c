//GABRIEL BARROS ALBERTINI -
//RAFAEL DE MENEZES ROS -
//VINICIUS ALVES MARQUES -

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAMANHO_PAGINA 4

// Estruturas para simulação
typedef struct {
    int process_id;
    int page_id;
    int address;
} MemoriaVirtual;

typedef struct {
    int process_id;
    int page_id;
    int frame_id;
    int valid;
} MemoriaFisica;

typedef struct {
    int process_id;
    int num_pages;
    MemoriaVirtual *pages;
} Processo;

typedef struct {
    int num_frames;
    MemoriaFisica *frames;
    int *fifo_queue; // Fila para FIFO
    int fifo_front;
    int fifo_rear;
    int fifo_size;
} MemoriaFisicaStruct;

// Função para inicializar a fila FIFO
void inicializar_fifo(MemoriaFisicaStruct *mem_fisica) {
    mem_fisica->fifo_queue = malloc(mem_fisica->num_frames * sizeof(int));
    mem_fisica->fifo_front = 0;
    mem_fisica->fifo_rear = -1;
    mem_fisica->fifo_size = 0;
}

// Função para adicionar um índice de frame à fila FIFO
void fifo_enqueue(MemoriaFisicaStruct *mem_fisica, int frame_index) {
    mem_fisica->fifo_rear = (mem_fisica->fifo_rear + 1) % mem_fisica->num_frames;
    mem_fisica->fifo_queue[mem_fisica->fifo_rear] = frame_index;
    mem_fisica->fifo_size++;
}

// Função para remover o índice de frame mais antigo da fila FIFO
int fifo_dequeue(MemoriaFisicaStruct *mem_fisica) {
    int frame_index = mem_fisica->fifo_queue[mem_fisica->fifo_front];
    mem_fisica->fifo_front = (mem_fisica->fifo_front + 1) % mem_fisica->num_frames;
    mem_fisica->fifo_size--;
    return frame_index;
}

// Função para ler as configurações do arquivo config.txt
void ler_configuracoes(const char *filename, int *num_frames, int *num_virtual_pages, int *num_processos, int *tam_processo, int *pagina_inicial_real, int *pagina_inicial_virtual) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo de configuração");
        exit(1);
    }
    
    fscanf(file, "Memoria_Real=%d\n", num_frames);
    fscanf(file, "Memoria_Virtual=%d\n", num_virtual_pages);
    fscanf(file, "Processos=%d\n", num_processos);
    fscanf(file, "Tam_Processo=%d\n", tam_processo);
    fscanf(file, "Pagina_Inicial_Real=%d\n", pagina_inicial_real);
    fscanf(file, "Pagina_Inicial_Virtual=%d\n", pagina_inicial_virtual);

    fclose(file);
}

// Função para exibir o estado inicial da memória virtual
void exibir_memoria_virtual(FILE *arquivo_saida, Processo *processos, int num_processos) {
    fprintf(arquivo_saida, "Estado Inicial da Memória Virtual – PID + PÁGINA + LOCALIZACAO\n");
    fprintf(arquivo_saida, "--------------- --------------- ---------------\n");
    
    for (int i = 0; i < num_processos; i++) {
        for (int j = 0; j < processos[i].num_pages; j++) {
            if (j == 0) {
                fprintf(arquivo_saida, "| P%d -%d- %d ", processos[i].process_id, j, processos[i].pages[j].address);
            } else if (j == 2) {
                fprintf(arquivo_saida, "| P%d -%d- %d |\n", processos[i].process_id, j, processos[i].pages[j].address);
            } else {
                fprintf(arquivo_saida, "| P%d -%d- %d ", processos[i].process_id, j, processos[i].pages[j].address);
            }
        }
        fprintf(arquivo_saida, "--------------- --------------- ---------------\n");
    }
    fprintf(arquivo_saida, "\n");
}

// Função para exibir o estado da memória física
void exibir_memoria_fisica(FILE *arquivo_saida, MemoriaFisicaStruct *mem_fisica, int num_frames) {
    fprintf(arquivo_saida, "Estado da Memória Física:\n");
    for (int i = 0; i < num_frames; i++) {
        if (mem_fisica->frames[i].valid) {
            fprintf(arquivo_saida, "| P%d-%d ", mem_fisica->frames[i].process_id, mem_fisica->frames[i].page_id);
        } else {
            fprintf(arquivo_saida, "| ---- ");
        }
    }
    fprintf(arquivo_saida, "|\n");
    fprintf(arquivo_saida, "\n");
}

// Função para simular falha de página e carregamento
int carregar_pagina(FILE *arquivo_saida, MemoriaFisicaStruct *mem_fisica, Processo *processo, int pagina_id, int num_frames) {
    int frame_index = -1;
    // Encontrar um frame vazio
    for (int i = 0; i < num_frames; i++) {
        if (!mem_fisica->frames[i].valid) {
            frame_index = i;
            break;
        }
    }

    // Se não houver frames vazios, substitui a página mais antiga (FIFO)
    if (frame_index == -1) {
        frame_index = fifo_dequeue(mem_fisica); // Remove o frame mais antigo
        fprintf(arquivo_saida, "Substituindo Página %d do Processo %d no Frame %d\n", 
               mem_fisica->frames[frame_index].page_id, 
               mem_fisica->frames[frame_index].process_id, 
               frame_index);
    }

    // Atualiza o frame e adiciona ao final da fila FIFO
    mem_fisica->frames[frame_index].process_id = processo->process_id;
    mem_fisica->frames[frame_index].page_id = pagina_id;
    mem_fisica->frames[frame_index].valid = 1;
    fifo_enqueue(mem_fisica, frame_index);

    return frame_index;
}

int main() {
    // Variáveis para armazenar as configurações
    int num_frames, num_virtual_pages, num_processos, tam_processo, pagina_inicial_real, pagina_inicial_virtual;

    // Abrir o arquivo de saída
    FILE *arquivo_saida = fopen("saida.txt", "w");
    if (!arquivo_saida) {
        perror("Erro ao abrir o arquivo de saída");
        return 1;
    }

    // Lendo as configurações do arquivo
    ler_configuracoes("config.txt", &num_frames, &num_virtual_pages, &num_processos, &tam_processo, &pagina_inicial_real, &pagina_inicial_virtual);

    // Inicializando os processos e a memória física
    Processo processos[num_processos];
    MemoriaFisicaStruct mem_fisica;
    mem_fisica.num_frames = num_frames;
    mem_fisica.frames = malloc(num_frames * sizeof(MemoriaFisica));

    // Inicializando a memória física
    for (int i = 0; i < num_frames; i++) {
        mem_fisica.frames[i].valid = 0;  // Memória começa vazia
    }
    inicializar_fifo(&mem_fisica);

    // Inicializando os processos e suas páginas
    for (int i = 0; i < num_processos; i++) {
        processos[i].process_id = i + 1;
        processos[i].num_pages = tam_processo;
        processos[i].pages = malloc(tam_processo * sizeof(MemoriaVirtual));
        
        int base_address = pagina_inicial_virtual + (i * 12); // Ajuste para endereços não se sobreporem
        for (int j = 0; j < tam_processo; j++) {
            processos[i].pages[j].page_id = j;
            processos[i].pages[j].address = base_address + (j * TAMANHO_PAGINA);
        }
    }

    // Exibindo o estado inicial da memória virtual
    fprintf(arquivo_saida, "EXECUÇÃO\n");
    exibir_memoria_virtual(arquivo_saida, processos, num_processos);

    // Exibindo o estado inicial da memória física
    fprintf(arquivo_saida, "Estado Inicial da Memória Real\n");
    exibir_memoria_fisica(arquivo_saida, &mem_fisica, num_frames);

    // Simulação
    fprintf(arquivo_saida, "Inicial da Execução\n");

    // Simulando as falhas de página e carregamento
    int time = 0;
    for (int i = 0; i < num_processos; i++) {
        for (int j = 0; j < processos[i].num_pages; j++) {
            if (time > 12) {
                break;  // Para a simulação quando o tempo chega a t=8
            }

            fprintf(arquivo_saida, "Tempo t=%d:\n", time);
            fprintf(arquivo_saida, "[PAGE FAULT] Página %d do Processo %d não está na memória física\n", j, i + 1);

            int frame = carregar_pagina(arquivo_saida, &mem_fisica, &processos[i], j, num_frames);
            fprintf(arquivo_saida, "Carregando Página %d do Processo %d no Frame %d\n", j, i + 1, frame);

            exibir_memoria_fisica(arquivo_saida, &mem_fisica, num_frames);
            time++;
        }
    }

    // Liberação da memória
    for (int i = 0; i < num_processos; i++) {
        free(processos[i].pages);
    }
    free(mem_fisica.frames);
    free(mem_fisica.fifo_queue);

    fclose(arquivo_saida); // Fechar o arquivo de saída

    return 0;
}
