#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h"  // Incluye solo el header

/* ------------------------------------------------------------------
   Definiciones y parámetros del simulador
   ------------------------------------------------------------------ */
#define LIMITE_COLA 100
#define OCUPADO     1
#define LIBRE       0
#define MAX_SERVIDORES  20  /* Capacidad máxima de servidores para dimensionar arreglos */

/* Variables globales de simulación */
int  num_eventos;           /* Número de tipos de evento: 1 (llegada) + m (salidas) */
int  m;                     /* Número de servidores (se lee desde paramGamma.txt) */
int  sig_tipo_evento;       /* 0: llegada, 1..m: salida en el servidor correspondiente */

int  num_en_cola;           /* Número actual de clientes en la cola */
int  estado_servidor[MAX_SERVIDORES]; /* Estado de cada servidor: OCUPADO o LIBRE */

int  num_esperas_requerido;     /* Número de clientes a simular */
int  num_clientes_espera;       /* Cantidad de clientes atendidos */
int  total_clientes_llegados;   /* Total de clientes que han llegado (para calcular Pcola) */
int  clientes_que_esperan;      /* Se incrementa cada vez que un cliente llega y no encuentra un servidor libre */

float alpha1, lambda1;    /* Parámetros de la Gamma para llegadas (λ₁ se variará) */
float alpha2, lambda2;    /* Parámetros de la Gamma para servicios */

double tiempo_simulacion;  /* Reloj de la simulación */
double tiempo_ultimo_evento;
double tiempo_sig_evento[MAX_SERVIDORES + 1]; /* [0] para llegada, [1..m] para salidas */

double total_de_esperas;   /* Suma de los tiempos de espera en la cola */
double area_num_en_cola;   /* Área bajo la curva del número de clientes en cola */
double area_servidores_ocup; /* Área acumulada de servidores ocupados */

double tiempo_llegada[LIMITE_COLA + 1];  /* Tiempo de llegada de cada cliente en cola */

/* Archivos */
FILE  *resultados;

/* Prototipos de funciones de simulación */
void inicializar(void);
void controltiempo(void);
void llegada(void);
void salida(int servidor_idx);
void actualizar_estad_prom_tiempo(void);
double random_gamma(double alpha, double rate);

/* Función que ejecuta la simulación (hasta atender N clientes) */
void run_simulation(void)
{
    inicializar();
    while (num_clientes_espera < num_esperas_requerido) {
        controltiempo();
        actualizar_estad_prom_tiempo();
        if (sig_tipo_evento == 0) {
            llegada();
        } else {
            salida(sig_tipo_evento);
        }
    }
}

/* ------------------------------------------------------------------
   Funciones de simulación
   ------------------------------------------------------------------ */
void inicializar(void)
{
    tiempo_simulacion   = 0.0;
    tiempo_ultimo_evento = 0.0;

    for (int i = 0; i < m; i++){
        estado_servidor[i] = LIBRE;
    }

    num_en_cola = 0;
    num_clientes_espera = 0;
    total_de_esperas = 0.0;
    area_num_en_cola = 0.0;
    area_servidores_ocup = 0.0;
    total_clientes_llegados = 0;
    clientes_que_esperan = 0;

    double primer_llegada = random_gamma(alpha1, lambda1);
    tiempo_sig_evento[0] = tiempo_simulacion + primer_llegada;
    for (int i = 1; i <= m; i++){
        tiempo_sig_evento[i] = 1.0e+30;
    }
    // Mensaje de depuración
    // printf("DEBUG: inicializar -> primera llegada en t=%.2f\n", tiempo_sig_evento[0]);
}

