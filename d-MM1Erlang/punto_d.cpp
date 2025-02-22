

/*

PARA USAR ESTO

1. g++ -o punto_e punto_e.cpp lcgrand.c -lm
2. ./punto_e
3. revisar result.txt

*/



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h"  

#define LIMITE_COLA    1000         /* Tamaño máximo de la cola (para Erlang C) */
#define INFINITY_TIME  1.0e+30      /* Representa "infinito" */
#define EVENTO_LLEGADA  1
#define EVENTO_SALIDA   2

/* Variables globales */
int tipo_simulacion;              // 0 = Erlang B; 1 = Erlang C
int num_servidores;               // Número de servidores
double t_interarribo_promedio;    // Tiempo medio entre llegadas
double t_servicio_promedio;       // Tiempo medio de servicio
int num_llegadas_requeridas;      // Número de llegadas a simular

int total_llegadas;             // Total de llegadas procesadas
int llamadas_atendidas;         // Llamadas atendidas
int llamadas_bloqueadas;        // (Erlang B) Llamadas bloqueadas
int llamadas_esperaron;         // (Erlang C) Llamadas que esperaron
double tiempo_total_espera;     // Tiempo total de espera
double tiempo_simulacion;       // Reloj de simulación
double area_ocupada;            // Tiempo acumulado de servidores ocupados

/* Para la programación de eventos (Erlang C) */
double proxima_llegada;         // Tiempo de la próxima llegada
double *proxima_salida;         // Arreglo de tiempos de próxima salida (uno por servidor)

/* Para Erlang B */
int ocupados_actuales;

FILE *parametros, *resultados;

/* Prototipos de funciones */
void simularErlangB(void);
void simularErlangC(void);
void reporte(void);
double exponencial(double media);
double uniforme(void);
double erlangB(double A, int N);
double erlangC(double A, int s);

int main(void)
{
    /* Abrir archivos de entrada y salida */
    parametros  = fopen("param.txt",  "r");
    resultados = fopen("result.txt", "w");
    if (parametros == NULL) {
        fprintf(stderr, "Error al abrir param.txt\n");
        exit(1);
    }
    
    /* Lectura de parámetros desde param.txt:
       t_interarribo_promedio t_servicio_promedio num_llegadas_requeridas num_servidores tipo_simulacion */
    fscanf(parametros, "%lf %lf %d %d %d",
           &t_interarribo_promedio, &t_servicio_promedio, &num_llegadas_requeridas, &num_servidores, &tipo_simulacion);
    
    fprintf(resultados, "Simulador de Colas para Fórmulas de Erlang\n\n");
    fprintf(resultados, "Tiempo Medio entre Llegadas: %f\n", t_interarribo_promedio);
    fprintf(resultados, "Tiempo Medio de Servicio:      %f\n", t_servicio_promedio);
    fprintf(resultados, "Número de Llegadas:           %d\n", num_llegadas_requeridas);
    fprintf(resultados, "Número de Servidores (N):     %d\n", num_servidores);
    if (tipo_simulacion == 0)
        fprintf(resultados, "Tipo de Simulación: Erlang B (Sistema de Pérdida)\n\n");
    else
        fprintf(resultados, "Tipo de Simulación: Erlang C (Sistema de Colas)\n\n");
    
    /* No se necesita srand() ya que usamos lcgrand */
    
    /* Inicializar contadores globales */
    total_llegadas = 0;
    llamadas_atendidas = 0;
    llamadas_bloqueadas = 0;
    llamadas_esperaron = 0;
    tiempo_total_espera = 0.0;
    tiempo_simulacion = 0.0;
    area_ocupada = 0.0;
    
    if (tipo_simulacion == 0) {
        simularErlangB();
    } else {
        simularErlangC();
    }
    
    reporte();
    
    fclose(parametros);
    fclose(resultados);
    if (proxima_salida != NULL)
        free(proxima_salida);
    
    return 0;
}

