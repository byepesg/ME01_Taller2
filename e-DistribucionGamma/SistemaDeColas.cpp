#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h"  /* Generador de números aleatorios */

#define LIMITE_COLA 100  /* Capacidad máxima de la cola */
#define OCUPADO      1  /* Indicador de Servidor Ocupado */
#define LIBRE      0  /* Indicador de Servidor Libre */

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

int main(void)  
{
    /* Abre el archivo de parámetros */
    FILE *parametros = fopen("param.txt", "r");
    if (parametros == NULL) {
        printf("❌ Error: No se pudo abrir param.txt\n");
        return 1;
    }

    /* Lee los parámetros desde el archivo */
    fscanf(parametros, "%f %f", &alpha1, &lambda1);  // α1, λ1 (ahora se usa el que está en el archivo)
    fscanf(parametros, "%f %f", &alpha2, &lambda2);  // α2, λ2
    fscanf(parametros, "%d %d", &m, &num_esperas_requerido); // m, num_esperas
    fclose(parametros);  // Cierra el archivo después de leerlo

    /* Abre el archivo de salida */
    resultados_lambda = fopen("resultados_lambda.txt", "a"); // Usa "a" para no sobrescribir
    if (resultados_lambda == NULL) {
        printf("❌ Error: No se pudo abrir resultados_lambda.txt\n");
        return 1;
    }

    /* Mensaje de depuración */
    printf("📌 Parámetros cargados desde param.txt: alpha1=%.2f, lambda1=%.2f, alpha2=%.2f, lambda2=%.2f, m=%d, num_esperas=%d\n",
           alpha1, lambda1, alpha2, lambda2, m, num_esperas_requerido);

    /* Inicializa la simulación */
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

    /* Calcula la probabilidad de cola */
    float P_cola = tiempo_total_con_cola / tiempo_simulacion;

    /* Guarda el resultado en el archivo */
    fprintf(resultados_lambda, "%.2f  %.3f\n", lambda1, P_cola);
    printf("λ1 = %.2f, P_cola = %.3f\n", lambda1, P_cola);

    reportes();
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


void reportes(void) {
    /* Abre el archivo de salida donde se guardarán los resultados detallados */
    FILE *resultados = fopen("resultados_detallados.txt", "a");
    if (resultados == NULL) {
        printf("❌ Error: No se pudo abrir resultados_detallados.txt\n");
        return;
    }

    /* Calcula métricas */
    float demora_promedio = (num_clientes_espera > 0) ? total_de_esperas / num_clientes_espera : 0;
    float clientes_promedio = area_num_entra_cola / tiempo_simulacion;
    float utilizacion = area_estado_servidor / (m * tiempo_simulacion); // Uso relativo a m servidores
    float P_cola = tiempo_total_con_cola / tiempo_simulacion;

    /* Escribe los resultados en el archivo */
    fprintf(resultados, "=========================================\n");
    fprintf(resultados, "Simulación con λ1 = %.2f\n", lambda1);
    fprintf(resultados, "=========================================\n");
    fprintf(resultados, "Demora promedio en la cola:      %.3f minutos\n", demora_promedio);
    fprintf(resultados, "Número promedio de clientes en cola: %.3f\n", clientes_promedio);
    fprintf(resultados, "Utilización de los servidores:   %.3f\n", utilizacion);
    fprintf(resultados, "Tiempo de terminación:           %.3f minutos\n", tiempo_simulacion);
    fprintf(resultados, "Probabilidad de que haya cola:   %.3f\n", P_cola);
    fprintf(resultados, "=========================================\n\n");

    /* Cierra el archivo */
    fclose(resultados);

    /* Imprime en la terminal para confirmar la ejecución */
    printf("\n📊 Reporte generado para λ1 = %.2f\n", lambda1);
    printf("Demora promedio en cola: %.3f minutos\n", demora_promedio);
    printf("Número promedio de clientes en cola: %.3f\n", clientes_promedio);
    printf("Utilización de los servidores: %.3f\n", utilizacion);
    printf("Tiempo de terminación: %.3f minutos\n", tiempo_simulacion);
    printf("Probabilidad de cola (P_cola): %.3f\n\n", P_cola);
}
