import math
import time
def racine_simple(nbr):
    for i in range(nbr-1):
        if i * i > nbr:
            return i-1

# Tres mauvais avec grand nombre
def racine_simple_approximation(nbr,approximation):
    racine = 0
    iteration = 0

    for j in range(0,approximation) :
        i = 0
        while ((racine + i*10**(-j)) * (racine + i*10**(-j)) < nbr) :
            iteration +=1
            i += 1

        i -= 1
        racine += i*10**(-j)
    print("Nombre d'itération :",iteration)
    return racine



#ELLE correspond a peu pres a ça en C:
def sqrtC(n, epsilon=10):
    if n < 0:
        raise ValueError("Square root of negative number is not defined")
    
    x = n
    # SUITE DE HERRON
    iteration = 0
    while abs(x * x - n) > 10**-epsilon:
        iteration +=1
        x = (x + n / x) / 2
    print("Nombre d'itération :",iteration)
    return x


def test(nbr):
    print("Racine carrée")
    start_time = time.perf_counter()
    # Utilise une double méthode : d'abord aprox avec decalage puis precision avec Newton-Raphson
    print(math.sqrt(nbr))
    end_time = time.perf_counter()
    print("Temps d'exécution :",end_time - start_time," s")

    print("Racine carrée simple")
    start_time = time.perf_counter()
    print(racine_simple_approximation(nbr,10))
    end_time = time.perf_counter()
    print("Temps d'exécution :",end_time - start_time," s")

    print("Racine carrée simple C")
    start_time = time.perf_counter()
    print(sqrtC(nbr,10))
    end_time = time.perf_counter()
    print("Temps d'exécution :",end_time - start_time," s")

nbr = 871683672678.3897298378237297
print("Calcul pour :",nbr)
test(nbr)
print()
nbr = 25
print("Calcul pour :",nbr)
test(nbr)




# sqrt en python est faire en C pour plus d'efficacité :


# ### Implémentation interne de `sqrt`
# L'implémentation de `sqrt` dépend de la bibliothèque standard utilisée (glibc, musl, etc.) et du processeur (x86, ARM, etc.). Généralement, elle utilise :

# 1. **L'instruction matérielle du processeur** :  
#    - Sur les architectures modernes, la plupart des processeurs disposent d'une instruction dédiée pour calculer la racine carrée, comme `fsqrt` en x86 (ASSEMBLEUR) (SSE/AVX) ou `SQRTSD` pour les nombres en virgule flottante.
#    - Si le matériel supporte une telle instruction, la bibliothèque standard l’utilise pour des performances optimales.

# 2. **L'algorithme de Newton-Raphson** (méthode de Newton-Raphson) :  
#    - Quand aucune instruction matérielle n’est disponible, une approche logicielle est utilisée, souvent basée sur la **méthode de Newton-Raphson** :
#      \[
#      x_{n+1} = \frac{1}{2} \left( x_n + \frac{S}{x_n} \right)
#      \]
#      où \( S \) est le nombre dont on cherche la racine carrée.

# 3. **Tableaux de lookup et approximation initiale** :  
#    - Pour améliorer la convergence, l'approximation initiale peut être obtenue avec une table de précalcul ou une astuce basée sur la représentation binaire IEEE 754 des nombres flottants.

# 4. **Approximations pour les performances** :  
#    - Certaines implémentations utilisent des approximations comme **l'algorithme de Fast Inverse Square Root**, célèbre dans le moteur Quake III.



# Une ou deux itérations suffisent pour une précision IEEE 754 complète.
# pseudocode CPU
# movsd xmm0, [val]  ; Charger x dans un registre SSE
# sqrtsd xmm1, xmm0  ; Approximation matérielle de sqrt(x)
# movsd xmm2, xmm0   ; Copier x
# divsd xmm2, xmm1   ; x / y_n
# addsd xmm1, xmm2   ; y_n + (x / y_n)
# mulsd xmm1, [half] ; Multiplier par 0.5


# Code en assembleur x86-64 (Intel syntax)

# section .data
#     half dq 0.5   ; Constante 0.5 en double précision
#     val  dq 25.0  ; Valeur dont on veut la racine carrée
#     result dq 0.0 ; Stockage du résultat

# section .text
#     global main
#     extern printf
#     extern exit

# main:
#     ; Charger la valeur de `val` dans xmm0
#     movsd xmm0, qword [val]   ; xmm0 = x (ex: 25.0)

#     ; Calculer sqrt(x) avec l'instruction matérielle SSE2
#     sqrtsd xmm1, xmm0         ; xmm1 = sqrt(x) (première approximation)

#     ; Copie de x pour l'itération de Newton-Raphson
#     movsd xmm2, xmm0          ; xmm2 = x

#     ; Division x / y_n (Newton-Raphson)
#     divsd xmm2, xmm1          ; xmm2 = x / y_n

#     ; y_n+1 = 0.5 * (y_n + x / y_n)
#     addsd xmm1, xmm2          ; xmm1 = y_n + (x / y_n)
#     mulsd xmm1, qword [half]  ; xmm1 = 0.5 * xmm1 (mise à jour de y_n)

#     ; Stocker le résultat
#     movsd qword [result], xmm1

#     ; Afficher le résultat avec printf
#     mov rdi, format
#     movq xmm0, xmm1
#     call printf

#     ; Quitter le programme
#     mov rdi, 0
#     call exit

# section .data
#     format db "Racine carrée approximée : %lf",10,0  ; Format pour printf
