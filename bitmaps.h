#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "Nokia5110.h"
#include "driverlib/hibernate.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"

//digitos
const int TAM_X_DIGITO = 3;
const int TAM_Y_DIGITO= 5;

const uint8_t digito_0[] = { //zero
  0x1f, 0x11, 0x1f
};

const uint8_t digito_1[] = { //um
  0x00, 0x02, 0x1f
};

const uint8_t digito_2[] = { //dois
  0x1d, 0x15, 0x17
};

const uint8_t digito_3[] = { //tres
  0x11, 0x15, 0x1f
};

const uint8_t digito_4[] = {
  0x07, 0x04, 0x1f
};

const uint8_t digito_5[] = { //cinco
  0x17, 0x15, 0x1d
};

const uint8_t digito_6[] = { //seis
  0x1f, 0x15, 0x1d
};

const uint8_t digito_7[] = { //sete
  0x01, 0x01, 0x1f
};

const uint8_t digito_8[] = { //oito
  0x1f, 0x15, 0x1f
};

const uint8_t digito_9[] = { //nove
  0x17, 0x15, 0x1f
};

// ---------------------- moedinhas

int TAM_X_MOEDA_GRANDE = 4;
int TAM_Y_MOEDA_GRANDE = 5;
const uint8_t moeda_grande[] = {
  0x0e, 0x1f, 0x1f, 0x0e
};

int TAM_X_MOEDA_PEQUENA = 2;
int TAM_Y_MOEDA_PEQUENA = 3;
const uint8_t moeda_pequena[] = {
  0x07, 0x07
};

// ----------------------- itens do mapa

//acampamento dos mendigo
const int TAM_X_ACAMPAMENTO = 19;
const int TAM_Y_ACAMPAMENTO = 10;
const uint8_t acampamento[2][38] = {
  { 0x80, 0xf0, 0xfc, 0xfe, 0xfc, 0xfc, 0xfc, 0xfc, 0xfe, 0xff, 0xf8, 0x80, 0x00, 0x00, 0x00, 0xc2, 0xc0, 0x08, 0x00,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x02, 0x03, 0x03, 0x03, 0x03, 0x02 },
  { 0x80, 0xf0, 0xfc, 0xfe, 0xfc, 0xfc, 0xfc, 0xfc, 0xfe, 0xff, 0xf8, 0x80, 0x00, 0x00, 0x10, 0x80, 0xe1, 0xc4, 0x00,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x02, 0x02, 0x03, 0x03, 0x03, 0x02 }
};

//bau
int TAM_X_BAU = 6;
int TAM_Y_BAU = 5;
const uint8_t bau[2][6] = {
{ 0x1c, 0x1e, 0x1e, 0x1e, 0x1e, 0x1c },
{ 0x1e, 0x1b, 0x19, 0x19, 0x1b, 0x1e }
};

//fogueira
int TAM_X_FOGUEIRA = 10;
int TAM_Y_FOGUEIRA = 10;
const uint8_t fogueira[2][20] = {
{ 0x00, 0x00, 0x00, 0xf0, 0xfc, 0xe0, 0xc1, 0x00, 0x00, 0x00,
  0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02 },
{ 0x00, 0x00, 0x00, 0x80, 0xf1, 0xfc, 0xe0, 0x04, 0x00, 0x00,
  0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02 }
};

//loja arco
int TAM_X_LOJA_ARCO = 20;
int TAM_Y_LOJA_ARCO = 20;
const uint8_t loja_arco[2][60] = {
{ 0x02, 0xff, 0x8e, 0x76, 0x06, 0xfe, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0xff, 0x07, 0xc7, 0xc7, 0xe7, 0x04, 0x00, 0x00, 0x00, 0x80, 0x40, 0xc0, 0x00, 0x80, 0x40, 0xc0, 0x00, 0x00, 0x00,
  0x0e, 0x0f, 0x0e, 0x0f, 0x0f, 0x0f, 0x0e, 0x0c, 0x03, 0x0d, 0x03, 0x05, 0x07, 0x01, 0x03, 0x05, 0x07, 0x0d, 0x03, 0x0c },
{ 0x02, 0xff, 0x8e, 0x76, 0x06, 0xfe, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0xff, 0x07, 0xe7, 0xe7, 0xf7, 0x04, 0x00, 0x00, 0x00, 0x80, 0x40, 0xc0, 0x00, 0x80, 0x40, 0xc0, 0x00, 0x00, 0x00,
  0x0e, 0x0f, 0x0e, 0x0f, 0x0f, 0x0f, 0x0e, 0x0c, 0x03, 0x0d, 0x03, 0x05, 0x07, 0x01, 0x03, 0x05, 0x07, 0x0d, 0x03, 0x0c }
};

