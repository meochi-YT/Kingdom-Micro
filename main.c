#include "bitmaps.h"
#include "Nokia5110.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "driverlib/gpio.h"
#include <stdbool.h>

#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"

/*
    DADOS IMPORTANTES:
    Tamanho: 48x84(A x L)
    Bitmap gerado por: https://sparks.gogo.co.nz/pcd8554-bmp.html

    PA4 = Botao extra
    PF0 = SW1
    PF4 = SW2
    LED: R = PF1 / G = PF2 / B = PF3
*/

#define WRITE_REG(x)          (*((volatile uint32_t *)(x)))
#define GPIO_O_GPIOPUR              0x510

void configura(){
    //SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    //habilita as portas do tipo F e depois as do A
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //destrava o PF0
    WRITE_REG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    WRITE_REG(GPIO_PORTF_BASE + GPIO_O_CR) = 0x01;

    //configura PF0 E PF4 como entrada
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE , GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE , GPIO_PIN_0| GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE , GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTA_BASE , GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

    //configura saida dos leds
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

    WRITE_REG(GPIO_PORTF_BASE + GPIO_O_GPIOPUR)  |= GPIO_PIN_4;
}

//funcao de plotar linhas
void plotLineLow(int x0, int y0, int x1, int y1){
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;

    if(dy < 0){
        yi = -1;
        dy = -dy;
    }
    int D = 2*dy - dx;
    int y = y0;
    int x;
    for(x = x0; x<x1; x++){
        Nokia5110_SetPxl(x,y);
        if(D > 0){
            y = y + yi;
            D = D - 2*dx;
        }
        D = D + 2*dy;
    }
}

void plotLineHigh(int x0, int y0, int x1, int y1){
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    if(dx < 0){
        xi = -1;
        dx = -dx;
    }
    int D = 2*dx - dy;
    int x = x0;
    int y;
    for(y = y0; y<y1; y++){
        Nokia5110_SetPxl(x,y);
        if(D > 0){
            x = x + xi;
            D = D - 2*dy;
        }
        D = D + 2*dx;
    }
}

void plotLine(int y0, int x0, int y1, int x1){
      if(abs(y1 - y0) < abs(x1 - x0)){
        if(x0 > x1){
          plotLineLow(x1, y1, x0, y0);
        }else{
          plotLineLow(x0, y0, x1, y1);
        }
      }else{
        if(y0 > y1){
          plotLineHigh(x1, y1, x0, y0);
        }else{
          plotLineHigh(x0, y0, x1, y1);
        }
      }
}

//le os botoes e retorna o estado de cada um
int le_botao(int bot){
    if(bot > 3){ return -1; }

    if(bot == 1){
        if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0) == 0x00){ return 1; }
    }else if(bot == 2){
        if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4) == 0X00){ return 1; }
    }else if(bot == 3){
        if(GPIOPinRead(GPIO_PORTA_BASE,GPIO_PIN_4) == GPIO_PIN_4){ return 1; }
    }else{
        return -1;
    }

    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------------------

// --------------------- define as variaveis globais que vao ser usadas

const int seg = 3; //quanto vale um segundo
const int TELA_X = 84; //tamanho tela x
const int TELA_Y = 48; //tamanho tela y
const int POS_CHAO = 32; //posicao onde os objetos serao printados em cima
const int VEL_ROLAGEM = 2; //pixels que uma atualizacao pula na rolagem da tela

bool skiped;

bool dia;
bool noite;
int contador_dia;

int vida_rei;

bool possibilidade_bau;

//struct dos npcs que tera no jogo
typedef struct {
    int moedas; //total de moedas acumulado pelo npc
    int posicao_x; //posicao x ATUAL do npc
    int origem_x; //posicao x onde o npc deve andar em volta
    int ocupado; //diz se o trabalhador ta ocupado ou nao
} Pessoa;

//vetores de pessoas
Pessoa vetor_arqueira[25];
Pessoa vetor_construtor[25];
Pessoa vetor_nativo[25];
Pessoa vetor_aldeao[25];

typedef struct {
    int posicao_x;
    int origem_x;
    int direcao;
    int status;
} Animal;

Animal vetor_coelho[25]; //vetor com os coelhos que as arqueiras vao matar

int monstro_spawn1;
int monstro_spawn2;
int contador_monstros;

Animal vetor_monstro[25]; //vetor com os monstros

int vetor_moedas[50]; //vetor com a posição X de cada moeda existente no mapa

typedef struct {
    int posicao_x;
    int mendigos;
} Acampamento;

Acampamento vetor_acampamentos[5]; //vetor dos acampamentos com os mendigos

//struct dos npcs que tera no jogo
typedef struct {
    int vida; //total de vida da barreira
    int posicao_x; //posicao x da barricada no mapa
    int nivel; //nivel da barricada 0, 1, 2
    int construindo;
} Barricada;

Barricada vetor_barricada[25]; //vetor com as barricadas

int contador_geral; //timer geral que é um DOUBLE INT PRA SER BEEEEM GRANDE

bool fogueira_acesa; //controla se a fogueira esta acesa e a cidade construida

int posicao_loja_martelo; //posicao fixa da loja de martelo no MAPA esta variavel eh estatica
int posicao_loja_arco; //posicao fixa da loja de arco no MAPA esta variavel eh estatica
int posicao_fogueira; //posicao fixa da fogueira central no MAPA esta variavel eh estatica
int posicao_bau; //posicao fixa do bau no MAPA esta variavel eh estatica

int arcos_venda;
int martelos_venda;

int posicao_fixa_cavalinho; //inicia posicao do cavalo(inicio da img)
int posicao_cavalo; //posicao do cavalo no mapa
int direcao_anterior_cavalinho;
int direcao_cavalinho; //inicia a direcao do cavalo pra direita(dir>=0)

//nuvens
int posicao_x_nuvem1_1;
int posicao_x_nuvem2_1;
int posicao_x_nuvem3_1;
int posicao_x_nuvem1_2;
int posicao_x_nuvem2_2;

//estado do bau: 0-fechado 1-aberto
int estado_bau = 0;

//chao
int posicao_chao1;
int posicao_chao2;
int posicao_chao3;

//placar
int total_moedas;

void desloca_x(int rol){ //na verdade ele nao muda tanto a posicao DO CAVALINHO, ele deve mudar a posicao do cenario
    //no caso uma rolagem positiva vira o cavalo pra esquerda(-direcao_cavalinho) e rola tudo pra direita(+rol)

    if(direcao_cavalinho != 0){//direcoes do cavalinho
        direcao_anterior_cavalinho = direcao_cavalinho;
    }
    direcao_cavalinho = -rol;

    //posicao do cavalinho no mapa virtual
    posicao_cavalo += -rol;

    posicao_x_nuvem1_1 += (rol/2);
    if(posicao_x_nuvem1_1 > TELA_X){posicao_x_nuvem1_1 = 0;}
    if(posicao_x_nuvem1_1 < 0){posicao_x_nuvem1_1 = TELA_X;} //usa o tamanho da maior nuvem

    posicao_x_nuvem2_1 += (rol/2);
    if(posicao_x_nuvem2_1 > TELA_X){posicao_x_nuvem2_1 = 0;}
    if(posicao_x_nuvem2_1 < 0){posicao_x_nuvem2_1 = TELA_X;}

    posicao_x_nuvem3_1 += (rol/2);
    if(posicao_x_nuvem3_1 > TELA_X){posicao_x_nuvem3_1 = 0;}
    if(posicao_x_nuvem3_1 < 0){posicao_x_nuvem3_1 = TELA_X;}

    posicao_x_nuvem1_2 += (rol/2);
    if(posicao_x_nuvem1_2 > TELA_X){posicao_x_nuvem1_2 = 0;}
    if(posicao_x_nuvem1_2 < 0){posicao_x_nuvem1_2 = TELA_X;} //usa o tamanho da maior nuvem

    posicao_x_nuvem2_2 += (rol/2);
    if(posicao_x_nuvem2_2 > TELA_X){posicao_x_nuvem2_2 = 0;}
    if(posicao_x_nuvem2_2 < 0){posicao_x_nuvem2_2 = TELA_X;}

    posicao_chao1 += rol;
    if(posicao_chao1 > TELA_X){posicao_chao1 = 0;}
    if(posicao_chao1 < 0){posicao_chao1 = TELA_X;}

    posicao_chao2 += rol;
    if(posicao_chao2 > TELA_X){posicao_chao2 = 0;}
    if(posicao_chao2 < 0){posicao_chao2 = TELA_X;}

    posicao_chao3 += rol;
    if(posicao_chao3 > TELA_X){posicao_chao3 = 0;}
    if(posicao_chao3 < 0){posicao_chao3 = TELA_X;}

}

