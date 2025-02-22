import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# 1. Leer el archivo CSV
df = pd.read_csv('resultados.csv')

# 2. Elegir la columna que usarás como eje X (lambda1)
x = df['lambda1']

# 3. Definir las columnas que deseas ajustar (omitimos 'lambda1')
columnas = [
    'Demora_promedio',
    'Clientes_promedio',
    'Utilizacion',
    'Pcola_arrivals',
    'Pcola_tiempo',
    'Tiempo_total'
]

# Parámetro: grado del polinomio a ajustar (cambia según necesites)
grado_polinomio = 47  # por ejemplo, polinomio cuadrático

for col_data in columnas:
    # Extraer la serie de valores
    y = df[col_data]

    # Ajustar polinomio usando numpy.polyfit
    # polyfit(x, y, deg) -> devuelve coeficientes [a0, a1, ..., a_deg] en orden de potencias descendentes
    coef = np.polyfit(x, y, grado_polinomio)
    
    # Generar valores ajustados
    # polyval([a0, a1, a2], x) -> evalúa el polinomio para cada x
    y_fit = np.polyval(coef, x)

    # 4. Imprimir polinomio en la terminal
    # Recordar: coef[0] * x^grado + coef[1] * x^(grado-1) + ...
    print(f"\n=== Ajuste para '{col_data}' (grado {grado_polinomio}) ===")
    # Construir un string con la forma a_n x^n + a_{n-1} x^{n-1} + ... + a_0
    # Ejemplo para grado 2: a0 x^2 + a1 x + a2
    termino = []
    pot = grado_polinomio
    for c in coef:
        termino.append(f"{c:.4f} x^{pot}")
        pot -= 1
    polinomio_str = " + ".join(termino)
    # Limpieza menor si el exponente llega a 1 o 0
    # (Podrías hacer un formateo más sofisticado si deseas)
    
    print("Coeficientes (de mayor a menor potencia):", coef)
    print("Polinomio aproximado:", polinomio_str)

    # 5. Graficar en una nueva figura
    plt.figure()
    # Puntos originales
    plt.plot(x, y, 'o', label='Datos')
    # Curva ajustada
    plt.plot(x, y_fit, '-', label='Ajuste polinómico')
    plt.title(f"{col_data} vs lambda1 (Ajuste grado {grado_polinomio})")
    plt.xlabel('lambda1')
    plt.ylabel(col_data)
    plt.grid(True)
    plt.legend()

# 6. Mostrar todas las figuras
plt.show()
