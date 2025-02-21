***Orden de ejecución***
1) **Compilar**:  g++ "Gamma.cpp" lcgrand.cpp -o simulador -lm
El anterior comando compilar el codigo y genera un archivo "simulador.exe".

2) .\simulador.exe

Al ejecutar el simulador se obtiene los un archivo llamado resultados.csv, donde se encuentra un excel con las siguiente columnas en orden:

- lambda1
- Demora_promedio
- Clientes_promedio
- Utilizacion
- Pcola (Probabilidad de que haya cola)
- Tiempo_total

El archivo paramGamma.txt se le pasan los siguientes datos en el siguiente orden, ej:
3.5 0 4.3 7.5 2 1000

$\alpha_1 = 3.5$
$\lambda_1 = 0$ (Se leen los $lambda_1$ de lambdas.txt no aqui)
$\alpha_2 = 4.3$
$\lambda_2 = 7.5$
m = 2 (numero de servidores atendiendo)
n = 1000 (numero de esperas requerido)
