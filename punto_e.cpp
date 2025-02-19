#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define LIMITE_COLA    1000         /* Tamaño máximo de la cola de espera (para Erlang C) */
#define INFINITY_TIME  1.0e+30      /* Un número muy grande para representar "infinito" */
#define EVENTO_LLEGADA  1
#define EVENTO_SALIDA   2

/* Variables globales */
int tipo_simulacion;              // Tipo de simulación: 0 = Erlang B; 1 = Erlang C.
int num_servidores;               // Número de servidores
double t_interarribo_promedio;    // Tiempo medio entre llegadas
double t_servicio_promedio;       // Tiempo medio de servicio
int num_llegadas_requeridas;      // Número de llegadas a simular

int total_llegadas;             // Total de llegadas procesadas
int llamadas_atendidas;         // Número de llamadas atendidas
int llamadas_bloqueadas;        // (Erlang B) Número de llamadas bloqueadas
int llamadas_esperaron;         // (Erlang C) Número de llamadas que esperaron
double tiempo_total_espera;     // Tiempo total de espera de las llamadas que esperaron

double tiempo_simulacion;       // Reloj de simulación
double area_ocupada;            // Tiempo acumulado de servidores ocupados (para estadísticas de promedio en el tiempo)

/* Para la programación de eventos (Erlang C) */
double proxima_llegada;         // Tiempo de la próxima llegada
double *proxima_salida;         // Arreglo de tiempos de próxima salida (uno por servidor)

/* Para la simulación de Erlang B (sistema de pérdida) usamos:
   - un contador de servidores ocupados actual
   - un arreglo (proxima_salida) para cada servidor ocupado (se establece en INFINITY_TIME cuando está libre)
*/
int ocupados_actuales;

/* Punteros a archivos */
FILE *parametros, *resultados;

/* Prototipos de funciones */
void simularErlangB(void);
void simularErlangC(void);
void reporte(void);
double exponencial(double media);
double uniforme(void);
double erlangB(double A, int N);
double erlangC(double A, int s);   // Probabilidad teórica de espera para Erlang C

int main(void)
{
    /* Abrir archivos de entrada y salida */
    parametros  = fopen("param.txt",  "r");
    resultados = fopen("result.txt", "w");
    if (parametros == NULL) {
        fprintf(stderr, "Error al abrir param.txt\n");
        exit(1);
    }
    
    /* Entrada esperada en param.txt: 
       t_interarribo_promedio t_servicio_promedio num_llegadas_requeridas num_servidores tipo_simulacion 
       (Para Erlang B, tipo_simulacion debe ser 0; para Erlang C, tipo_simulacion debe ser 1)
    */
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
    
    /* Semilla para el generador de números aleatorios */
    srand((unsigned) time(NULL));
    
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
  Simulación para Erlang B (sistema de pérdida) usando tiempos fijos de llegada.
  En este enfoque se genera cada tiempo de llegada de forma independiente.
  Antes de procesar una llegada, se "avanza" la simulación procesando todas las salidas que ocurran
  estrictamente antes de esa llegada.
  (Una llamada que llega se pierde si todos los servidores están ocupados en su tiempo programado.)
---------------------------------------------------------*/
void simularErlangB(void)
{
    /* Asignar e inicializar los tiempos de salida para cada servidor */
    proxima_salida = (double *) malloc(num_servidores * sizeof(double));
    for (int i = 0; i < num_servidores; i++) {
        proxima_salida[i] = INFINITY_TIME;
    }
    ocupados_actuales = 0;
    
    double ultimo_tiempo_evento = 0.0;  // Para el cálculo del promedio de ocupación
    double tiempo_llegada = 0.0;
    
    for (int i = 0; i < num_llegadas_requeridas; i++) {
        /* Generar el siguiente tiempo de llegada */
        double interarribo = exponencial(t_interarribo_promedio);
        tiempo_llegada += interarribo;
        
        /* Procesar todas las salidas que ocurran estrictamente antes de esta llegada */
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
                /* Avanzar el tiempo de simulación al tiempo de salida */
                tiempo_simulacion = min_salida;
                area_ocupada += ocupados_actuales * (tiempo_simulacion - ultimo_tiempo_evento);
                ultimo_tiempo_evento = tiempo_simulacion;
                /* Procesar la salida: liberar el servidor */
                proxima_salida[indice_min_salida] = INFINITY_TIME;
                ocupados_actuales--;
            } else {
                break;
            }
        }
        
        /* Ahora procesar el evento de llegada en el tiempo "tiempo_llegada" */
        tiempo_simulacion = tiempo_llegada;
        area_ocupada += ocupados_actuales * (tiempo_simulacion - ultimo_tiempo_evento);
        ultimo_tiempo_evento = tiempo_simulacion;
        total_llegadas++;
        
        if (ocupados_actuales < num_servidores) {
            /* Hay un servidor libre: se atiende la llamada */
            llamadas_atendidas++;
            /* Encontrar un servidor libre (donde proxima_salida es INFINITY_TIME) */
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
            /* Todos los servidores están ocupados en el tiempo de llegada: se bloquea la llamada */
            llamadas_bloqueadas++;
        }
    }
    
    /* Procesar las salidas restantes (para el cálculo del área) */
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
  Simulación para Erlang C (sistema de colas) usando programación de eventos estándar.
  (Este es esencialmente el código original para Erlang C.)
