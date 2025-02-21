#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.cpp"

#define LIMITE_COLA 100  /* Capacidad maxima de la cola */
#define OCUPADO      1  /* Indicador de Servidor Ocupado */
#define LIBRE      0  /* Indicador de Servidor Libre */

int sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos,
    num_entra_cola, estado_servidor, num_servidores;
float area_num_entra_cola, area_estado_servidor, alpha1, lambda1,
      alpha2, lambda2, tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1],
      tiempo_ultimo_evento, tiempo_sig_evento[3], total_de_esperas;
FILE *parametros, *resultados;

void inicializar(void);
void controltiempo(void);
void llegada(void);
void salida(void);
void reportes(void);
void actualizar_estad_prom_tiempo(void);
float gamma_dist(float alpha, float lambda);
float calcular_probabilidad_cola(void);

void inicializar(void) {
    tiempo_simulacion = 0.0;
    estado_servidor = LIBRE;
    num_entra_cola = 0;
    tiempo_ultimo_evento = 0.0;
    num_clientes_espera = 0;
    total_de_esperas = 0.0;
    area_num_entra_cola = 0.0;
    area_estado_servidor = 0.0;
    tiempo_sig_evento[1] = tiempo_simulacion + gamma_dist(alpha1, lambda1);
    tiempo_sig_evento[2] = 1.0e+30;
}

void llegada(void) {
    tiempo_sig_evento[1] = tiempo_simulacion + gamma_dist(alpha1, lambda1);
    if (estado_servidor < num_servidores) {
        total_de_esperas += 0.0;
        ++num_clientes_espera;
        ++estado_servidor;
        tiempo_sig_evento[2] = tiempo_simulacion + gamma_dist(alpha2, lambda2);
    } else {
        ++num_entra_cola;
        if (num_entra_cola > LIMITE_COLA) {
            fprintf(resultados, "\nError: Cola desbordada en %f", tiempo_simulacion);
            exit(2);
        }
        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    }
}

void salida(void) {
    int i;
    float espera;
    if (num_entra_cola == 0) {
        --estado_servidor;
        if (estado_servidor == 0) {
            tiempo_sig_evento[2] = 1.0e+30;
        }
    } else {
        --num_entra_cola;
        espera = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;
        ++num_clientes_espera;
        tiempo_sig_evento[2] = tiempo_simulacion + gamma_dist(alpha2, lambda2);
        for (i = 1; i <= num_entra_cola; ++i)
            tiempo_llegada[i] = tiempo_llegada[i + 1];
    }
}

float calcular_probabilidad_cola(void) {
    return (float)num_entra_cola / (num_clientes_espera + num_entra_cola);
}

void reportes(void) {
    fprintf(resultados, "\nEspera promedio en la cola: %.3f minutos\n", total_de_esperas / num_clientes_espera);
    fprintf(resultados, "Numero promedio en cola: %.3f\n", area_num_entra_cola / tiempo_simulacion);
    fprintf(resultados, "Uso del servidor: %.3f\n", area_estado_servidor / tiempo_simulacion);
    fprintf(resultados, "Tiempo de terminacion de la simulacion: %.3f minutos\n", tiempo_simulacion);
    fprintf(resultados, "Probabilidad de que se forme cola: %.3f\n", calcular_probabilidad_cola());
}