void inicia_posicoes(){
    int i;
    int tamanho;

    contador_geral = 0;

    dia = true;
    noite = false;
    possibilidade_bau = false;

    vida_rei = 5;

    contador_dia = 1; //ja começa no dia um obviamente

    fogueira_acesa = false;

    posicao_fixa_cavalinho = (TELA_X/2) - (TAM_X_CAVALINHO/2); //posicao fixa do cavalo na tela, esta variavel eh estatica
    posicao_cavalo = (TELA_X/2) - (TAM_X_CAVALINHO/2) - 70; //posicao do cavalo no mapa
    direcao_anterior_cavalinho = 0;
    direcao_cavalinho = 0; //inicia a direcao do cavalo pra direita(dir>=0)

    posicao_fogueira = (TELA_X/2) - (TAM_X_FOGUEIRA/2) + (TELA_X/2) + 10; //posicao fixa da fogueira central no MAPA esta variavel eh estatica
    posicao_loja_martelo = posicao_fogueira + 30; //posicao fixa da loja de martelo no MAPA esta variavel eh estatica
    posicao_loja_arco = posicao_fogueira - 30; //posicao fixa da loja de arco no MAPA esta variavel eh estatica
    posicao_bau = posicao_fogueira + TAM_X_FOGUEIRA + 5; //posicao fixa do bau no MAPA esta variavel eh estatica

    arcos_venda = 0;
    martelos_venda = 0;

    posicao_x_nuvem1_1 = 4; //posicao inicial das nuvens
    posicao_x_nuvem2_1 = 21;
    posicao_x_nuvem3_1 = 65;
    posicao_x_nuvem1_2 = 30;
    posicao_x_nuvem2_2 = 50;

    estado_bau = 0; //fechado

    posicao_chao1 = 17; //posicao inicial das pedras no chao
    posicao_chao2 = 3;
    posicao_chao3 = 50;

    total_moedas = 40; //placar inicial ------------------------------------------------------------------ TOTAAALL MOEDAS !!!!!!

    //AVISO: -20000 eh o valor zero, ou seja, que nao ira aparecer na tela
    tamanho = 5;
    for(i=0; i<tamanho; i++){
        vetor_acampamentos[i].posicao_x = -20000;
        vetor_acampamentos[i].mendigos = 0;
    }

    vetor_acampamentos[0].posicao_x = posicao_loja_martelo + TAM_X_LOJA_MARTELO - TAM_X_ACAMPAMENTO + 275;
    vetor_acampamentos[1].posicao_x = posicao_loja_arco - 275;
    //vetor_acampamentos[2] = 100;
    //vetor_acampamentos[3] = 100;
    //vetor_acampamentos[4] = 100;

    monstro_spawn1 = vetor_acampamentos[1].posicao_x - 100;
    monstro_spawn2 = vetor_acampamentos[0].posicao_x + 100;

    contador_monstros = 0;

    //zera vetor da posicao das moedas
    tamanho = 50;
    for(i=0; i<tamanho; i++){
        vetor_moedas[i] = -20000;
    }

    tamanho = 25;
    for(i=0; i<tamanho; i++){
        vetor_barricada[i].posicao_x = -20000;
        vetor_barricada[i].vida = 0; //vida = 0 quer dizer que é uma posicao sem barricada construida
        vetor_barricada[i].nivel = 0;
        vetor_barricada[i].construindo = -20000;
    }

    //até 25 barricadas

    vetor_barricada[0].posicao_x = posicao_loja_arco - 35;
    vetor_barricada[1].posicao_x = posicao_loja_martelo + TAM_X_LOJA_MARTELO - TAM_X_BARRICADA + 40;
    vetor_barricada[2].posicao_x = vetor_barricada[0].posicao_x - 45;
    vetor_barricada[3].posicao_x = vetor_barricada[1].posicao_x + 45;
    vetor_barricada[4].posicao_x = vetor_barricada[2].posicao_x - 70;
    vetor_barricada[5].posicao_x = vetor_barricada[3].posicao_x + 70;
    vetor_barricada[6].posicao_x = vetor_barricada[4].posicao_x - 30;
    vetor_barricada[7].posicao_x = vetor_barricada[5].posicao_x + 30;
    vetor_barricada[8].posicao_x = vetor_barricada[6].posicao_x - 50;
    vetor_barricada[9].posicao_x = vetor_barricada[7].posicao_x + 50;

    //moedas iniciais do jogo
    vetor_moedas[0] = -10;
    vetor_moedas[1] = -6;
    vetor_moedas[2] = -3;
    vetor_moedas[3] = 0;
    vetor_moedas[4] = 5;
    vetor_moedas[5] = 8;
    vetor_moedas[6] = 13;
    vetor_moedas[7] = 17;
    vetor_moedas[8] = 20;
    vetor_moedas[9] = 25;

    tamanho = 25;
    for(i=0; i<tamanho; i++){
        vetor_nativo[i].posicao_x = -20000;
        vetor_nativo[i].moedas = 0;
        vetor_nativo[i].origem_x = 0;
        vetor_nativo[i].ocupado = 0;
    }

    vetor_nativo[0].posicao_x = posicao_loja_arco - TAM_X_NATIVO - 50;
    vetor_nativo[0].origem_x = vetor_nativo[0].posicao_x;

    vetor_nativo[1].posicao_x = posicao_loja_arco - TAM_X_NATIVO - 10;
    vetor_nativo[1].origem_x = vetor_nativo[1].posicao_x;

    vetor_nativo[2].posicao_x = posicao_loja_martelo + TAM_X_LOJA_MARTELO + TAM_X_NATIVO + 10;
    vetor_nativo[2].origem_x = vetor_nativo[2].posicao_x;

    vetor_nativo[3].posicao_x = posicao_loja_martelo + TAM_X_LOJA_MARTELO + TAM_X_NATIVO + 50;
    vetor_nativo[3].origem_x = vetor_nativo[3].posicao_x;

    tamanho = 25;
    for(i=0; i<tamanho; i++){
        vetor_aldeao[i].posicao_x = -20000;
        vetor_aldeao[i].moedas = 0;
        vetor_aldeao[i].origem_x = posicao_fogueira;
        vetor_aldeao[i].ocupado = 0;
    }

    //vetor que guarda a posicao atual de cada arqueira presente no jogo junto com a quantidade de moedas
    tamanho = 25;
    for(i=0; i<tamanho; i++){
        vetor_arqueira[i].posicao_x = -20000;
        vetor_arqueira[i].moedas = 0;
        vetor_arqueira[i].origem_x = 0;
        vetor_arqueira[i].ocupado = 0;
    }

    //vetor que guarda a posicao atual de cada construtor presente no jogo
    tamanho = 25;
    for(i=0; i<tamanho; i++){
        vetor_construtor[i].posicao_x = -20000;
        vetor_construtor[i].moedas = 0;
        vetor_construtor[i].origem_x = 0;
        vetor_construtor[i].ocupado = 0;
    }

    tamanho = 25;
    for(i=0; i<tamanho; i++){
        vetor_coelho[i].posicao_x = -20000;
        vetor_coelho[i].origem_x = -20000;
        vetor_coelho[i].direcao = 0;
        vetor_coelho[i].status = 0;
    }

    tamanho = 25;
    for(i=0; i<tamanho; i++){
        vetor_monstro[i].posicao_x = -20000;
        vetor_monstro[i].origem_x = -20000;
        vetor_monstro[i].direcao = 0;
        vetor_monstro[i].status = 0;
    }

}//fim da funcao inicia posicoes

void move_npcs(){
    int i;
    //nativos
    for(i=0; i<25; i++){
       if(vetor_nativo[i].posicao_x != -20000){ //se npc existe
           vetor_nativo[i].posicao_x += (rand()%3)-1; //anda randomicamente entre -1 e +1
           if(vetor_nativo[i].posicao_x > vetor_nativo[i].origem_x + 5){ //se passou da origem vai um pra esquerda
               vetor_nativo[i].posicao_x += -1;
           }
           if(vetor_nativo[i].posicao_x < vetor_nativo[i].origem_x - 5){ //se passou da origem vai um pra direita
              vetor_nativo[i].posicao_x += 1;
          }
       }
    }

    //aldeoes
    for(i=0; i<25; i++){
       if(vetor_aldeao[i].posicao_x != -20000){ //se npc existe
           vetor_aldeao[i].posicao_x += (rand()%3)-1; //anda randomicamente entre -1 e +1
           if(vetor_aldeao[i].posicao_x > vetor_aldeao[i].origem_x + 5){ //se passou da origem vai um pra esquerda
               vetor_aldeao[i].posicao_x += -1;
           }
           if(vetor_aldeao[i].posicao_x < vetor_aldeao[i].origem_x - 5){ //se passou da origem vai um pra direita
              vetor_aldeao[i].posicao_x += 1;
          }
       }
    }

    //construtor - o construtor se move diferente, ele fica parado no centro do mapa esperando mandar construir algo...
    for(i=0; i<25; i++){
       if(vetor_construtor[i].posicao_x != -20000){ //se npc existe
           vetor_construtor[i].posicao_x += (rand()%3)-1; //anda randomicamente entre -1 e +1
           if(vetor_construtor[i].posicao_x > vetor_construtor[i].origem_x + 5){ //se passou da origem vai um pra esquerda
               vetor_construtor[i].posicao_x += -1;
           }
           if(vetor_construtor[i].posicao_x < vetor_construtor[i].origem_x - 5){ //se passou da origem vai um pra direita
               vetor_construtor[i].posicao_x += 1;
          }
       }
    }

    //arqueira - a arqueira se move de forma a se aproximar dos coelhos, pra matar eles e conseguir moedas
    for(i=0; i<25; i++){
       if(vetor_arqueira[i].posicao_x != -20000){ //se npc existe
           vetor_arqueira[i].posicao_x += (rand()%3)-1; //anda randomicamente entre -1 e +1
           if(vetor_arqueira[i].posicao_x > vetor_arqueira[i].origem_x + 10){ //se passou da origem vai um pra esquerda
               vetor_arqueira[i].posicao_x += -1;
           }
           if(vetor_arqueira[i].posicao_x < vetor_arqueira[i].origem_x - 10){ //se passou da origem vai um pra direita
               vetor_arqueira[i].posicao_x += 1;
          }
       }
    }

    //coelho
    for(i=0; i<25; i++){
       if(vetor_coelho[i].posicao_x != -20000){ //se npc existe
           int direcao = (rand()%3)-1;
           vetor_coelho[i].direcao = direcao;
           vetor_coelho[i].posicao_x += direcao; //anda randomicamente entre -1 e +1
           if(vetor_coelho[i].posicao_x > vetor_coelho[i].origem_x + 5){ //se passou da origem vai um pra esquerda
               vetor_coelho[i].posicao_x += -1;
           }
           if(vetor_coelho[i].posicao_x < vetor_coelho[i].origem_x - 5){ //se passou da origem vai um pra direita
               vetor_coelho[i].posicao_x += 1;
           }
       }
    }

    //monstros
    for(i=0; i<25; i++){
       if(vetor_monstro[i].posicao_x != -20000){ //se npc existe
           int direcao = (rand()%3)-1;
           vetor_monstro[i].direcao = direcao*2;
           vetor_monstro[i].posicao_x += direcao*2;

           if(vetor_monstro[i].posicao_x > vetor_monstro[i].origem_x + 5){ //se passou da origem vai um pra esquerda
               vetor_monstro[i].posicao_x += -1 ;
               vetor_monstro[i].direcao = -1;
           }
           if(vetor_monstro[i].posicao_x < vetor_monstro[i].origem_x - 5){ //se passou da origem vai um pra direita
               vetor_monstro[i].posicao_x += 1 ;
               vetor_monstro[i].direcao = 1;
           }
       }
    }
}