//loja martelo
int TAM_X_LOJA_MARTELO = 20;
int TAM_Y_LOJA_MARTELO = 20;
const uint8_t loja_martelo[2][60] = {
{  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xe6, 0x06, 0xe6, 0xfe, 0x02, 0x00, 0x00,
  0x00, 0x60, 0xe0, 0x60, 0x00, 0x60, 0xe0, 0x60, 0x00, 0x00, 0x00, 0x04, 0xff, 0x07, 0x06, 0xe7, 0xe7, 0xe4, 0x40, 0x00,
  0x0f, 0x01, 0x07, 0x01, 0x01, 0x01, 0x07, 0x01, 0x0f, 0x00, 0x0e, 0x0e, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xe6, 0x06, 0xe6, 0xfe, 0x02, 0x00, 0x00,
  0x00, 0x60, 0xe0, 0x60, 0x00, 0x60, 0xe0, 0x60, 0x00, 0x00, 0x00, 0x04, 0xff, 0x07, 0x06, 0xf7, 0xf7, 0xf4, 0x20, 0x00,
  0x0f, 0x01, 0x07, 0x01, 0x01, 0x01, 0x07, 0x01, 0x0f, 0x00, 0x0e, 0x0e, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0e, 0x0e }
};

//baricadas
const int TAM_X_BARRICADA = 7;
const int TAM_Y_BARRICADA = 9;
const uint8_t barricada[3][14] = {
   { 0x00, 0x04, 0xdc, 0xf8, 0xf0, 0xfc, 0x3f,
     0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
   { 0x80, 0xf8, 0xff, 0xff, 0xff, 0xf8, 0x80,
     0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
   { 0x80, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0x80,
     0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}
};

const int TAM_X_NUVEM1 = 11;
const int TAM_Y_NUVEM1= 5;
const uint8_t nuvem1[] = {
  0x06, 0x1e, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x0f, 0x0e
};

const int TAM_X_NUVEM2 = 5;
const int TAM_Y_NUVEM2= 4;
const uint8_t nuvem2[] = {
  0x07, 0x0f, 0x0f, 0x0e, 0x0e
};

const int TAM_X_NUVEM3 = 10;
const int TAM_Y_NUVEM3 = 6;
const uint8_t nuvem3[] = {
  0x1e, 0x1e, 0x3e, 0x3f, 0x3f, 0x3f, 0x3f, 0x1f, 0x1e, 0x1c
};

// ------------------------------------------------------------------------- personagens

// ------------- colehinhooo
const int TAM_X_COELHO = 5;
const int TAM_Y_COELHO = 5;
const uint8_t coelho_dir[2][5] = {
  { 0x08, 0x18, 0x18, 0x1e, 0x18},
  { 0x08, 0x1c, 0x0d, 0x1e, 0x0c}
};

const uint8_t coelho_esq[2][5] = {
  { 0x18, 0x1e, 0x18, 0x18, 0x08},
  { 0x0c, 0x1e, 0x0d, 0x1c, 0x08}
};

// -------------- monstro

const int TAM_X_MONSTRO = 8;
const int TAM_Y_MONSTRO = 8;
const uint8_t monstro_dir[2][8] = {
  { 0xc0, 0xf0, 0xfc, 0xfe, 0x3f, 0xfb, 0xb1, 0x1b },
  { 0xc0, 0xf8, 0xfe, 0x7e, 0xbb, 0xf1, 0x3b, 0x1e }
};

const uint8_t monstro_esq[2][8] = {
  { 0x1b, 0xb1, 0xfb, 0x3f, 0xfe, 0xfc, 0xf0, 0xc0 },
  { 0x1e, 0x3b, 0xf1, 0xbb, 0x7e, 0xfe, 0xf8, 0xc0 }
};

// ---------------------------------------------------- cavalinho

const int TAM_X_CAVALINHO = 17;
const int TAM_Y_CAVALINHO = 15;
const uint8_t cavalinho_dir[3][34] = {
   { 0x00, 0x90, 0x90, 0xd0, 0xd8, 0xc8, 0xd3, 0xfe, 0xff, 0xfe, 0xc3, 0xe0, 0xf0, 0xfc, 0x78, 0xfc, 0xe0,
     0x1f, 0x07, 0x01, 0x1f, 0x7f, 0x0f, 0x3f, 0x6f, 0x07, 0x07, 0x07, 0x07, 0x0f, 0x3f, 0x6e, 0x18, 0x30},
   { 0x00, 0x90, 0x98, 0xc8, 0xc8, 0xd8, 0xd3, 0xfe, 0xff, 0xfe, 0xc3, 0xe0, 0xf0, 0xfc, 0x78, 0xfc, 0xe0,
     0x0f, 0x07, 0x01, 0x7f, 0x0f, 0x7f, 0x1f, 0x07, 0x07, 0x07, 0x0f, 0x7f, 0x1f, 0x0f, 0x3c, 0x60, 0x00},
   { 0x00, 0x88, 0x88, 0xd8, 0xd0, 0xd0, 0xa6, 0xfc, 0xfe, 0xfc, 0x86, 0xe0, 0xf0, 0xf8, 0xf0, 0xf8, 0xc0,
     0x1f, 0x0f, 0x71, 0x1f, 0x4f, 0x7f, 0x0f, 0x07, 0x07, 0x07, 0x6f, 0x3f, 0x0f, 0x7f, 0x38, 0x01, 0x01}
};

const uint8_t cavalinho_esq[3][34] = {
   { 0xe0, 0xfc, 0x78, 0xfc, 0xf0, 0xe0, 0xc3, 0xfe, 0xff, 0xfe, 0xd3, 0xc8, 0xd8, 0xd0, 0x90, 0x90, 0x00,
     0x30, 0x18, 0x6e, 0x3f, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x6f, 0x3f, 0x0f, 0x7f, 0x1f, 0x01, 0x07, 0x1f},
   { 0xe0, 0xfc, 0x78, 0xfc, 0xf0, 0xe0, 0xc3, 0xfe, 0xff, 0xfe, 0xd3, 0xd8, 0xc8, 0xc8, 0x98, 0x90, 0x00,
     0x00, 0x60, 0x3c, 0x0f, 0x1f, 0x7f, 0x0f, 0x07, 0x07, 0x07, 0x1f, 0x7f, 0x0f, 0x7f, 0x01, 0x07, 0x0f},
   { 0xc0, 0xf8, 0xf0, 0xf8, 0xf0, 0xe0, 0x86, 0xfc, 0xfe, 0xfc, 0xa6, 0xd0, 0xd0, 0xd8, 0x88, 0x88, 0x00,
     0x01, 0x01, 0x38, 0x7f, 0x0f, 0x3f, 0x6f, 0x07, 0x07, 0x07, 0x0f, 0x7f, 0x4f, 0x1f, 0x71, 0x0f, 0x1f}
};

// -------------------------------------------- arqueiro

const int TAM_X_ARQUEIRA = 8;
const int TAM_Y_ARQUEIRA = 8;
const uint8_t arqueira_dir[2][8] = {
   { 0x10, 0xff, 0x7e, 0xfe, 0x10, 0x38, 0x44, 0x7c},
   { 0x08, 0xff, 0x7f, 0xff, 0x08, 0x1c, 0x22, 0x3e}
};

const uint8_t arqueira_esq[2][8] = {
   { 0x7c, 0x44, 0x38, 0x10, 0xfe, 0x7e, 0xff, 0x10},
   { 0x3e, 0x22, 0x1c, 0x08, 0xff, 0x7f, 0xff, 0x08}
};

// ------------------------------------------------ construtor

const int TAM_X_CONSTRUTOR = 8;
const int TAM_Y_CONSTRUTOR = 8;
const uint8_t construtor_dir[2][8] = { //os npcs virados pra direita nao foram usados ainda pq vai ter que gastar tempo programando
   { 0x14, 0xfe, 0x7e, 0xfe, 0x10, 0x16, 0x7e, 0x06},
   { 0x0a, 0xff, 0x7f, 0xff, 0x08, 0x0b, 0x3f, 0x03}
};
const uint8_t construtor_esq[2][8] = {
   { 0x06, 0x7e, 0x16, 0x10, 0xfe, 0x7e, 0xfe, 0x14},
   { 0x03, 0x3f, 0x0b, 0x08, 0xff, 0x7f, 0xff, 0x0a}
};

// ----------------- nativo
const int TAM_X_NATIVO = 7;
const int TAM_Y_NATIVO = 8;
const uint8_t nativo[2][7] = {
   { 0x30, 0x08, 0xff, 0x3f, 0xff, 0x08, 0x30},
   { 0x30, 0x08, 0xfe, 0x7e, 0xfe, 0x08, 0x30}
};

// ------------------------------------------------ textos

//barrinha de carregamento
const int TAM_X_BARRINHA = 19;
const int TAM_Y_BARRINHA = 6;
const uint8_t barrinha[3][19] = {
   { 0x3f, 0x21, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x3f},
   { 0x3f, 0x21, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x3f},
   { 0x3f, 0x21, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x21, 0x3f}
};

//texto inicial "Many failed, this king will do better"
const int TAM_X_INICIAL = 78;
const int TAM_Y_INICIAL = 14;
const uint8_t inicial[] = {
  0x3e, 0x06, 0x18, 0x06, 0x3e, 0x00, 0x12, 0x2a, 0x3c, 0x00, 0x3c, 0x08, 0x04, 0x38, 0x00, 0x5c, 0x50, 0x3c, 0x00, 0x00, 0x00, 0x04, 0x3c, 0x0a, 0x00, 0x12, 0x2a, 0x3c, 0x00, 0x3d, 0x00, 0x3f, 0x00, 0x1c, 0x2a, 0x22, 0x00, 0x30, 0x28, 0x3e, 0x00, 0x20, 0x60, 0x00, 0x00, 0x00, 0x1e, 0x24, 0x10, 0x00, 0x3e, 0x08, 0x04, 0x38, 0x00, 0x3d, 0x00, 0x2c, 0x2a, 0x3a, 0x00, 0x00, 0x00, 0x3f, 0x08, 0x14, 0x23, 0x00, 0x3d, 0x00, 0x3c, 0x08, 0x04, 0x38, 0x00, 0x98, 0x54, 0xec,
  0x1e, 0x20, 0x18, 0x20, 0x1e, 0x00, 0x3d, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x30, 0x28, 0x3e, 0x00, 0x38, 0x24, 0x18, 0x00, 0x00, 0x00, 0x3e, 0x28, 0x30, 0x00, 0x1c, 0x2a, 0x22, 0x00, 0x1e, 0x24, 0x10, 0x00, 0x1e, 0x24, 0x10, 0x00, 0x1c, 0x2a, 0x22, 0x00, 0x3e, 0x04, 0x02, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00
};

//"Hold swt to skip"
const int TAM_X_SKIP = 70;
const int TAM_Y_SKIP = 8;
const uint8_t skip[] = {
  0x3e, 0x08, 0x3e, 0x00, 0x38, 0x24, 0x18, 0x00, 0x3f, 0x00, 0x30, 0x28, 0x3e, 0x00, 0x00, 0x00, 0x30, 0x28, 0x3e, 0x00, 0x38, 0x24, 0x18, 0x00, 0x1c, 0x20, 0x18, 0x20, 0x1c, 0x00, 0x3c, 0x08, 0x04, 0x38, 0x00, 0x00, 0x00, 0x14, 0x3e, 0x22, 0x2a, 0x22, 0x3e, 0x14, 0x00, 0x00, 0x00, 0x1e, 0x24, 0x00, 0x38, 0x24, 0x18, 0x00, 0x00, 0x00, 0x2c, 0x2a, 0x3a, 0x00, 0x3e, 0x08, 0x14, 0x00, 0x3a, 0x00, 0xf8, 0x28, 0x28, 0x10
};

const int TAM_X_LIGHTAFIRE = 84;
const int TAM_Y_LIGHTAFIRE = 15;
const uint8_t lightafire[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x10, 0x7c, 0x00, 0x70, 0x48, 0x30, 0x00, 0x7e, 0x00, 0x60, 0x50, 0x7c, 0x00, 0x00, 0x00, 0x60, 0x50, 0x7c, 0x00, 0x70, 0x48, 0x30, 0x00, 0x38, 0x40, 0x30, 0x40, 0x38, 0x00, 0x78, 0x10, 0x08, 0x70, 0x00, 0x00, 0x00, 0x28, 0x7c, 0x44, 0x54, 0x44, 0x7c, 0x28, 0x00, 0x00, 0x00, 0x3c, 0x48, 0x00, 0x70, 0x48, 0x30, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x74, 0x00, 0x4c, 0xaa, 0x76, 0x00, 0x7c, 0x10, 0x08, 0x70, 0x00, 0x3c, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x2a, 0x3c, 0x00, 0x00, 0x00, 0x04, 0x3c, 0x0a, 0x00, 0x3a, 0x00, 0x3e, 0x04, 0x02, 0x00, 0x1c, 0x2a, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
