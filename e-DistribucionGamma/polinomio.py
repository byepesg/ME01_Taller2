import numpy as np
import matplotlib.pyplot as plt


# Load data from the file, skipping the first row (header)
data = np.loadtxt("resultados_lambda.txt", skiprows=1)

# Extract lambda1 and P_cola values
lambda1_vals = data[:, 0].tolist()  # First column
p_cola_vals = data[:, 1].tolist()  # Second column

print("lambda1_vals =", lambda1_vals)
print("p_cola_vals =", p_cola_vals)


# Datos de la simulación
# lambda1_vals = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5]
# p_cola_vals = [0.95, 0.85, 0.70, 0.50, 0.30, 0.10, 0.05]

# Ajuste polinómico de grado 3
coeficientes = np.polyfit(lambda1_vals, p_cola_vals, 20)
polinomio = np.poly1d(coeficientes)

# Graficar
x = np.linspace(0.5, 3.5, 100)
y = polinomio(x)

plt.scatter(lambda1_vals, p_cola_vals, color='red', label='Datos de simulación')
plt.plot(x, y, label=f"Ajuste: {polinomio}", color='blue')
plt.xlabel("λ1")
plt.ylabel("P_cola(λ1)")
plt.title("Probabilidad de que se forme cola en función de λ1")
plt.legend()
plt.grid()
plt.show()
