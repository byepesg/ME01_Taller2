#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include "lcgrand.h"

#define LIMITE_COLA 100
#define OCUPADO 1
#define LIBRE 0

int sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos, num_entra_cola, estado_servidor;
float area_num_entra_cola, area_estado_servidor, media_entre_llegadas, media_atencion, tiempo_simulacion, tiempo_ultimo_evento;
std::vector<float> tiempo_llegada;
std::vector<float> tiempo_sig_evento;
float total_de_esperas;
std::ifstream parametros;
std::ofstream resultados;

// Parámetros Gamma
float alpha1, lambda1, alpha2, lambda2;
int m; // Número de servidores

// Función para generar números aleatorios con distribución Gamma
float gamma_rand(float alpha, float lambda) {
    float sum = 0.0;
    for (int i = 0; i < alpha; ++i) {
        sum += -log(lcgrand(1)) / lambda;
    }
    return sum;
}

void inicializar(void);
void controltiempo(void);
void llegada(void);
void salida(void);
void reportes(void);
void actualizar_estad_prom_tiempo(void);
float gamma_rand(float alpha, float lambda);  // Declaración de gamma_rand

int main(void) {
    parametros.open("param.txt");
    resultados.open("result.txt");

    // Leer parámetros
    parametros >> alpha1 >> lambda1 >> alpha2 >> lambda2 >> m >> num_esperas_requerido;

    // Inicializar variables
    tiempo_llegada.resize(LIMITE_COLA + 1);
    tiempo_sig_evento.resize(m + 2); // Eventos: llegada + m servidores

    inicializar();

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

    reportes();

    parametros.close();
    resultados.close();
    return 0;
}

void inicializar(void) {
    tiempo_simulacion = 0.0;
    estado_servidor = LIBRE;
    num_entra_cola = 0;
    tiempo_ultimo_evento = 0.0;
    num_clientes_espera = 0;
    total_de_esperas = 0.0;
    area_num_entra_cola = 0.0;
    area_estado_servidor = 0.0;

    // Programar la primera llegada
    tiempo_sig_evento[1] = tiempo_simulacion + gamma_rand(alpha1, lambda1);
    for (int i = 2; i <= m + 1; ++i) {
        tiempo_sig_evento[i] = 1.0e+30; // Servidores libres
    }
}

void llegada(void) {
    float espera;
    tiempo_sig_evento[1] = tiempo_simulacion + gamma_rand(alpha1, lambda1);

    if (estado_servidor == OCUPADO) {
        ++num_entra_cola;
        if (num_entra_cola > LIMITE_COLA) {
            resultados << "\nDesbordamiento de la cola en el tiempo " << tiempo_simulacion;
            exit(2);
        }
        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    } else {
        espera = 0.0;
        total_de_esperas += espera;
        ++num_clientes_espera;
        estado_servidor = OCUPADO;
        tiempo_sig_evento[2] = tiempo_simulacion + gamma_rand(alpha2, lambda2);
    }
}

void salida(void) {
    float espera;
    if (num_entra_cola == 0) {
        estado_servidor = LIBRE;
        tiempo_sig_evento[2] = 1.0e+30;
    } else {
        --num_entra_cola;
        espera = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;
        ++num_clientes_espera;
        tiempo_sig_evento[2] = tiempo_simulacion + gamma_rand(alpha2, lambda2);
        for (int i = 1; i <= num_entra_cola; ++i) {
            tiempo_llegada[i] = tiempo_llegada[i + 1];
        }
    }
}

void reportes(void) {
    resultados << "\n\nEspera promedio en la cola: " << total_de_esperas / num_clientes_espera << " minutos\n";
    resultados << "Número promedio en cola: " << area_num_entra_cola / tiempo_simulacion << "\n";
    resultados << "Uso del servidor: " << area_estado_servidor / tiempo_simulacion << "\n";
    resultados << "Tiempo de terminación de la simulación: " << tiempo_simulacion << " minutos\n";
}

void actualizar_estad_prom_tiempo(void) {
    float time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;
    area_num_entra_cola += num_entra_cola * time_since_last_event;
    area_estado_servidor += estado_servidor * time_since_last_event;
}