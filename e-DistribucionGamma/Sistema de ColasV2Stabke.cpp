#include <iostream>
#include <fstream>
#include <cmath>
#include <random>
#include "lcgrand.h"

#define LIMITE_COLA 100  
#define OCUPADO 1  
#define LIBRE 0  

int sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos,
    num_entra_cola, estado_servidor, servidores_ocupados, m;
float area_num_entra_cola, area_estado_servidor, media_entre_llegadas, media_atencion,
      tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1], tiempo_ultimo_evento, 
      tiempo_sig_evento[3], total_de_esperas, tiempo_cola;
std::ofstream resultados;
std::ifstream parametros;

// Generador aleatorio para distribución Gamma
std::default_random_engine generator;
std::gamma_distribution<float> gamma_entre_llegadas(3.5, 1.0);
std::gamma_distribution<float> gamma_atencion(4.3, 1.0/7.5);

void inicializar();
void controltiempo();
void llegada();
void salida();
void reportes();
void actualizar_estad_prom_tiempo();
float generar_gamma(float alpha, float lambda);

int main() {
    parametros.open("param.txt");
    resultados.open("result.txt");

    if (!parametros.is_open()) {
        std::cerr << "Error: No se pudo abrir param.txt\n";
        return 1;
    }
    if (!resultados.is_open()) {
        std::cerr << "Error: No se pudo abrir result.txt\n";
        return 1;
    }

    num_eventos = 2;

    parametros >> media_entre_llegadas >> media_atencion >> num_esperas_requerido >> m;

    resultados << "Sistema de Colas (Gamma/Gamma/m)\n\n";
    resultados << "Tiempo promedio de llegada: " << media_entre_llegadas << " minutos\n";
    resultados << "Tiempo promedio de atencion: " << media_atencion << " minutos\n";
    resultados << "Numero de clientes: " << num_esperas_requerido << "\n";
    resultados << "Numero de servidores: " << m << "\n\n";

    inicializar();

    while (num_clientes_espera < num_esperas_requerido) {
        controltiempo();
        actualizar_estad_prom_tiempo();

        switch (sig_tipo_evento) {
            case 1: llegada(); break;
            case 2: salida(); break;
        }
    }

    reportes();
    parametros.close();
    resultados.close();

    return 0;
}

void inicializar() {
    tiempo_simulacion = 0.0;
    estado_servidor = LIBRE;
    servidores_ocupados = 0;
    num_entra_cola = 0;
    tiempo_ultimo_evento = 0.0;
    tiempo_cola = 0.0;

    num_clientes_espera = 0;
    total_de_esperas = 0.0;
    area_num_entra_cola = 0.0;
    area_estado_servidor = 0.0;

    tiempo_sig_evento[1] = tiempo_simulacion + generar_gamma(3.5, media_entre_llegadas);
    tiempo_sig_evento[2] = 1.0e+30;  // Inicia sin salidas programadas
}

void controltiempo() {
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
        std::cerr << "Error: La lista de eventos está vacía en el tiempo " << tiempo_simulacion << "\n";
        exit(1);
    }

    tiempo_simulacion = min_tiempo_sig_evento;
}

void llegada() {
    tiempo_sig_evento[1] = tiempo_simulacion + generar_gamma(3.5, media_entre_llegadas);

    if (servidores_ocupados < m) {
        // Si hay servidores disponibles, el cliente se atiende inmediatamente.
        servidores_ocupados++;
        tiempo_sig_evento[2] = tiempo_simulacion + generar_gamma(4.3, media_atencion);
    } else {
        // Si la cola está llena, ignoramos la llegada y solo registramos el evento.
        if (num_entra_cola >= LIMITE_COLA) {
            resultados << "⚠️ Advertencia: La cola alcanzó su límite en el tiempo " << tiempo_simulacion << " ⚠️\n";
            return;
        }
        // Si hay espacio, el cliente entra en la cola.
        num_entra_cola++;
        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    }
}


void salida() {
    servidores_ocupados--;

    if (num_entra_cola > 0) {
        num_entra_cola--;
        servidores_ocupados++;
        tiempo_sig_evento[2] = tiempo_simulacion + generar_gamma(4.3, media_atencion);
    } else {
        tiempo_sig_evento[2] = 1.0e+30;  // No hay clientes en espera
    }
}

void reportes() {
    if (tiempo_simulacion > 0) {
        resultados << "\nProbabilidad de cola P_cola: " << tiempo_cola / tiempo_simulacion << "\n";
    } else {
        resultados << "\nError: Tiempo de simulación = 0\n";
    }
}

void actualizar_estad_prom_tiempo() {
    float time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;

    if (num_entra_cola > 0) {
        tiempo_cola += time_since_last_event;
    }
}

float generar_gamma(float alpha, float lambda) {
    std::gamma_distribution<float> distribucion(alpha, 1.0 / lambda);
    return distribucion(generator);
}
