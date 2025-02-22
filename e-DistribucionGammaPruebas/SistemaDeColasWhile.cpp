#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h"  /* Generador de números aleatorios */

#define LIMITE_COLA 100  /* Capacidad máxima de la cola */
#define OCUPADO      1  /* Indicador de Servidor Ocupado */
#define LIBRE      0  /* Indicador de Servidor Libre */
#define NUM_LAMBDA 20 /* Número de valores de λ1 a probar */

int   sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos,
      num_entra_cola, estado_servidor, m;
float area_num_entra_cola, area_estado_servidor, tiempo_simulacion, 
      tiempo_llegada[LIMITE_COLA + 1], tiempo_ultimo_evento, tiempo_sig_evento[101], 
      total_de_esperas, tiempo_total_con_cola;
float alpha1, lambda1, alpha2, lambda2; // Parámetros de la distribución Gamma

FILE  *resultados_lambda;

void  inicializar(void);
void  controltiempo(void);
void  llegada(void);
void  salida(void);
void  reportes(void);
void  actualizar_estad_prom_tiempo(void);
float gamma_rand(float alpha, float lambda);

int main(void)  /* Función principal */
{
    /* Abre el archivo de salida */
    resultados_lambda = fopen("resultados_lambda.txt", "w");
    fprintf(resultados_lambda, "λ1  P_cola(λ1)\n");

    /* Fijamos los parámetros fijos */
    alpha1 = 3.5;
    alpha2 = 4.3;
    lambda2 = 7.5;
    m = 10; /* Número de servidores */
    num_esperas_requerido = 1000; /* Número de clientes a simular */

    /* Probamos varios valores de lambda1 */
    for (lambda1 = 0.5; lambda1 <= 20.0; lambda1 += 0.5) {
        /* Inicializa la simulación */
        inicializar();

        /* Corre la simulación */
        while (num_clientes_espera < num_esperas_requerido) {
            controltiempo();
            actualizar_estad_prom_tiempo();

            switch (sig_tipo_evento) {
                case 1:
                    llegada();
                    break;
                default:
                    salida();
                    break;
            }
        }

        /* Calcula la probabilidad de que haya cola */
        float P_cola = tiempo_total_con_cola / tiempo_simulacion;

        /* Guarda el resultado */
        fprintf(resultados_lambda, "%.2f  %.3f\n", lambda1, P_cola);
        printf("λ1 = %.2f, P_cola = %.3f\n", lambda1, P_cola);
    }

    fclose(resultados_lambda);
    return 0;
}

void inicializar(void)  
{
    tiempo_simulacion = 0.0;
    estado_servidor   = LIBRE;
    num_entra_cola    = 0;
    tiempo_ultimo_evento = 0.0;
    num_clientes_espera  = 0;
    total_de_esperas    = 0.0;
    area_num_entra_cola = 0.0;
    area_estado_servidor = 0.0;
    tiempo_total_con_cola = 0.0;

    /* Inicializa la lista de eventos */
    tiempo_sig_evento[1] = tiempo_simulacion + gamma_rand(alpha1, lambda1);
    for (int i = 2; i <= m + 1; i++) {
        tiempo_sig_evento[i] = 1.0e+30;
    }
}

void controltiempo(void)  
{
    float min_tiempo_sig_evento = 1.0e+29;
    sig_tipo_evento = 0;

    for (int i = 1; i <= m + 1; i++) {
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento) {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento = i;
        }
    }

    tiempo_simulacion = min_tiempo_sig_evento;
}

void llegada(void)  
{
    tiempo_sig_evento[1] = tiempo_simulacion + gamma_rand(alpha1, lambda1);

    int servidores_ocupados = 0;
    for (int i = 2; i <= m + 1; i++) {
        if (tiempo_sig_evento[i] != 1.0e+30) {
            servidores_ocupados++;
        }
    }

    if (servidores_ocupados < m) {
        for (int i = 2; i <= m + 1; i++) {
            if (tiempo_sig_evento[i] == 1.0e+30) {
                tiempo_sig_evento[i] = tiempo_simulacion + gamma_rand(alpha2, lambda2);
                break;
            }
        }
    } else {
        ++num_entra_cola;
        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    }
}

void salida(void)  
{
    int servidor = sig_tipo_evento;
    
    if (num_entra_cola == 0) {
        tiempo_sig_evento[servidor] = 1.0e+30;
    } else {
        --num_entra_cola;
        total_de_esperas += tiempo_simulacion - tiempo_llegada[1];
        ++num_clientes_espera;
        tiempo_sig_evento[servidor] = tiempo_simulacion + gamma_rand(alpha2, lambda2);
        
        for (int i = 1; i <= num_entra_cola; ++i) {
            tiempo_llegada[i] = tiempo_llegada[i + 1];
        }
    }
}

void actualizar_estad_prom_tiempo(void) 
{
    float time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;
    area_num_entra_cola += num_entra_cola * time_since_last_event;
    area_estado_servidor += (num_entra_cola > 0) * time_since_last_event;
    if (num_entra_cola > 0) {
        tiempo_total_con_cola += time_since_last_event;
    }
}

float gamma_rand(float alpha, float lambda) 
{
    float sum = 0.0;
    for (int i = 0; i < (int)alpha; ++i) {
        sum += -log(lcgrand(1)) / lambda;
    }
    return sum;
}