/*-------------------------------------------------------
  Simulación para Erlang B (sistema de pérdida).
---------------------------------------------------------*/
void simularErlangB(void)
{
    proxima_salida = (double *) malloc(num_servidores * sizeof(double));
    for (int i = 0; i < num_servidores; i++) {
        proxima_salida[i] = INFINITY_TIME;
    }
    ocupados_actuales = 0;
    
    double ultimo_tiempo_evento = 0.0;
    double tiempo_llegada = 0.0;
    
    for (int i = 0; i < num_llegadas_requeridas; i++) {
        double interarribo = exponencial(t_interarribo_promedio);
        tiempo_llegada += interarribo;
        
        while (1) {
            double min_salida = INFINITY_TIME;
            int indice_min_salida = -1;
            for (int j = 0; j < num_servidores; j++) {
                if (proxima_salida[j] < min_salida) {
                    min_salida = proxima_salida[j];
                    indice_min_salida = j;
                }
            }
            if (min_salida < tiempo_llegada) {
                tiempo_simulacion = min_salida;
                area_ocupada += ocupados_actuales * (tiempo_simulacion - ultimo_tiempo_evento);
                ultimo_tiempo_evento = tiempo_simulacion;
                proxima_salida[indice_min_salida] = INFINITY_TIME;
                ocupados_actuales--;
            } else {
                break;
            }
        }
        
        tiempo_simulacion = tiempo_llegada;
        area_ocupada += ocupados_actuales * (tiempo_simulacion - ultimo_tiempo_evento);
        ultimo_tiempo_evento = tiempo_simulacion;
        total_llegadas++;
        
        if (ocupados_actuales < num_servidores) {
            llamadas_atendidas++;
            int indice_libre = -1;
            for (int j = 0; j < num_servidores; j++) {
                if (proxima_salida[j] == INFINITY_TIME) {
                    indice_libre = j;
                    break;
                }
            }
            proxima_salida[indice_libre] = tiempo_simulacion + exponencial(t_servicio_promedio);
            ocupados_actuales++;
        } else {
            llamadas_bloqueadas++;
        }
    }
    
    while (ocupados_actuales > 0) {
        double min_salida = INFINITY_TIME;
        int indice_min_salida = -1;
        for (int j = 0; j < num_servidores; j++) {
            if (proxima_salida[j] < min_salida) {
                min_salida = proxima_salida[j];
                indice_min_salida = j;
            }
        }
        tiempo_simulacion = min_salida;
        area_ocupada += ocupados_actuales * (tiempo_simulacion - ultimo_tiempo_evento);
        ultimo_tiempo_evento = tiempo_simulacion;
        proxima_salida[indice_min_salida] = INFINITY_TIME;
        ocupados_actuales--;
    }
}

/*-------------------------------------------------------
  Simulación para Erlang C (sistema de colas).
---------------------------------------------------------*/
void simularErlangC(void)
{
    double ultimo_tiempo_evento = tiempo_simulacion;
    int num_en_cola = 0;
    double cola_espera[LIMITE_COLA];
    
    proxima_llegada = tiempo_simulacion + exponencial(t_interarribo_promedio);
    
    proxima_salida = (double *) malloc(num_servidores * sizeof(double));
    for (int i = 0; i < num_servidores; i++) {
        proxima_salida[i] = INFINITY_TIME;
    }
    
    while (llamadas_atendidas < num_llegadas_requeridas) {
        double min_tiempo = proxima_llegada;
        int tipo_evento = EVENTO_LLEGADA;
        int servidor_evento = -1;
        for (int i = 0; i < num_servidores; i++) {
            if (proxima_salida[i] < min_tiempo) {
                min_tiempo = proxima_salida[i];
                tipo_evento = EVENTO_SALIDA;
                servidor_evento = i;
            }
        }
        tiempo_simulacion = min_tiempo;
        double tiempo_desde_ultimo_evento = tiempo_simulacion - ultimo_tiempo_evento;
        ultimo_tiempo_evento = tiempo_simulacion;
        
        if (tipo_evento == EVENTO_LLEGADA) {
            total_llegadas++;
            proxima_llegada = tiempo_simulacion + exponencial(t_interarribo_promedio);
            int servidor_libre = -1;
            for (int i = 0; i < num_servidores; i++) {
                if (proxima_salida[i] == INFINITY_TIME) {
                    servidor_libre = i;
                    break;
                }
            }
            if (servidor_libre != -1) {
                proxima_salida[servidor_libre] = tiempo_simulacion + exponencial(t_servicio_promedio);
                llamadas_atendidas++;
            } else {
                if (num_en_cola < LIMITE_COLA) {
                    cola_espera[num_en_cola++] = tiempo_simulacion;
                    llamadas_esperaron++;
                } else {
                    fprintf(stderr, "Desbordamiento de la cola en el tiempo %f\n", tiempo_simulacion);
                    exit(1);
                }
            }
        } else {  // Evento de salida
            if (num_en_cola > 0) {
                double tiempo_llegada_evento = cola_espera[0];
                for (int i = 0; i < num_en_cola - 1; i++) {
                    cola_espera[i] = cola_espera[i + 1];
                }
                num_en_cola--;
                tiempo_total_espera += (tiempo_simulacion - tiempo_llegada_evento);
                proxima_salida[servidor_evento] = tiempo_simulacion + exponencial(t_servicio_promedio);
                llamadas_atendidas++;
            } else {
                proxima_salida[servidor_evento] = INFINITY_TIME;
            }
        }
    }
}