void renderiza_barricadas(){
    int i;
    if(fogueira_acesa == 1){
        for(i=0; i<25;i++){
            if(vetor_barricada[i].posicao_x != -20000){//se existe a barricada
                if(vetor_barricada[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_barricada[i].posicao_x-posicao_cavalo+posicao_fixa_cavalinho < TELA_X - TAM_X_BARRICADA){
                    if(vetor_barricada[i].vida == 0){//se estiver no nivel 0 - ou seja quebrada
                        Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_BAU+2, bau[0], TAM_X_BAU, TAM_Y_BAU);
                    }else{
                        Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_BARRICADA, barricada[vetor_barricada[i].nivel], TAM_X_BARRICADA, TAM_Y_BARRICADA);
                    }
                }
            }
        }
    }
}

void renderiza_npcs(){
    int i;
    //coelho
    for(i=0; i<25; i++){
        if(vetor_coelho[i].posicao_x != -20000){
            if(vetor_coelho[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_coelho[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_COELHO){
                if(vetor_coelho[i].direcao >= 0){ //verifica direcao - se for direita
                    Nokia5110_PrintBMP2( vetor_coelho[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_COELHO, coelho_dir[(contador_geral/3)%2], TAM_X_COELHO, TAM_Y_COELHO);
                }else if(vetor_coelho[i].direcao < 0){
                    Nokia5110_PrintBMP2( vetor_coelho[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_COELHO, coelho_esq[(contador_geral/3)%2], TAM_X_COELHO, TAM_Y_COELHO);
                }
            }
        }
    }
    //nativos
    for(i=0; i<25; i++){
       if(vetor_nativo[i].posicao_x != -20000){
           if(vetor_nativo[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_nativo[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_NATIVO){
               Nokia5110_PrintBMP2(vetor_nativo[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_NATIVO, nativo[(contador_geral/4)%2], TAM_X_NATIVO, TAM_Y_NATIVO);
           }
       }
    }
    //aldeao
    for(i=0; i<25; i++){
       if(vetor_aldeao[i].posicao_x != -20000){
           if(vetor_aldeao[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_aldeao[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_NATIVO){
               Nokia5110_PrintBMP2(vetor_aldeao[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_NATIVO, nativo[(contador_geral/2)%2], TAM_X_NATIVO, TAM_Y_NATIVO);
           }
       }
    }
    //construtor
    for(i=0; i<25; i++){
       if(vetor_construtor[i].posicao_x != -20000){
           if(vetor_construtor[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_construtor[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_CONSTRUTOR){
               Nokia5110_PrintBMP2(vetor_construtor[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_CONSTRUTOR, construtor_esq[(contador_geral/3)%2], TAM_X_CONSTRUTOR, TAM_Y_CONSTRUTOR);
           }
       }
    }
    //arqueira
    for(i=0; i<25; i++){
       if(vetor_arqueira[i].posicao_x != -20000){
           if(vetor_arqueira[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_arqueira[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_ARQUEIRA){
               Nokia5110_PrintBMP2(vetor_arqueira[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_ARQUEIRA, arqueira_esq[(contador_geral/3)%2], TAM_X_ARQUEIRA, TAM_Y_ARQUEIRA);
           }
       }
    }

    //MONSTROS
    for(i=0; i<25; i++){
        if(vetor_monstro[i].posicao_x != -20000){
            if(vetor_monstro[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_monstro[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_MONSTRO){
                if(vetor_monstro[i].direcao >= 0){ //verifica direcao - se for direita
                    Nokia5110_PrintBMP2( vetor_monstro[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_MONSTRO, monstro_dir[(contador_geral/2)%2], TAM_X_MONSTRO, TAM_Y_MONSTRO);
                }else if(vetor_monstro[i].direcao < 0){
                    Nokia5110_PrintBMP2( vetor_monstro[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_MONSTRO, monstro_esq[(contador_geral/2)%2], TAM_X_MONSTRO, TAM_Y_MONSTRO);
                }
            }
        }
    }
}

void renderiza_centro(){
    if(fogueira_acesa == 1){
        //desenha o bau fechado por cima do cavalo como essa funcao vem depois
        if(posicao_bau-posicao_cavalo + posicao_fixa_cavalinho >= 0 && posicao_bau-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_BAU){
            Nokia5110_PrintBMP2(posicao_bau-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_BAU, bau[estado_bau], TAM_X_BAU, TAM_Y_BAU);
        }
    }else{//desenha fogueira apagada por cima do cavalo ja que essa funcao vem depois
        if(posicao_fogueira-posicao_cavalo + posicao_fixa_cavalinho >= 0 && posicao_fogueira-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_BAU){
            Nokia5110_PrintBMP2(posicao_fogueira-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_BAU+2, bau[0], TAM_X_BAU, TAM_Y_BAU);
        }
    }
}

void renderiza_lojas(){
    if(fogueira_acesa == 1){
        //desenha fogueira acesa
        if(posicao_fogueira-posicao_cavalo + posicao_fixa_cavalinho >= 0 && posicao_fogueira-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_FOGUEIRA){
            Nokia5110_PrintBMP2(posicao_fogueira-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_FOGUEIRA, fogueira[(contador_geral/3)%2], TAM_X_FOGUEIRA, TAM_Y_FOGUEIRA);
        }
        if(posicao_loja_martelo-posicao_cavalo + posicao_fixa_cavalinho >= 0 && posicao_loja_martelo-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_LOJA_MARTELO){
            Nokia5110_PrintBMP2(posicao_loja_martelo-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_LOJA_MARTELO, loja_martelo[(contador_geral/4)%2], TAM_X_LOJA_MARTELO, TAM_Y_LOJA_MARTELO);
        }
        if(posicao_loja_arco-posicao_cavalo + posicao_fixa_cavalinho >= 0 && posicao_loja_arco-posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_LOJA_ARCO){
            Nokia5110_PrintBMP2(posicao_loja_arco-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_LOJA_ARCO, loja_arco[(contador_geral/6)%2], TAM_X_LOJA_ARCO, TAM_Y_LOJA_ARCO);
        }

        //renderiza acampamento
        int i;
        for(i=0;i<5;i++){
            if(vetor_acampamentos[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_acampamentos[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho < TELA_X - TAM_X_ACAMPAMENTO){
                Nokia5110_PrintBMP2(vetor_acampamentos[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO - TAM_Y_ACAMPAMENTO, acampamento[(contador_geral/2)%2], TAM_X_ACAMPAMENTO, TAM_Y_ACAMPAMENTO);
            }
        }
    }
}

void renderiza_moedas_chao(){

   int i=0;
   for(i=0; i<50; i++){
       if(vetor_moedas[i] != -20000){
           if(vetor_moedas[i]-posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_moedas[i]-posicao_cavalo + posicao_fixa_cavalinho < TELA_X-TAM_X_MOEDA_PEQUENA){
               Nokia5110_PrintBMP2(vetor_moedas[i]-posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO-TAM_Y_MOEDA_PEQUENA-1, moeda_pequena, TAM_X_MOEDA_PEQUENA, TAM_Y_MOEDA_PEQUENA);
           }
       }
   }
}//fim da funcao renderiza_moedas_chao

void renderiza_vida_objetos(){
    //numero de arcos
    if(fogueira_acesa == 1){
        if(posicao_loja_arco-posicao_cavalo + posicao_fixa_cavalinho >= 0 && posicao_loja_arco-posicao_cavalo + posicao_fixa_cavalinho < TELA_X-TAM_X_LOJA_ARCO){
            if(arcos_venda == 0){ Nokia5110_PrintBMP2(posicao_loja_arco-posicao_cavalo + posicao_fixa_cavalinho + 10, POS_CHAO - 15, digito_0, TAM_X_DIGITO, TAM_Y_DIGITO); }
            if(arcos_venda == 1){ Nokia5110_PrintBMP2(posicao_loja_arco-posicao_cavalo + posicao_fixa_cavalinho + 10, POS_CHAO - 15, digito_1, TAM_X_DIGITO, TAM_Y_DIGITO); }
            if(arcos_venda == 2){ Nokia5110_PrintBMP2(posicao_loja_arco-posicao_cavalo + posicao_fixa_cavalinho + 10, POS_CHAO - 15, digito_2, TAM_X_DIGITO, TAM_Y_DIGITO); }
            if(arcos_venda == 3){ Nokia5110_PrintBMP2(posicao_loja_arco-posicao_cavalo + posicao_fixa_cavalinho + 10, POS_CHAO - 15, digito_3, TAM_X_DIGITO, TAM_Y_DIGITO); }
        }

        //numero de martelos
        if(posicao_loja_martelo-posicao_cavalo + posicao_fixa_cavalinho >= 0 && posicao_loja_martelo-posicao_cavalo + posicao_fixa_cavalinho < TELA_X-TAM_X_LOJA_MARTELO){
            if(martelos_venda == 0){ Nokia5110_PrintBMP2(posicao_loja_martelo-posicao_cavalo + posicao_fixa_cavalinho + 5, POS_CHAO - 15, digito_0, TAM_X_DIGITO, TAM_Y_DIGITO); }
            if(martelos_venda == 1){ Nokia5110_PrintBMP2(posicao_loja_martelo-posicao_cavalo + posicao_fixa_cavalinho + 5, POS_CHAO - 15, digito_1, TAM_X_DIGITO, TAM_Y_DIGITO); }
            if(martelos_venda == 2){ Nokia5110_PrintBMP2(posicao_loja_martelo-posicao_cavalo + posicao_fixa_cavalinho + 5, POS_CHAO - 15, digito_2, TAM_X_DIGITO, TAM_Y_DIGITO); }
            if(martelos_venda == 3){ Nokia5110_PrintBMP2(posicao_loja_martelo-posicao_cavalo + posicao_fixa_cavalinho + 5, POS_CHAO - 15, digito_3, TAM_X_DIGITO, TAM_Y_DIGITO); }
        }
    }

    //vida barricada i
    int i;
    if(fogueira_acesa == 1){
        for(i=0; i<25;i++){
            if(vetor_barricada[i].posicao_x != -20000){//se existe a barricada e nao estah em construcao
                if(vetor_barricada[i].posicao_x-posicao_cavalo + posicao_fixa_cavalinho >= 0 && vetor_barricada[i].posicao_x-posicao_cavalo+posicao_fixa_cavalinho < TELA_X - TAM_X_BARRICADA){
                    if(vetor_barricada[i].vida == 0){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_0, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 1){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_1, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 2){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_2, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 3){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_3, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 4){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_4, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 5){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_5, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 6){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_6, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 7){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_7, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 8){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_8, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 9){ Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_9, TAM_X_DIGITO, TAM_Y_DIGITO); }
                    if(vetor_barricada[i].vida == 10){
                        Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho , POS_CHAO - TAM_Y_BARRICADA - 6, digito_1, TAM_X_DIGITO, TAM_Y_DIGITO);
                        Nokia5110_PrintBMP2(vetor_barricada[i].posicao_x - posicao_cavalo + posicao_fixa_cavalinho + TAM_X_DIGITO+1, POS_CHAO - TAM_Y_BARRICADA - 6, digito_0, TAM_X_DIGITO, TAM_Y_DIGITO);
                    }
                }
            }
        }
    }

}//fim da funcao renderiza vida dos objetos e quyantidade de itens

void renderiza_placar(){ //printa numero de moedas na tela

    Nokia5110_PrintBMP2(TELA_X-3-1-1-3-1-TAM_X_MOEDA_GRANDE, 2, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);

    if( (total_moedas/10) == 0){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_0, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas/10) == 1){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_1, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas/10) == 2){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_2, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas/10) == 3){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_3, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas/10) == 4){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_4, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas/10) == 5){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_5, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas/10) == 6){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_6, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas/10) == 7){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_7, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas/10) == 8){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_8, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas/10) == 9){ Nokia5110_PrintBMP2(TELA_X-3-1-1-3, 2, digito_9, TAM_X_DIGITO, TAM_Y_DIGITO); }

    if( (total_moedas%10) == 0){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_0, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas%10) == 1){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_1, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas%10) == 2){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_2, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas%10) == 3){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_3, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas%10) == 4){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_4, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas%10) == 5){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_5, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas%10) == 6){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_6, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas%10) == 7){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_7, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas%10) == 8){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_8, TAM_X_DIGITO, TAM_Y_DIGITO); }
    if( (total_moedas%10) == 9){ Nokia5110_PrintBMP2(TELA_X-3-1, 2, digito_9, TAM_X_DIGITO, TAM_Y_DIGITO); }
}//fim da funcao renderiza placas

void renderiza_pedras_chao(){
    if(posicao_chao1 <= TELA_X-TAM_X_NUVEM2){ Nokia5110_PrintBMP2( posicao_chao1, POS_CHAO+1, nuvem2, TAM_X_NUVEM2, TAM_Y_NUVEM2); }
    if(posicao_chao2 <= TELA_X-TAM_X_NUVEM2){ Nokia5110_PrintBMP2( posicao_chao2, POS_CHAO+2, nuvem2, TAM_X_NUVEM2, TAM_Y_NUVEM2); }
    if(posicao_chao3 <= TELA_X-TAM_X_NUVEM2){ Nokia5110_PrintBMP2( posicao_chao3, POS_CHAO+3, nuvem2, TAM_X_NUVEM2, TAM_Y_NUVEM2); }
}//fim da funcao renderiza_pedras_chao

void renderiza_nuvens(){
    if(posicao_x_nuvem1_1 <= TELA_X-TAM_X_NUVEM1){ Nokia5110_PrintBMP2( posicao_x_nuvem1_1, 1, nuvem1, TAM_X_NUVEM1, TAM_Y_NUVEM1); }
    if(posicao_x_nuvem2_1 <= TELA_X-TAM_X_NUVEM2){ Nokia5110_PrintBMP2( posicao_x_nuvem2_1, 0, nuvem2, TAM_X_NUVEM2, TAM_Y_NUVEM2); }
    if(posicao_x_nuvem3_1 <= TELA_X-TAM_X_NUVEM3){ Nokia5110_PrintBMP2( posicao_x_nuvem3_1, 0, nuvem3, TAM_X_NUVEM3, TAM_Y_NUVEM3); }
    if(posicao_x_nuvem1_2 <= TELA_X-TAM_X_NUVEM1){ Nokia5110_PrintBMP2( posicao_x_nuvem1_2, 0, nuvem1, TAM_X_NUVEM1, TAM_Y_NUVEM1); }
    if(posicao_x_nuvem2_2 <= TELA_X-TAM_X_NUVEM2){ Nokia5110_PrintBMP2( posicao_x_nuvem2_2, 4, nuvem2, TAM_X_NUVEM2, TAM_Y_NUVEM2); }
}//fim da funcao renderiza_nuvens

void renderiza_posicoes(){//printa todas as iamgens no buffer.

    //renderiza as lojinhas por trás do cavalo e a fogueira
    renderiza_lojas();

    //renderiza o cavalo
    if(posicao_fixa_cavalinho < 84 - TAM_X_CAVALINHO && posicao_fixa_cavalinho >= 0){
        if(direcao_cavalinho > 0){ //verifica direcao do cavalo - se for direita
            Nokia5110_PrintBMP2( posicao_fixa_cavalinho, POS_CHAO-TAM_Y_CAVALINHO, cavalinho_dir[(contador_geral)%3], TAM_X_CAVALINHO, TAM_Y_CAVALINHO);
        }else if(direcao_cavalinho < 0){
            Nokia5110_PrintBMP2( posicao_fixa_cavalinho, POS_CHAO-TAM_Y_CAVALINHO ,cavalinho_esq[(contador_geral)%3], TAM_X_CAVALINHO, TAM_Y_CAVALINHO);
        }else if(direcao_cavalinho == 0 && direcao_anterior_cavalinho >= 0){
            Nokia5110_PrintBMP2( posicao_fixa_cavalinho, POS_CHAO-TAM_Y_CAVALINHO ,cavalinho_dir[1], TAM_X_CAVALINHO, TAM_Y_CAVALINHO);
        }else if(direcao_cavalinho == 0 && direcao_anterior_cavalinho < 0){
            Nokia5110_PrintBMP2( posicao_fixa_cavalinho, POS_CHAO-TAM_Y_CAVALINHO ,cavalinho_esq[1], TAM_X_CAVALINHO, TAM_Y_CAVALINHO);
        }
    }

    //renderiza o centro do reino
    renderiza_centro();

    renderiza_barricadas();

    //renderiza os npcs que estarao no jogo
    renderiza_npcs();

    //nuvens
    renderiza_nuvens();

    //chão
    renderiza_pedras_chao();
    plotLine(0,POS_CHAO,84,POS_CHAO);
    //plotLine(0,POS_CHAO+1,84,POS_CHAO+1);
    plotLine(0,POS_CHAO+(2*TAM_Y_NUVEM2)-1,84,POS_CHAO+(2*TAM_Y_NUVEM2)-1);

    renderiza_vida_objetos();

    renderiza_placar(); //escreve o numero de moedas que o rei tem(placar)

    renderiza_moedas_chao(); //renderiza as moedas que estao no chao

}//fim da funcao renderiza posicoes

void abre_bau(){
    if(possibilidade_bau == true &&
    fabs( posicao_cavalo + (TAM_X_CAVALINHO/2) - posicao_bau ) <= 8){//se bau PODE ser aberto e o cavalo apssa perto
        estado_bau = 1;
        possibilidade_bau = false;
        int moedinhas = rand()%9 + 1; //de 1 até 10 moedas
        total_moedas += moedinhas;
    }
}

void verifica_colisao(){

    //colisao com moedas para coletar elas
    int i;
    for(i=0; i<50; i++){
        if(vetor_moedas[i] == -20000){
            //nao faz nada
        }else if(fabs((posicao_cavalo + (TAM_X_CAVALINHO/2)) - vetor_moedas[i]) <= 6){
            total_moedas++;
            vetor_moedas[i] = -20000;
            //break;
        }
    }

    //verifica se o cavalinho está na fogueira para interagir com ela
    if(fabs((posicao_cavalo + (TAM_X_CAVALINHO/2)) - posicao_fogueira) <= 6){
        if(fogueira_acesa == false){ //se a fogueira está apagada
            //mensagem de LIGHT A FIRE aqui
            if(skiped == false){
                //mostra light a fire em cima da fogueira que deve ser acesa
                Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) - (TAM_X_LIGHTAFIRE/2), 0 , lightafire, TAM_X_LIGHTAFIRE, TAM_Y_LIGHTAFIRE);
            }
            //mostra lugar pra por moeda em cima da cabeça do rei
            Nokia5110_PrintBMP2(posicao_fogueira - posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
            if(le_botao(3) == 1 && total_moedas > 0){ //se botao 3 for apertado e tiver dinheiro
                total_moedas--; //diminui 1 moeda
                fogueira_acesa = true; //ascende fogueira
                //break;
            }
        }else{//fogueira ja está acesa, aqui vai a parte de evoluir o reino

        }
    }

    //colisao com nativos para recrutar eles e mandar pro centro, na fogueirinha
    int j;
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){
            if(vetor_nativo[i].posicao_x == -20000){ //se npc nao existe
                //nao faz nada porque o nativo dessa posicao do vetor nao existe
            }else if(fabs( (posicao_cavalo + (TAM_X_CAVALINHO/2)) - vetor_nativo[i].posicao_x ) <= 6){
                Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2), POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                if(le_botao(3) == 1 && total_moedas > 0){ //se botao 3 for apertado e tiver dinheiro
                    for(j=0;j<25;j++){
                        if(vetor_aldeao[j].posicao_x == -20000){ //se a posicao nao estiver ocupada ele coloca o aldeao novo
                            vetor_aldeao[j].posicao_x = vetor_nativo[i].posicao_x;
                            vetor_nativo[i].posicao_x = -20000;//reseta posicao nativo
                            vetor_aldeao[j].moedas = vetor_nativo[i].moedas;
                            vetor_nativo[i].moedas = 0; //reseta moedas nativo
                            vetor_aldeao[j].origem_x = posicao_fogueira;
                            vetor_nativo[i].origem_x = -20000;
                            total_moedas--; //diminui 1 moeda
                            break;
                        }else{
                            //se a posicao estiver ocupada
                        }
                    }//fim do for 2
                    //break;
                }
                //break;
            }
        }
    }//fim da verificacao de colisao com nativo

    //verifica qual das loja o aldeao deve ir comprar
    for(i=0; i<25; i++){
        if(vetor_aldeao[i].posicao_x != -20000){ //aldeao existe
            if(arcos_venda > 0 || martelos_venda > 0){ //tem martelo OU arcos a venda
                if(fabs(vetor_aldeao[i].posicao_x - posicao_loja_arco) <= fabs(vetor_aldeao[i].posicao_x - posicao_loja_martelo)){
                    //ta mais perto da loja de arco
                    if(arcos_venda > 0){
                        vetor_aldeao[i].origem_x = posicao_loja_arco;
                        break;
                    }else if(martelos_venda > 0){ //nao tem arcos a venda mas tem martelos
                        vetor_aldeao[i].origem_x = posicao_loja_martelo;
                        break;
                    }
                }else{ //ta mais perto da loja de martelo
                    if(martelos_venda > 0){
                        vetor_aldeao[i].origem_x = posicao_loja_martelo;
                        break;
                    }else if(arcos_venda > 0){ //nao tem martelos a venda mas tem arcos
                        vetor_aldeao[i].origem_x = posicao_loja_arco;
                        break;
                    }
                }
            }else{ //nao tem nada pra vender entao vai pra fogueira esperar lá
                vetor_aldeao[i].origem_x = posicao_fogueira;
                break;
            }
        }
    }

    //colisao com loja de arco
    if(fogueira_acesa == true){
        if(fabs((posicao_cavalo + (TAM_X_CAVALINHO/2)) - posicao_loja_arco) <= 6){
            if(arcos_venda < 3){ //se a loja n esta cheia
                //mostra lugar pra por moeda em cima da cabeça do rei
                Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) -3, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) +3, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                if(le_botao(3) == 1 && total_moedas >= 2){ //se botao 3 for apertado e tiver dinheiro
                    total_moedas -= 2; //diminui 2 moeda
                    arcos_venda++; //aumenta um arco
                    //break;
                }
            }
        }
    }

    //colisao com loja de martelo
    if(fogueira_acesa == true){
        if(fabs((posicao_cavalo + (TAM_X_CAVALINHO/2)) - posicao_loja_martelo) <= 6){
            if(martelos_venda < 3){ //se a loja n esta cheia
                //mostra lugar pra por moeda em cima da cabeça do rei
                Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) -6, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) , POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) +6, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                if(le_botao(3) == 1 && total_moedas >= 3){ //se botao 3 for apertado e tiver dinheiro
                    total_moedas -= 3; //diminui 3 moeda
                    martelos_venda++; //aumenta um martelo
                    //break;
                }
            }
        }
    }

    //colisao DO NPC com loja de martelo para pegar um martelo e virar um construtor
    if(fogueira_acesa == true){ //verifica se a loja existe antes de gastar processamento
        for(i=0; i<25; i++){
            if(vetor_aldeao[i].posicao_x != -20000){//se aldeao existe
                if(fabs(vetor_aldeao[i].posicao_x - posicao_loja_martelo) <= 6){
                    if(martelos_venda >= 1){ //se a loja tiver ao menos um martelo
                        for(j=0;j<25;j++){
                            if(vetor_construtor[j].posicao_x == -20000){ //se a posicao nao estiver ocupada ele coloca o construtor novo
                                vetor_construtor[j].posicao_x = vetor_aldeao[i].posicao_x;
                                vetor_aldeao[i].posicao_x = -20000;//reseta posicao aldeao
                                vetor_construtor[j].moedas = vetor_aldeao[i].moedas;
                                vetor_aldeao[i].moedas = 0; //reseta moedas aldeao
                                vetor_construtor[j].origem_x = posicao_loja_martelo + 6;
                                vetor_aldeao[i].origem_x = 0;
                                martelos_venda--; //diminui 1 martelo
                                break;
                            }else{
                                //se a posicao estiver ocupada
                            }
                        }//fim do for 2
                    }
                }
            }
        }
    }

    //colisao DO NPC com loja de ARCO para pegar um ARCO e virar uma ARQUEIRA
    if(fogueira_acesa == true){ //verifica se a loja existe antes de gastar processamento
        for(i=0; i<25; i++){
            if(vetor_aldeao[i].posicao_x != -20000){//se aldeao existe
                if(fabs(vetor_aldeao[i].posicao_x - posicao_loja_arco) <= 6){
                    if(arcos_venda >= 1){ //se a loja tiver ao menos um arco
                        for(j=0;j<25;j++){
                            if(vetor_arqueira[j].posicao_x == -20000){ //se a posicao nao estiver ocupada ele coloca a arqueira nova
                                vetor_arqueira[j].posicao_x = vetor_aldeao[i].posicao_x;
                                vetor_aldeao[i].posicao_x = -20000;//reseta posicao aldeao
                                vetor_arqueira[j].moedas = vetor_aldeao[i].moedas;
                                vetor_aldeao[i].moedas = 0; //reseta moedas aldeao
                                vetor_arqueira[j].origem_x = posicao_loja_arco + 6;
                                vetor_aldeao[i].origem_x = 0;
                                arcos_venda--; //diminui 1 martelo
                                break;
                            }else{
                                //se a posicao estiver ocupada
                            }
                        }//fim do for 2
                    }
                }
            }
        }
    }

    //interacao do construtor com a barricada em construcao
    //aqui temos um bug, ja descoberto:
    /* [BUG] -  INVESTIGAR ISSO - arrumado????!!!!
     * Se um construtor estiver definido para construir a barricada 1, e o 2 pra construir a barricada 2, mas o construtor da
     * barricada 1 termianr antes e chegar a tempo na barricada 2 pra construir ela, no caminho dele indo pra casa, ele vai terminar ela,
     * e vai deixar o construtor da 2 preso na 2 pra sempre(ele vai estar sempre em status ocupado entao ele ta
     * REALMENTE PRESO PRA SEMPRE???)*/

    if(fogueira_acesa == true){
        for(i=0; i<25; i++){ //passa por todas as barricadas
            if(vetor_barricada[i].posicao_x != -20000){ //verifica se a barricada EXISTE
                //se barricada esta com construcao OUUUUUUUUUUUU barricada nao está com a vida cheia(e tambem nao está em cosntrucao, e ja foi terminada) - sendo que construtor NAO FOI CHAMADO AINDA
                if(vetor_barricada[i].construindo == 1 || ((vetor_barricada[i].vida < 10 && vetor_barricada[i].vida > 0) && vetor_barricada[i].construindo == -20000) ){
                    //requisita um construtor e seta ele pra ocupado
                    for(j=0; j<25; j++){ //todos os construtores até achar um
                        if( (vetor_construtor[j].posicao_x != -20000) && (vetor_construtor[j].ocupado == 0) ){//se construtor existe e nao esta ocupado
                            vetor_construtor[j].origem_x = vetor_barricada[i].posicao_x;
                            vetor_construtor[j].ocupado = 10000;
                            vetor_barricada[i].construindo = 0; //construtor ja foi chamado entao seta pra 0
                            break;
                        }
                    }
                }else if(vetor_barricada[i].construindo == 0){ //construtor ja foi chamado
                    for(j=0; j<25; j++){ //todos os construtores sem excecao - verifica se algum construtor esta perto dela
                        //está em cima da barricada CERTA
                        if(fabs( ( vetor_construtor[j].posicao_x + (TAM_X_CONSTRUTOR/2) ) - (vetor_barricada[i].posicao_x + (TAM_X_BARRICADA/2)) ) <= 6
                                && fabs( ( vetor_construtor[j].posicao_x + (TAM_X_CONSTRUTOR/2) ) - ( vetor_construtor[j].origem_x + (TAM_X_CONSTRUTOR/2) ) ) <= 5){//está em cima da barricada
                            vetor_construtor[j].ocupado = 0;
                            vetor_barricada[i].vida = 10;
                            vetor_barricada[i].construindo = -20000; //construtor ja terminou
                            vetor_construtor[j].origem_x = posicao_loja_martelo;
                            //break; agora sem esse break o bug está teoricamente arrumado ja que todos os construtor que tiver perto
                            //da barreira pode construir ela e ir embora
                        }
                    }
                }
            }
        }
    }

    //interacao do rei com a barricada
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){
            if(vetor_barricada[i].posicao_x != -20000 && vetor_barricada[i].construindo == -20000){ //barricada existe e nao esta em contrucao
                if(fabs((posicao_cavalo + (TAM_X_CAVALINHO/2)) - vetor_barricada[i].posicao_x) <= 4){//está em cima da barricada
                    if(vetor_barricada[i].vida <= 0){// --------------------- vida ta vazia(terrinha)
                        if(vetor_barricada[i].nivel == 0){
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2), POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            if(le_botao(3) == 1 && total_moedas >= 1 && vetor_barricada[i].construindo == -20000){ //se botao 3 for apertado e tiver dinheiro
                                vetor_barricada[i].construindo = 1;//define status pra em construcao
                                vetor_barricada[i].vida = 0;
                                vetor_barricada[i].nivel = 0;
                                total_moedas-=1; //diminui uma moeda
                                break;
                            }
                        }else if(vetor_barricada[i].nivel == 1){
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) -3, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) +3, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            if(le_botao(3) == 1 && total_moedas >= 2 && vetor_barricada[i].construindo == -20000){ //se botao 3 for apertado e tiver dinheiro
                                vetor_barricada[i].construindo = 1;//define status pra em construcao
                                vetor_barricada[i].vida = 0;
                                vetor_barricada[i].nivel = 1;
                                total_moedas -= 2; //diminui 2 moeda
                                break;
                            }
                        }else if(vetor_barricada[i].nivel == 2){
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) -6, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) , POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) +6, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            if(le_botao(3) == 1 && total_moedas >= 3 && vetor_barricada[i].construindo == -20000){ //se botao 3 for apertado e tiver dinheiro
                                vetor_barricada[i].construindo = 1;//define status pra em construcao
                                vetor_barricada[i].vida = 0;
                                vetor_barricada[i].nivel = 2;
                                total_moedas -= 3; //diminui 3 moeda
                                break;
                            }
                        }

                    }else if(vetor_barricada[i].vida >= 10){ // -------------------- vida ta cheia
                        if(vetor_barricada[i].nivel == 0){
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) -3, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) +3, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            if(le_botao(3) == 1 && total_moedas >= 2 && vetor_barricada[i].construindo == -20000){ //se botao 3 for apertado e tiver dinheiro
                                vetor_barricada[i].construindo = 1;//define status pra em construcao
                                vetor_barricada[i].vida = 0;
                                vetor_barricada[i].nivel = 1;
                                total_moedas -= 2; //diminui 2 moeda
                                break;
                            }
                        }else if(vetor_barricada[i].nivel == 1){
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) -6, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) , POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            Nokia5110_PrintBMP2(posicao_fixa_cavalinho + (TAM_X_CAVALINHO/2) +6, POS_CHAO - TAM_Y_CAVALINHO - TAM_Y_MOEDA_GRANDE -3, moeda_grande, TAM_X_MOEDA_GRANDE, TAM_Y_MOEDA_GRANDE);
                            if(le_botao(3) == 1 && total_moedas >= 3 && vetor_barricada[i].construindo == -20000){ //se botao 3 for apertado e tiver dinheiro
                                vetor_barricada[i].construindo = 1;//define status pra em construcao
                                vetor_barricada[i].vida = 0;
                                vetor_barricada[i].nivel = 2;
                                total_moedas -= 3; //diminui 3 moeda
                                break;
                            }
                        }else if(vetor_barricada[i].nivel == 2){
                            //nao da pra fazer nada pq ta no nivel maximo e com vida maxima
                        }

                    }else{ // --------------------------- VIDA TA NO MEIO TERMO
                                //construtor vem arrumar atomaticamente entao esta interacao nao eh necesaria
                    }
                }
            }
        }
    }

    //interacao da arqueira com o rei pra pegar a moeda
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){
            if(vetor_arqueira[i].posicao_x != -20000){ //se ela existe
                if(fabs( (posicao_cavalo + (TAM_X_CAVALINHO/2)) - vetor_arqueira[i].posicao_x ) <= 6){ //se ta proxima do rei
                    if(vetor_arqueira[i].moedas > 0){//se tem moedas
                        total_moedas += vetor_arqueira[i].moedas; //adiciona as moedas pro rei
                        vetor_arqueira[i].moedas = 0; //zera moedas da arqueira
                    }
                }
            }
        }
    }//fim da verificacao de colisao

    //interacao da arqueira com os monstros pra matar eles
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){ //passa por todos os monstros
            if(vetor_monstro[i].posicao_x != -20000){ //verifica se o coelho existe
                for(j=0;j<25;j++){//passa por todas as arqueiras
                    if(vetor_arqueira[j].posicao_x != -20000){
                        if( fabs(vetor_arqueira[j].posicao_x - vetor_monstro[i].posicao_x) <= 25 &&
                        fabs(vetor_arqueira[j].posicao_x - vetor_arqueira[j].origem_x) <= 25){ //se ta proxima do monstro e proxima do destino que era pra ela estar
                            //a cada 1.5+- seg ela atira
                            if(contador_geral % (3*seg) == 0){
                                //tem uma pequena chance de acertar
                                int tentativa = rand()%3 - 1; //numero entre -1 e 2, sendo que é o desvio da flecha em y

                                //imprime linha do tiro
                                int tamanho_linha = fabs(vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2) - posicao_cavalo + posicao_fixa_cavalinho - vetor_monstro[i].posicao_x + (TAM_X_MONSTRO/2) - posicao_cavalo + posicao_fixa_cavalinho);
                                int menor_x_linha;
                                int maior_x_linha;

                                if(vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2)> vetor_monstro[i].posicao_x + (TAM_X_MONSTRO/2)){
                                    maior_x_linha = vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2);
                                    menor_x_linha = vetor_monstro[i].posicao_x + (TAM_X_MONSTRO/2);
                                }else{
                                    menor_x_linha = vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2);
                                    maior_x_linha = vetor_monstro[i].posicao_x + (TAM_X_MONSTRO/2);
                                }

                                if(menor_x_linha - posicao_cavalo + posicao_fixa_cavalinho >= 0 && maior_x_linha - posicao_cavalo + posicao_fixa_cavalinho < TELA_X){
                                    plotLine(vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2) - posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO - (TAM_Y_ARQUEIRA/2), vetor_monstro[i].posicao_x + (TAM_X_MONSTRO/2) - posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO - (TAM_Y_MONSTRO/2) + (tentativa*4));
                                    SysCtlDelay((SysCtlClockGet()*2 )/200);
                                }

                                if(tentativa == 0){
                                    //mata o monstro
                                    vetor_monstro[i].posicao_x = -20000;
                                    vetor_monstro[i].origem_x = -20000;
                                    vetor_monstro[i].direcao = 0;
                                    vetor_monstro[i].status = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //interacao do monstro com a barricada pra destruir ela
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){
            if(vetor_monstro[i].posicao_x != -20000){//monstro existe
                for(j=0;j<25;j++){ //percorre barricadas
                    if(fabs( vetor_monstro[i].posicao_x - vetor_barricada[j].posicao_x ) <= 5){ //se monstro ta eprto da barricada
                        if( ( (contador_geral+1)%(1 * 2*seg) ) == 0){ //ataca a cada 1seg
                            if(vetor_barricada[j].vida > 0){
                                vetor_monstro[i].origem_x = vetor_barricada[j].posicao_x; //poe o monstro pra atacar essa barricada até ela cair
                                switch (vetor_barricada[j].nivel){
                                    case 0:
                                        vetor_barricada[j].vida -= 3;
                                        break;
                                    case 1:
                                        vetor_barricada[j].vida -= 2;
                                        break;
                                    case 2:
                                        vetor_barricada[j].vida -= 1;
                                        break;
                                }
                            }else{ //se a vida da barricada for menor que 0
                                if(contador_dia%2 == 1){//noite impar
                                    vetor_monstro[i].origem_x = monstro_spawn2;
                                }else{//noite par
                                    vetor_monstro[i].origem_x = monstro_spawn1;
                                }
                            }
                        }
                    }
                    if(vetor_barricada[j].vida < 0){
                        vetor_barricada[j].vida = 0;
                    }
                }
            }
        }
    }

    //interacao com o npc pra matar ele
    int k;
    //mata arqueira
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){
            if(vetor_monstro[i].posicao_x != -20000){//monstro existe
                for(j=0;j<25;j++){ //percorre arqueiras
                    if(fabs( vetor_monstro[i].posicao_x - vetor_arqueira[j].posicao_x ) <= 3){ //se monstro ta perto da arqueira
                        if( ( (contador_geral+1)%(1 * 2*seg) ) == 0){ //ataca a cada 1seg
                            //mata a arqueira
                            for(k=0;k<25;k++){ //percorre todos os coelhos
                                if(fabs( vetor_arqueira[j].origem_x - vetor_coelho[k].posicao_x ) <= 11){ //se a posicao do coelho bate com a posicao que a aruqeira tava indo pra matar
                                    vetor_coelho[k].status = 0; //coloca o status do coelho pra disponivel pra caçar
                                }
                            }

                            //mata arqueira - tranforma em aldeao
                            for(k=0; k<25; k++){
                                if(vetor_aldeao[k].posicao_x == -20000){
                                    vetor_aldeao[k].posicao_x = vetor_arqueira[j].posicao_x;
                                    vetor_aldeao[k].origem_x = posicao_fogueira;
                                    break;
                                    //outros atributos do aldeao nao sao usados
                                }
                            }
                            //apaga arqueira
                            vetor_arqueira[j].posicao_x = -20000;
                            vetor_arqueira[j].origem_x = -20000;
                            vetor_arqueira[j].ocupado = 0;
                            vetor_arqueira[j].moedas = 0;
                        }
                    }
                }
            }
        }
    }

    //mata construtor
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){
            if(vetor_monstro[i].posicao_x != -20000){//monstro existe
                for(j=0;j<25;j++){ //percorre construtor
                    if(fabs( vetor_monstro[i].posicao_x - vetor_construtor[j].posicao_x ) <= 3){ //se monstro ta perto da arqueira
                        if( ( (contador_geral+1)%(1 * 2*seg) ) == 0){ //ataca a cada 1seg
                            //mata o construtor
                            for(k=0;k<25;k++){ //percorre todas aas barricadas
                                if(fabs( vetor_construtor[j].origem_x - vetor_barricada[k].posicao_x ) <= 5){ //se a posicao do coelho bate com a posicao que a aruqeira tava indo pra matar
                                    vetor_barricada[k].construindo = -20000;
                                }
                            }

                            //mata arqueira - tranforma em aldeao
                            for(k=0; k<25; k++){
                                if(vetor_aldeao[k].posicao_x == -20000){
                                    vetor_aldeao[k].posicao_x = vetor_construtor[j].posicao_x;
                                    vetor_aldeao[k].origem_x = posicao_fogueira;
                                    break;
                                    //outros atributos do aldeao nao sao usados
                                }
                            }
                            //apaga arqueira
                            vetor_construtor[j].posicao_x = -20000;
                            vetor_construtor[j].origem_x = -20000;
                            vetor_construtor[j].ocupado = 0;
                            vetor_construtor[j].moedas = 0;
                        }
                    }
                }
            }
        }
    }

    //mata nativos
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){
            if(vetor_monstro[i].posicao_x != -20000){//monstro existe
                for(j=0;j<25;j++){ //percorre aldeao
                    if(fabs( vetor_monstro[i].posicao_x - vetor_aldeao[j].posicao_x ) <= 3){ //se monstro ta perto do aldeao
                        if( ( (contador_geral+1)%(1 * 2*seg) ) == 0){ //ataca a cada 1seg

                            //mata aldeao - tranforma em nativo
                            for(k=0; k<25; k++){
                                if(vetor_nativo[k].posicao_x == -20000){
                                    vetor_nativo[k].posicao_x = vetor_aldeao[j].posicao_x;
                                    if(contador_dia%2 == 1){//noite impar
                                        vetor_nativo[k].origem_x = vetor_acampamentos[1].posicao_x;
                                    }else{//noite par
                                        vetor_nativo[k].origem_x = vetor_acampamentos[0].posicao_x;
                                    }
                                    break;
                                    //outros atributos do nativo foda-se
                                }
                            }

                            //apaga aldeao
                            vetor_aldeao[j].posicao_x = -20000;
                            vetor_aldeao[j].origem_x = -20000;
                            vetor_aldeao[j].ocupado = 0;
                            vetor_aldeao[j].moedas = 0;
                        }
                    }
                }
            }
        }
    }

    //mata rei... ------------------------------------------------------------------------- MATA REI
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){
            if(vetor_monstro[i].posicao_x != -20000){//monstro existe
                if(fabs( vetor_monstro[i].posicao_x - posicao_cavalo ) <= 5){ //se monstro ta perto da arqueira
                    if( ( (contador_geral+1)%(1 * 2*seg) ) == 0){
                        //ataca rei
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0xFF); //ascende led vermelho
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0x00);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);
                        SysCtlDelay((SysCtlClockGet()*3 )/200);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0xFF); //ascende led vermelho
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0xFF); //azul
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00); //verde
                        SysCtlDelay((SysCtlClockGet()*5 )/200);
                        vida_rei -= 1;
                    }
                }
            }
        }
    }

    abre_bau(); //verifica se o bau pode ser aberto e abre o bau

}//fim da funcao verifica colisao