---------------------------------------------------------*/
void simularErlangC(void)
{
    double ultimo_tiempo_evento = tiempo_simulacion;
    int num_en_cola = 0;
    double cola_espera[LIMITE_COLA];
    
    /* Programar la primera llegada */
    proxima_llegada = tiempo_simulacion + exponencial(t_interarribo_promedio);
    
    /* Asignar e inicializar los tiempos de salida para cada servidor */
    proxima_salida = (double *) malloc(num_servidores * sizeof(double));
    for (int i = 0; i < num_servidores; i++) {
        proxima_salida[i] = INFINITY_TIME;
    }
    
    while (llamadas_atendidas < num_llegadas_requeridas) {
        /* Determinar el siguiente evento */
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
        /* (Se pueden agregar cálculos del área de forma similar si se desea) */
        ultimo_tiempo_evento = tiempo_simulacion;
        
        if (tipo_evento == EVENTO_LLEGADA) {
            total_llegadas++;
            proxima_llegada = tiempo_simulacion + exponencial(t_interarribo_promedio);
            /* Procesar la llegada: si hay un servidor libre, iniciar el servicio; de lo contrario, encolar */
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
                    cola_espera[i] = cola_espera[i+1];
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
  Reportar los resultados de la simulación.
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
        
        /* Calcular la carga ofrecida A = λ/μ, donde λ = 1/t_interarribo_promedio y μ = 1/t_servicio_promedio */
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
        
        /* Calcular la carga ofrecida A = λ/μ para Erlang C también */
        double lambda = 1.0 / t_interarribo_promedio;
        double mu = 1.0 / t_servicio_promedio;
        double A = lambda / mu;
        /* Calcular la probabilidad teórica de espera usando la fórmula de Erlang C */
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
  Generador Uniforme(0,1) usando rand()
---------------------------------------------------------*/
double uniforme(void)
{
    /* Retorna un número estrictamente entre 0 y 1 */
    return (((double) rand() + 1.0) / ((double) RAND_MAX + 2.0));
}

/*-------------------------------------------------------
  Calcular la probabilidad de bloqueo de Erlang B usando recursión.
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
  Calcular la probabilidad teórica de espera para una cola M/M/s usando la fórmula de Erlang C.
---------------------------------------------------------*/
double erlangC(double A, int s)
{
    if (A >= s) {
        /* El sistema es inestable; retornar 1 ya que todas las llegadas esperarán */
        return 1.0;
    }
    
    double suma = 0.0;
    double termino = 1.0;  // Término para n = 0: A^0 / 0! = 1
    int n;
    for (n = 0; n < s; n++) {
        if (n > 0)
            termino *= A / n;  // Calcular A^n/n! de forma recursiva
        suma += termino;
    }
    /* "termino" ahora tiene A^(s-1)/(s-1)!; calcular el término para n = s */
    double termino_s = termino * (A / s);  // A^s/s!
    
    double PC = termino_s * (s / (s - A)) / (suma + termino_s * (s / (s - A)));
    return PC;
}