void controltiempo(void)
{
    double min_tiempo = 1.0e+29;
    sig_tipo_evento = -1;
    for (int i = 0; i <= m; i++){
        if (tiempo_sig_evento[i] < min_tiempo) {
            min_tiempo = tiempo_sig_evento[i];
            sig_tipo_evento = i;
        }
    }
    if (sig_tipo_evento == -1) {
        // printf("DEBUG: Lista de eventos vacía en t=%.2f\n", tiempo_simulacion);
        exit(1);
    }
    tiempo_simulacion = min_tiempo;
    // Debug: imprimir evento
    // printf("DEBUG: controltiempo -> sig_tipo_evento=%d, t=%.2f\n", sig_tipo_evento, tiempo_simulacion);
}

void llegada(void)
{
    double prox_llegada = random_gamma(alpha1, lambda1);
    tiempo_sig_evento[0] = tiempo_simulacion + prox_llegada;
    total_clientes_llegados++;

    // printf("DEBUG: LLEGADA -> t=%.2f, total_lleg=%d\n", tiempo_simulacion, total_clientes_llegados);

    int servidor_libre = -1;
    for (int i = 0; i < m; i++){
        if (estado_servidor[i] == LIBRE) {
            servidor_libre = i;
            break;
        }
    }

    if (servidor_libre == -1) {
        num_en_cola++;
        if (num_en_cola > LIMITE_COLA) {
            // printf("DEBUG: Desbordamiento a t=%.2f\n", tiempo_simulacion);
            exit(2);
        }
        tiempo_llegada[num_en_cola] = tiempo_simulacion;
        // printf("DEBUG: LLEGADA -> Servidores ocupados. Cliente en cola. num_en_cola=%d\n", num_en_cola);
        /* 
         * Contamos aquí que el cliente tuvo que esperar 
         * (porque no encontró un servidor libre). 
         */
        clientes_que_esperan++;  
    } else {
        estado_servidor[servidor_libre] = OCUPADO;
        num_clientes_espera++;
        double servicio = random_gamma(alpha2, lambda2);
        tiempo_sig_evento[servidor_libre + 1] = tiempo_simulacion + servicio;
        // printf("DEBUG: LLEGADA -> Se atiende inmediato en servidor=%d, fin_serv=%.2f\n",
        //        servidor_libre, tiempo_sig_evento[servidor_libre + 1]);
    }
}

void salida(int i)
{
    int serv_idx = i - 1;
    // printf("DEBUG: SALIDA -> servidor=%d, t=%.2f\n", serv_idx, tiempo_simulacion);
    if (num_en_cola == 0) {
        estado_servidor[serv_idx] = LIBRE;
        tiempo_sig_evento[i] = 1.0e+30;
        // printf("DEBUG: SALIDA -> Cola vacía, servidor %d queda LIBRE\n", serv_idx);
    } else {
        num_en_cola--;
        double espera = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;
        num_clientes_espera++;

        /* 
         * ANTES: aquí se hacía otro "clientes_que_esperan++" si espera>0, 
         * lo cual provocaba doble conteo. Lo eliminamos para no sobrecontar. 
         */

        /* Ajustar tiempos de llegada (corrimiento del array) */
        for (int j = 1; j <= num_en_cola; j++){
            tiempo_llegada[j] = tiempo_llegada[j+1];
        }
        double servicio = random_gamma(alpha2, lambda2);
        tiempo_sig_evento[i] = tiempo_simulacion + servicio;
        // printf("DEBUG: SALIDA -> Siguiente de la cola entra a servidor=%d, espera=%.2f, fin_serv=%.2f\n",
        //        serv_idx, espera, tiempo_sig_evento[i]);
    }
}

void actualizar_estad_prom_tiempo(void)
{
    double dt = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;
    area_num_en_cola += num_en_cola * dt;
    int ocup = 0;
    for (int i = 0; i < m; i++){
        if (estado_servidor[i] == OCUPADO)
            ocup++;
    }
    area_servidores_ocup += ocup * dt;
}

/* ------------------------------------------------------------------
   random_gamma: genera una variable aleatoria con distribución Gamma(alpha, rate)
   (Método de Marsaglia & Tsang)
   ------------------------------------------------------------------ */