void spawna_entidades(){
    //spawna coelhos a cada tempo
    if( ( (contador_geral+1)%(15 * 2*seg) ) == 0){ // ------------------------- TEMPO DE SPAWN COELHOS: 15 seg +-
        int i;
        for(i=0; i<25; i++){
            if(vetor_coelho[i].posicao_x == -20000){//nao existe coelho nessa posicao do vetor
                int posicao_spawn = (rand()%400) - 200 + (posicao_fogueira/2); //nmero randomico entre -200 e +200 pra lá ou pra ca da fogueira
                vetor_coelho[i].origem_x = posicao_spawn;
                vetor_coelho[i].posicao_x = posicao_spawn;
                vetor_coelho[i].direcao = 0; //qualquer valor >= 0 eh pra direita
                break;
            }
        }
    }

    //spawna mendigos nos acampamentos
    if(fogueira_acesa == true){
        if( ( (contador_geral+1)%(90 * 2*seg) ) == 0){ // ------------------------- TEMPO DE SPAWN mendigos: 90 seg+-
            int i,j;
            for(i=0; i<5; i++){//propcura todos os acampamentos
                vetor_acampamentos[i].mendigos = 0;
                for(j=0; j<10; j++){//procura todos os mendigos e calcula quantos estao perto do acamapmento em questao
                    if(fabs( vetor_acampamentos[i].posicao_x - vetor_nativo[j].posicao_x ) <= 10){
                        vetor_acampamentos[i].mendigos += 1; //soma um mendigo se tiver um perto
                    }
                }
                if(vetor_acampamentos[i].posicao_x != -20000 && vetor_acampamentos[i].mendigos < 2){ //acampamento existe e tem menos que 2 mendigos
                    //spawna mendigo proximo ao acampamento
                    for(j=0; j<25; j++){ //pocura posicao de mendigo valida
                        if(vetor_nativo[j].posicao_x == -20000){//nao existe nativo(mendigo) nessa posicao do vetor
                            int variacao = (rand()%10) - 5; //variacao de 5 pra la ou pra ca
                            vetor_nativo[j].origem_x = vetor_acampamentos[i].posicao_x + variacao;
                            vetor_nativo[j].posicao_x = vetor_nativo[j].origem_x;
                            //vetor_acampamentos[i].mendigos += 1; //aumenta um mendigo no acampamento isso é inutil agora, calcula toda vez
                            break;
                        }
                    }//fim do for - nao achou nativo ou ja alocou um nativo
                    break; //assim, ele só spawna UM MENDIGO por vez em cada acampamento e nao um em cada ao mesmo tempo
                }
            }
        }
    }
}