/*-------------------------------------------------------
  Reporte de los resultados de la simulación.
---------------------------------------------------------*/
void reporte(void)
{
    fprintf(resultados, "\n\n--- Resultados de la Simulación ---\n");
    fprintf(resultados, "Tiempo Total de Simulación: %f\n", tiempo_simulacion);
    fprintf(resultados, "Total de Llegadas:          %d\n", total_llegadas);
    
    if (tipo_simulacion == 0) {  /* Resultados de Erlang B */
        fprintf(resultados, "Total Atendidas:            %d\n", llamadas_atendidas);
        fprintf(resultados, "Total Bloqueadas:           %d\n", llamadas_bloqueadas);
        double probabilidad_bloqueo = (double) llamadas_bloqueadas / total_llegadas;
        fprintf(resultados, "Probabilidad de Bloqueo (Simulada): %f\n", probabilidad_bloqueo);
        
        double lambda = 1.0 / t_interarribo_promedio;
        double mu = 1.0 / t_servicio_promedio;
        double A = lambda / mu;
        double B_teorico = erlangB(A, num_servidores);
        fprintf(resultados, "Carga Ofrecida (A = λ/μ):     %f\n", A);
        fprintf(resultados, "Probabilidad de Bloqueo (Teórica Erlang B): %f\n", B_teorico);
        fprintf(resultados, "Número Promedio de Servidores Ocupados: %f\n", area_ocupada / tiempo_simulacion);
    } else {  /* Resultados de Erlang C */
        fprintf(resultados, "Total Atendidas:            %d\n", llamadas_atendidas);
        fprintf(resultados, "Total de Clientes que Esperaron: %d\n", llamadas_esperaron);
        double probabilidad_espera_sim = (double) llamadas_esperaron / total_llegadas;
        double tiempo_espera_promedio = (llamadas_esperaron > 0) ? tiempo_total_espera / llamadas_esperaron : 0.0;
        fprintf(resultados, "Probabilidad de Espera (Simulada):   %f\n", probabilidad_espera_sim);
        fprintf(resultados, "Tiempo Promedio de Espera:             %f\n", tiempo_espera_promedio);
        
        double lambda = 1.0 / t_interarribo_promedio;
        double mu = 1.0 / t_servicio_promedio;
        double A = lambda / mu;
        double probabilidad_espera_teorica = erlangC(A, num_servidores);
        fprintf(resultados, "Probabilidad de Espera (Teórica Erlang C):   %f\n", probabilidad_espera_teorica);
    }
}

/*-------------------------------------------------------
  Generador de variable exponencial usando el método de inversión.
---------------------------------------------------------*/
double exponencial(double media)
{
    double u = uniforme();
    return -media * log(u);
}

/*-------------------------------------------------------
  Generador Uniforme(0,1) usando lcgrand.
---------------------------------------------------------*/
double uniforme(void)
{
    // Utiliza lcgrand con stream 0 para generar números uniformes
    return (double) lcgrand(0);
}

/*-------------------------------------------------------
  Cálculo de la probabilidad de bloqueo (Erlang B) usando recursión.
---------------------------------------------------------*/
double erlangB(double A, int N)
{
    double B = 1.0;
    for (int i = 1; i <= N; i++) {
        B = (A * B) / (i + A * B);
    }
    return B;
}

/*-------------------------------------------------------
  Cálculo de la probabilidad teórica de espera (Erlang C).
---------------------------------------------------------*/
double erlangC(double A, int s)
{
    if (A >= s) {
        return 1.0;  // Sistema inestable
    }
    
    double suma = 0.0;
    double termino = 1.0;
    int n;
    for (n = 0; n < s; n++) {
        if (n > 0)
            termino *= A / n;
        suma += termino;
    }
    double termino_s = termino * (A / s);
    double PC = termino_s * (s / (s - A)) / (suma + termino_s * (s / (s - A)));
    return PC;
}
