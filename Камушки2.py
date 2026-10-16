from itertools import product
import random

def find_best_distribution(weights, m):
    n = len(weights)
    total_weight = sum(weights)
    ideal_weight = total_weight / m  # Идеальный вес(Средний)
    
    best_distribution = None
    min_deviation = float('inf')
    
    for dist in product(range(m), repeat=n):
        piles = [0] * m
        for i, pile_num in enumerate(dist):
            piles[pile_num] += weights[i]
        
        total_deviation = 0
        for pile_weight in piles:
            total_deviation += abs(pile_weight - ideal_weight)
        
        if total_deviation < min_deviation:
            min_deviation = total_deviation
            best_distribution = dist
    
    return best_distribution, min_deviation, ideal_weight

weights = [random.randint(1, 10) for _ in range(6)]
m = 3
weights = [78,5,66,75,28,25,23,35,20,40]
print(f"Камни: {weights}")
print(f"Кучи: {m}")

best_dist, deviation, ideal = find_best_distribution(weights, m)

piles = [[] for _ in range(m)]
pile_weights = [0] * m

for i, pile_num in enumerate(best_dist):
    piles[pile_num].append(weights[i])
    pile_weights[pile_num] += weights[i]

print(f"\nИдеальный вес каждой кучи: {ideal:.2f}")
print("\nЛучшее распределение:")
for i, pile in enumerate(piles):
    current_weight = sum(pile)
    current_deviation = abs(current_weight - ideal)
    print(f"Куча {i+1}: {pile} (вес: {current_weight}, отклонение: {current_deviation:.2f})")

print(f"\nСуммарное отклонение от идеала: {deviation:.2f}")