void calcula_dia(){
    if(fogueira_acesa == true){
        if( (contador_geral+1) % (120*2*seg) == 0) { //lembrando que seg é na verdade tipo 0.5 segundo entao o segundo numero ali é o min
            if(dia){
                dia = false;
                noite = true; // --------------- torna noite

            }else if(noite){
                dia = true; // ------ TORNA DIA
                vida_rei = 5;
                contador_dia +=1; //aumenta um dia
                //dai aqui abre o bau tambem e da moedinhas
                if(fogueira_acesa == true){
                    estado_bau = 0; //fecha bau na força
                    possibilidade_bau = true; //permite abrir bau
                }
                noite = false;
                int i;
                for(i=0;i<25;i++){ //deixa todos os npcs disponiveis novamente
                    if(vetor_arqueira[i].ocupado == 1){ //se arqueira ta ocupada
                        vetor_arqueira[i].ocupado = 0;
                    }
                }
                //zera monstros spawnados no mapa
                contador_monstros = 0;
                for(i=0; i<25; i++){
                    if(vetor_monstro[i].posicao_x != -20000){//existe um monstro
                        vetor_monstro[i].direcao = 0;
                        vetor_monstro[i].origem_x = -20000;
                        vetor_monstro[i].posicao_x = -20000;
                        vetor_monstro[i].status = 0;
                    }
                }
            }
        }

        if(noite){ //se for noite, entao pode spawnar monstros ------------------------ CUIDADO COM A QNTDADE DO SPAWN DOS MONSTROS

            //spawna_monstros
            if( ( (contador_geral+1)%seg ) == 0 && contador_monstros < contador_dia){ // ------------------------- TEMPO DE SPAWN monstros: 0.5 seg
                int i;
                for(i=0; i<25; i++){
                    if(vetor_monstro[i].posicao_x == -20000){//nao existe monstro nessa posicao do vetor

                        //noite impar é na esquerda noite par é na direita
                        if(contador_dia%2 == 1){//noite impar
                            vetor_monstro[i].posicao_x = monstro_spawn1;
                            vetor_monstro[i].origem_x = monstro_spawn2;
                        }else{//noite par
                            vetor_monstro[i].posicao_x = monstro_spawn2;
                            vetor_monstro[i].origem_x = monstro_spawn1;
                        }
                        vetor_monstro[i].direcao = 0; //qualquer valor >= 0 eh pra direita
                        contador_monstros += 1;
                        break; //pra spawnar um montro só por vez
                    }
                }
            }

            int barricada_longe, barricadinha, i;
            //calcula ultima barricada amis distante do lado que os monstros vao nascer
            if(contador_dia % 2 == 1){//noite impar - deve calcular a barricada mais pra esquerda
                barricadinha = posicao_fogueira;
                for(i=0;i<25;i++){
                    if(vetor_barricada[i].vida > 0){ //a barricada tem que estar erguida e nao destruida
                        if(vetor_barricada[i].posicao_x < barricadinha){
                            barricadinha = vetor_barricada[i].posicao_x;
                        }
                    }
                }
                barricada_longe = barricadinha + TAM_X_BARRICADA + 2;
            }else{//noite par - barricada mais pra direita
                barricadinha = posicao_fogueira;
                for(i=0;i<25;i++){
                    if(vetor_barricada[i].vida > 0){ //a barricada tem que estar erguida e nao destruida
                        if(vetor_barricada[i].posicao_x > barricadinha){
                            barricadinha = vetor_barricada[i].posicao_x;
                        }
                    }
                }
                barricada_longe = barricadinha - 5;
            }

            for(i=0; i<25; i++){ //tira todos os coelhos de estao sendo caçados
                vetor_coelho[i].status = 0;
            }

            for(i=0;i<25;i++){ //vai colocando as arqueiras pra defender a barricada mais longe
                if(vetor_arqueira[i].posicao_x != -20000){
                    vetor_arqueira[i].origem_x = barricada_longe;
                    vetor_arqueira[i].ocupado = 1;
                }
            }
        }else if(dia){ //se for de dia, os monstros vao embora
            //espanta todos os monstros - mata, nao sei
            //nao é aqui pq aqui ele faz varias vezes nao uma só

        }
    }
    // 1- vermelho 2- azul 3- verde
    if(dia){//ascende led na cor amarela (vermelho + verde)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0xFF);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0xFF);
    }else if(noite){ //ascende o led na cor azul
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0x00);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0xFF);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);
    }
}

