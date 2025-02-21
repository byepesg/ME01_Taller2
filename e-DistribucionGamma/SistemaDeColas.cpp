#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h"

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

int main(void) {
    parametros  = fopen("param.txt", "r");
    resultados = fopen("result.txt", "w");

    num_eventos = 2;
    fscanf(parametros, "%f %f %f %f %d %d", &alpha1, &lambda1, &alpha2, &lambda2, &num_esperas_requerido, &num_servidores);
    fprintf(resultados, "Sistema de Colas Gamma/Gamma/m\n\n");
    inicializar();

    while (num_clientes_espera < num_esperas_requerido) {
        controltiempo();
        actualizar_estad_prom_tiempo();
        switch (sig_tipo_evento) {
            case 1:
                llegada();
                break;
            case 2:
                salida();
                break;
        }
    }
    reportes();
    fclose(parametros);
    fclose(resultados);
    return 0;
}

float gamma_dist(float alpha, float lambda) {
    float d, c, x, v, u;
    d = alpha - 1.0/3.0;
    c = 1.0 / sqrt(9.0 * d);
    do {
        do {
            x = lcgrand(1);
            v = pow(1.0 + c * x, 3);
        } while (v <= 0);
        u = lcgrand(2);
    } while (u > (1.0 - 0.331 * pow(x, 4)) && log(u) > 0.5 * x * x + d * (1 - v + log(v)));
    return d * v / lambda;
}

void controltiempo(void) {
    int i;
    float min_tiempo_sig_evento = 1.0e+29;
    sig_tipo_evento = 0;
    for (i = 1; i <= num_eventos; ++i) {
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento) {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento = i;
        }
    }
    if (sig_tipo_evento == 0) {
        fprintf(resultados, "\nError: Lista de eventos vacía en %f", tiempo_simulacion);
        exit(1);
    }
    tiempo_simulacion = min_tiempo_sig_evento;
}

void actualizar_estad_prom_tiempo(void) {
    float time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;
    area_num_entra_cola += num_entra_cola * time_since_last_event;
    area_estado_servidor += estado_servidor * time_since_last_event;
}

void reportes(void) {
    fprintf(resultados, "\nEspera promedio en la cola: %.3f minutos\n", total_de_esperas / num_clientes_espera);
    fprintf(resultados, "Numero promedio en cola: %.3f\n", area_num_entra_cola / tiempo_simulacion);
    fprintf(resultados, "Uso del servidor: %.3f\n", area_estado_servidor / tiempo_simulacion);
    fprintf(resultados, "Tiempo de terminacion de la simulacion: %.3f minutos\n", tiempo_simulacion);
}
