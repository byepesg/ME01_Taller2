#!/bin/bash
# Compile the C program
gcc -o simulacion_gamma SistemaDeColas.cpp lcgrand.cpp -std=c++11

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed! Please check your code."
    exit 1
fi

echo "✅ Compilation successful! Running simulations..."
# Lista de valores de lambda_1 que queremos probar
lambda1_values=(0.5 1.0 1.5 2.0 2.5 3.0 3.5 4.0 4.5 5.0 )
#lambda1_values=(0.5 1.0 1.5 2.0 2.5 3.0 3.5 4.0 4.5 5.0 5.5 6.0 6.5 7.0 7.5 8.0 8.5 9.0 9.5 10.0 10.5 11.0 11.5 12.0 12.5 13.0 13.5 14.0 14.5 15.0 15.5 16.0 16.5 17.0 17.5 18.0 18.5 19.0 19.5 20.0)

# Archivo de salida donde se guardarán los resultados
output_file="resultados_lambda.txt"

# Limpiar el archivo de salida antes de empezar
echo "λ1  P_cola(λ1)" > $output_file

# Ejecutar la simulación para cada valor de lambda1
for lambda1 in "${lambda1_values[@]}"
do
    # Crear un nuevo archivo param.txt con el valor de lambda1
    echo -e "3.5 $lambda1\n4.3 7.5\n1 1000" > param.txt

    # Ejecutar la simulación
    ./simulacion_gamma

    # Extraer el valor de P_cola de result.txt (corregido)
    P_cola=$(grep "Probabilidad de que se forme cola" result.txt | awk '{print $(NF)}')

    # Validar si P_cola es numérico
    if [[ $P_cola =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        echo "$lambda1  $P_cola" >> $output_file
        echo "Simulación para λ1 = $lambda1 completada. P_cola = $P_cola"
    else
        echo "Error: No se pudo obtener P_cola para λ1 = $lambda1"
    fi
done

echo "Simulaciones finalizadas. Resultados guardados en $output_file."