void arqueira_mata_coelho(){

    int i, j;

    //verifica se tem coelho e leva a primeira arqueira disponivel
    for(i=0;i<25;i++){
        if(vetor_coelho[i].posicao_x != -20000 && vetor_coelho[i].status == 0){ //se coelho existe e nao esta sendo caçado
            for(j=0;j<25;j++){
                if(vetor_arqueira[j].posicao_x != -20000 && vetor_arqueira[j].ocupado != 1){ //arqueira existe e nao ta ocupada
                    int variacao = rand()%10 -5; //de -5 até 5
                    vetor_arqueira[j].origem_x = vetor_coelho[i].posicao_x + variacao;
                    vetor_arqueira[j].ocupado = 1; //ta ocupada agora
                    vetor_coelho[i].status = 1; //coelho está sendo caçado
                    break;
                }
            }
        }
    }

    //interacao da arqueira com o coelho pra matar ele
    if(fogueira_acesa == true){
        for(i=0; i<25; i++){ //passa por todos os coelhos
            if(vetor_coelho[i].posicao_x != -20000){ //verifica se o coelho existe
                for(j=0;j<25;j++){//passa por todas as arqueiras
                    if(vetor_arqueira[j].posicao_x != -20000){
                        if( fabs(vetor_arqueira[j].posicao_x - vetor_coelho[i].posicao_x) <= 25 &&
                        fabs(vetor_arqueira[j].posicao_x - vetor_arqueira[j].origem_x) <= 25){ //se ta proximo o bastante do coelho CERTO
                            //a cada 1+- seg ela atira
                            if(contador_geral % (3*seg) == 0){
                                //tem uma pequena chance de acertar
                                int tentativa = rand()%3 - 1; //numero entre -1 e 2, sendo que é o desvio da flecha em y

                                //imprime linha do tiro
                                int tamanho_linha = fabs(vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2) - posicao_cavalo + posicao_fixa_cavalinho - vetor_coelho[i].posicao_x + (TAM_X_COELHO/2) - posicao_cavalo + posicao_fixa_cavalinho);
                                int menor_x_linha;
                                int maior_x_linha;

                                if(vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2)> vetor_coelho[i].posicao_x + (TAM_X_COELHO/2)){
                                    maior_x_linha = vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2);
                                    menor_x_linha = vetor_coelho[i].posicao_x + (TAM_X_COELHO/2);
                                }else{
                                    menor_x_linha = vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2);
                                    maior_x_linha = vetor_coelho[i].posicao_x + (TAM_X_COELHO/2);
                                }

                                if(menor_x_linha - posicao_cavalo + posicao_fixa_cavalinho >= 0 && maior_x_linha - posicao_cavalo + posicao_fixa_cavalinho < TELA_X){
                                    plotLine(vetor_arqueira[j].posicao_x + (TAM_X_ARQUEIRA/2) - posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO - (TAM_Y_ARQUEIRA/2), vetor_coelho[i].posicao_x + (TAM_X_COELHO/2) - posicao_cavalo + posicao_fixa_cavalinho, POS_CHAO - (TAM_Y_COELHO/2) + (tentativa*4));
                                    SysCtlDelay((SysCtlClockGet()*2 )/200);
                                }

                                if(tentativa == 0){
                                    //mata o coelho e dá uma moeda pra arqueira
                                    vetor_coelho[i].posicao_x = -20000;
                                    vetor_coelho[i].origem_x = -20000;
                                    vetor_coelho[i].direcao = 0;
                                    vetor_coelho[i].status = 0;
                                    vetor_arqueira[j].moedas += 1;
                                    vetor_arqueira[j].ocupado = 0;
                                    /*if( fabs( vetor_arqueira[j].posicao_x - vetor_arqueira[j].origem_x) <= 25 ){//está perto do coelho CERTO, dai ela caga pra
                                        NA VERDADE TIRA ISSO PQ PODE DAR PROBLEMA DE UMA ARQUEIRA MATAR O COELHO DA OUTRA E DAI FODEU
                                        vetor_arqueira[j].ocupado = 0;
                                        //vetor_arqueira[j].origem_x = posicao_fogueira; - deixa a origem la onde nasceu o coelho kkkk
                                    }*/
                                }
                            }
                        }
                    }
                }
            }
        }
    }


}