double random_gamma(double alpha, double rate)
{
    if (alpha < 1.0) {
        double u = lcgrand(1);
        return random_gamma(alpha + 1.0, rate) * pow(u, 1.0/alpha);
    }
    double d = alpha - 1.0/3.0;
    double c = 1.0 / sqrt(9.0*d);
    while (1) {
        double x, v, u;
        double r1 = lcgrand(1);
        double r2 = lcgrand(1);
        x = sqrt(-2.0 * log(r1)) * cos(2.0 * M_PI * r2);
        v = 1.0 + c * x;
        if (v <= 0)
            continue;
        v = v * v * v;
        u = lcgrand(1);
        if (u < 1.0 - 0.0331 * (x * x) * (x * x))
            return d * v / rate;
        if (log(u) < 0.5 * x * x + d * (1.0 - v + log(v)))
            return d * v / rate;
    }
}

/* ------------------------------------------------------------------
   main (automatizado para múltiples valores de lambda1)
   ------------------------------------------------------------------ */
int main(void)
{
    /* Primero, leemos los parámetros fijos (excepto lambda1) desde paramGamma.txt */
    FILE* param = fopen("paramGamma.txt", "r");
    if (!param) {
        printf("ERROR: No se pudo abrir paramGamma.txt\n");
        return 1;
    }
    /* Leemos: alpha1, dummy (valor ignorado para lambda1), alpha2, lambda2, m, num_esperas_requerido */
    float dummy;
    fscanf(param, "%f %f %f %f %d %d", &alpha1, &dummy, &alpha2, &lambda2, &m, &num_esperas_requerido);
    fclose(param);

    /* Abrir el archivo de valores de lambda1 */
    FILE* infile = fopen("lambdas.txt", "r");
    if (!infile) {
        printf("ERROR: No se pudo abrir lambdas.txt\n");
        return 1;
    }

    /* Abrir el archivo CSV para guardar resultados */
    FILE* csv = fopen("resultados.csv", "w");
    if (!csv) {
        printf("ERROR: No se pudo crear resultados.csv\n");
        return 1;
    }
    /* Escribir encabezado en CSV */
    fprintf(csv, "lambda1,Demora_promedio,Clientes_promedio,Utilizacion,Pcola,Tiempo_total\n");

    /* Para cada valor de lambda1 en lambdas.txt */
    while (fscanf(infile, "%f", &lambda1) == 1) {
        // printf("DEBUG: Ejecutando simulación para lambda1=%.2f\n", lambda1);

        /* Reiniciar acumuladores globales (se hace en inicializar) y correr simulación */
        run_simulation();

        /* Calcular métricas */
        double demora_promedio = (num_clientes_espera > 0) ? (total_de_esperas / num_clientes_espera) : 0.0;
        double clientes_promedio = (tiempo_simulacion > 0.0) ? (area_num_en_cola / tiempo_simulacion) : 0.0;
        double utilizacion = (tiempo_simulacion > 0.0) ? (area_servidores_ocup / (m * tiempo_simulacion)) : 0.0;
        double pcola = (total_clientes_llegados > 0)
                           ? ((double)clientes_que_esperan / total_clientes_llegados)
                           : 0.0;
        double tiempo_total = tiempo_simulacion;

        /* Escribir línea en CSV */
        fprintf(csv, "%.2f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                lambda1, demora_promedio, clientes_promedio, utilizacion, pcola, tiempo_total);
        /* Imprimir en consola (opcional) */
        // printf("DEBUG: lambda1=%.2f: Delay=%.3f, ColaProm=%.3f, Util=%.3f, Pcola=%.3f, Tiempo=%.3f\n",
        //        lambda1, demora_promedio, clientes_promedio, utilizacion, pcola, tiempo_total);
    }

    fclose(infile);
    fclose(csv);
    return 0;
}