// ---------------------------------------------------- INÍCIO DA MAIN
int main(void) {

    //falta ainda:
    // OKKK - colisao dos monstros com as barreiras - e destruir elas
    // OK ??? - mosntros matarem npcs

    //arrumar bugs
    // OKKK - fazer os aldeaos pegarem o que estiver disponivel de ferramentas e nao o que tiver mais perto lá

    // MEIO QUE OK mas fodase - tutorial
    // OKKKK - monstros ja morrem, já vao pro centro, nas nascem, já sao renderizados.
    //  OKKKK aparentemente TA TOP - toque de recolher de noite pro pessoal voltar pra fogueira central
    // OKKK aparentemente TA TOP - bau abrir quando fica de dia
    // OKKKK - acho que tem que encurtar a noite? nao sei
    // OKKKK - acampamento que os mendigos nascem
    // OKKKK - contador de noites sobrevividas

    // OKKK??? - fazer as arqueiras irem pra proximo da barreira mais longe quando ficar de noite e nao pro centro
    // OKKKK - arrumar pras arqueiras matarem os coelhos até acabar mesmo, fazer uma lógica melhor

    // rei n morre ainda
    // construtor tem que arrumar barricada que nao ta 10/10

    //variavel de debug
    while(1){
        bool DEBUG = false;

        SysCtlClockSet(SYSCTL_SYSDIV_1|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);

        Nokia5110_Init();
        Nokia5110_Clear();
        Nokia5110_ClearBuffer();

        //variavel que ativa ou desativa o tutorial
        skiped = false;

        //configura os botoes e tudo mais
        configura();

        //variaveis de iniciacao da tela de carregamento
        bool comecou = false;
        int TM_comecou = 0;
        int countdown_iniciar = 10*seg; //seg na verdade vale como 0.5 segundos

        int i;
        for(i=0;i<9;i++){ //tela de carregamento inicial
            if(DEBUG){
                break;
            }
            Nokia5110_Clear();
            Nokia5110_PrintBMP2(34, 10, cavalinho_dir[i%3], TAM_X_CAVALINHO, TAM_Y_CAVALINHO);
            Nokia5110_PrintBMP2(32,30,barrinha[i/3],19,6);
            Nokia5110_DisplayBuffer();
            SysCtlDelay(16000000u/10u);
        }

        inicia_posicoes();

        while(1){ //entra no programa principal
            SysCtlDelay((SysCtlClockGet())/200);
            SysCtlDelay(16000000u/30u);

            //clear geral
            Nokia5110_Clear();
            Nokia5110_ClearBuffer();

            if(vida_rei <= 0){ //game over
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0xFF); //vermelho
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0x00);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00);
                SysCtlDelay((SysCtlClockGet()*100)/200);
                break; //sai do programa principal, e volta pra rotina inicial, o que faz o jogo resetar!
            }

            move_npcs(); //move todos os npcs pelo mapa, coelhos, vilager, arqueira, construtor...
            spawna_entidades(); //spawna as entidades como coelhos em lugares aleatorios e os monsros, e os mendigos de tempo em tempo

            //renderiza no buffer todas as posicoes dos objetos
            renderiza_posicoes();

            //verifica colisoes dos objetos com outros objetos
            verifica_colisao();

            calcula_dia(); //essa funcao cuida do dia e da noite e do led

            arqueira_mata_coelho();

            if(!comecou){ //se nao comecou
                Nokia5110_PrintBMP2(3,0,inicial,TAM_X_INICIAL,TAM_Y_INICIAL); //desenha a msg incial
                Nokia5110_PrintBMP2(7,40,skip,TAM_X_SKIP,TAM_Y_SKIP); //desenha a msg skip
                countdown_iniciar--;//decresce o countdown iniciar a cada ciclo

                if(le_botao(3) == 1){//se o botao de acao for pressionado(skip)
                    skiped = true; //skip vai ser true
                    comecou = true; //quer dizer que comecou
                    TM_comecou = 5*seg; //seta o tempo da mensagem comecou pra x seg
                    Nokia5110_Clear();//apaga tudo da tela
                }

                if(le_botao(1) == 1 || le_botao(2) == 1){ //se andou pra um dos lados
                    comecou = true; //quer dizer que comecou
                    TM_comecou = 5*seg; //seta o tempo da mensagem comecou pra x seg
                    Nokia5110_Clear();//apaga tudo da tela
                }

                if(countdown_iniciar <= 0){//se acabou o tempo de decisao e ele nem apertou nada, soh comeca...
                    comecou = true;//quer dizer que comecou ja
                    Nokia5110_Clear();//apaga tudo da tela
                }
            }else{
                if(TM_comecou > 0){//enquanto o TM do "comecou" for maior que zero, desenha a mensagem inicial
                    Nokia5110_PrintBMP2(3,0,inicial,TAM_X_INICIAL,TAM_Y_INICIAL);//printa mensagem inicial para ficar só ela na tela
                    TM_comecou--;//diminui o TM de comecou a cada ciclo
                }
            }//fim do inicio do jogo

            if(le_botao(1) == 1){
                desloca_x(VEL_ROLAGEM); //rola as coisas pra direita
            }else if(le_botao(2) == 1){
                desloca_x(-VEL_ROLAGEM); //rola as coisas as coisas pra esquerda
            }else{//nao se move
                desloca_x(0);
            }

            Nokia5110_DisplayBuffer();//printa o buffer na tela
            contador_geral++;//aumenta o contador
        }//fim do loop infinito
    }
}//fim do programa
