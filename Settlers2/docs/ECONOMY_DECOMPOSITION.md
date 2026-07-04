# Economy Decomposition — Full Graph & Architecture

## 1. Полный граф экономики Settlers II

```
RENEWABLE (природные ресурсы)
──────────────────────────────────────────────────
  Trees ──→ Woodcutter ──→ Wood
    ↑            │
    │            ├──→ Forester ──→ Trees (замыкание)
    │            │
    │            └──→ Sawmill ──→ Planks ──→ Toolmaker ──→ Tools
    │                                             ↑
  Animals ──→ Hunter ──→ Meat ──┐                  │
                                ├──→ Mine ──→ Ore  │
  Fish ────→ Fisher ──→ Fish ──┘                  │
                                                  │
  Arable Land ──→ Farm ──→ Grain ──→ Mill ──→ Flour ──→ Bakery ──→ Bread ──→ Mine
                                                  │                       │
                                                  │            ┌──────────┘
                                                  │            │
                                                  └──→ Well ──→ Water

  Rock ─→ Quarry ─→ Stone ──────────────────────────────────────────→ Toolmaker

EXTRACTION (истощаемые ресурсы)
──────────────────────────────────────────────────
  Coal seam ──→ CoalMine ──→ Coal ──→ IronSmelter ──→ IronBar ──→ Smith ──→ Weapons
                    ↑ (требует Food)      ↑ (требует Coal)          ↑
  Iron ore ──→ IronMine ──→ IronOre ─────┘                        │
                    ↑ (требует Food)                               │
  Gold ore ──→ GoldMine ──→ Gold ──→ Mint ──→ Coins               │
                    ↑ (требует Food)                               │
                                                                   │
MILITARY (конечное потребление)                                    │
──────────────────────────────────────────────────                 │
  Barracks ──→ Soldiers (потребляет Weapons) ──────────────────────┘
  Harbor ──→ Ships (потребляет Planks)
```

## 2. Три типа контуров

### Тип A: Renewable (возобновляемые)
Ресурс существует в мире как популяция. Добытчик уменьшает популяцию. Популяция регенерирует.

| Ресурс мира | Добытчик | Продукт | Регенерация |
|-------------|----------|---------|-------------|
| Trees | Woodcutter | Wood | Forester (сажает) |
| Animals | Hunter | Meat | Автоматическая (каждый N тиков) |
| Fish | Fisher | Fish | Автоматическая (каждый N тиков) |
| Arable Land | Farm | Grain | Смена полей (ротация) |

**Архитектурный паттерн**: Woodcutter/Tree уже реализован через TreeSystem. Hunter/Animals и Fisher/Fish — **копия того же паттерна**. Значит, нужен общий `RenewableResourceSystem`.

### Тип B: Transformation (трансформация) ✅ УЖЕ РЕАЛИЗОВАН
Ресурс A → Ресурс B через ProductionDefinition. Не зависит от мира.

| Здание | Вход | Выход |
|--------|------|-------|
| Sawmill | 2×Wood | 1×Planks |
| Toolmaker | 1×Wood + 1×Stone | 1×Tools |
| Mill | 1×Grain | 1×Flour |
| Bakery | 1×Flour | 1×Bread |
| IronSmelter | 1×IronOre + 1×Coal | 1×IronBar |

**Статус**: Полностью data-driven через `ProductionDefinition`. Изменений не требует.

### Тип C: Consumption (потребление) 🔴 НОВЫЙ ТИП
Шахта потребляет еду для добычи руды. Без еды — 0 добычи.

| Шахта | Потребляет | Производит | Без еды |
|-------|-----------|------------|---------|
| CoalMine | Food (Meat/Fish/Bread) | Coal | 0 добычи |
| IronMine | Food (Meat/Fish/Bread) | IronOre | 0 добычи |
| GoldMine | Food (Meat/Fish/Bread) | Gold | 0 добычи |

**Архитектурная новизна**: Это НЕ production. Шахта — producer (Ore), но её production rate зависит от наличия Food. Это bidirectional dependency.

## 3. Проектирование ConsumptionDefinition

### ConsumptionDefinition.h
```cpp
#pragma once
#include "../Core/ResourceTypes.h"
#include "../Core/ProductionTypes.h"

namespace World {

    static const int kMaxFoodTypes = 4;

    struct FoodOption {
        ResourceType resource;  // Meat, Fish, или Bread
        int efficiency;         // Единиц добычи за единицу еды
    };

    struct ConsumptionDefinition {
        ProductionType mineType;           // PT_CoalMine, PT_IronMine, PT_GoldMine
        FoodOption foodOptions[kMaxFoodTypes];  // Альтернативные виды еды
        int baseRate;                      // Добыча без еды (0 или малая)
        int fedRate;                       // Добыча с едой (полная скорость)
        int foodPerCycle;                  // Сколько еды потребляет за цикл
    };

    const ConsumptionDefinition& GetConsumptionDefinition(ProductionType mineType);

    // Definition Query API
    bool IsFoodResource(ResourceType resource);
    ProductionType GetMineConsuming(ResourceType foodResource);
}
```

### ConsumptionDefinition.cpp
```cpp
#include "ConsumptionDefinition.h"

namespace World {

    static const ConsumptionDefinition g_consumptions[] = {
        // mineType    foodOptions                         baseRate  fedRate  foodPerCycle
        { PT_CoalMine, { { ResourceType_Meat, 1 },
                          { ResourceType_Fish, 1 },
                          { ResourceType_Bread, 3 },
                          { ResourceType_None, 0 } },        0,        30,       1 },
        { PT_IronMine, { { ResourceType_Meat, 1 },
                          { ResourceType_Fish, 1 },
                          { ResourceType_Bread, 3 },
                          { ResourceType_None, 0 } },        0,        30,       1 },
        { PT_GoldMine, { { ResourceType_Meat, 1 },
                          { ResourceType_Fish, 1 },
                          { ResourceType_Bread, 3 },
                          { ResourceType_None, 0 } },        0,        30,       1 },
    };

    // ... accessor functions
}
```

### ConsumptionSystem (новый ISimulationSystem)
```
Tick():
  for each mine in world.productionBuildings:
    if mine.type in { CoalMine, IronMine, GoldMine }:
      def = GetConsumptionDefinition(mine.productionType)
      hasFood = CheckFoodDelivery(mine)
      if hasFood:
        mine.cycleTime = def.fedRate
        ConsumeFood(mine)  // уменьшает inputDelivered
      else:
        mine.cycleTime = def.baseRate  // 0 = стоять
        RequestFood(mine)  // SetDemand через DemandManager
```

### ConsumptionSystem vs ProductionSystem

**ConsumptionSystem** — отдельная система, потому что:
1. ProductionSystem уже отвечает за "сырьё → продукт"
2. ConsumptionSystem отвечает за "еда → разрешение на добычу"
3. У шахты два входа: еда (через Consumption) и ничего (руда — через Production)
4. Если смешать, ProductionSystem получит double responsibility

**Взаимодействие**:
```
ProductionSystem::ProcessProduction() → шахта добывает руду
ConsumptionSystem::Tick() → проверяет наличие еды, ставит rate
```

Флаг `mine.fed` в `ProductionBuilding`:
- ConsumptionSystem устанавливает `mine.fed = true/false`
- ProductionSystem читает `mine.fed` для шахт

## 4. Проектирование RenewableResourceDefinition

### Текущий паттерн (TreeSystem)
```
TreeSystem.h (header-only):
  - SeedTrees(world, matureCount, emptySpots)
  - AdvanceTreeGrowth(world) — каждые 100 тиков
  - CanWoodcutterProduce(world) — проверяет treeMatureCount > 0
  - ConsumeTree(world) — treeMatureCount--
  - PlantSapling(world) — emptySpotCount--, saplingCount++
```

WorldModel содержит 5 счётчиков: `treeMatureCount, treeYoungCount, treeSaplingCount, treeStumpCount, treeEmptyCount`.

### Обобщение: RenewableResourceDefinition
```cpp
struct RenewableResourceDefinition {
    ResourceType resource;       // Meat, Fish
    int regrowInterval;          // Тиков между регенерацией
    int regrowAmount;            // Сколько особей добавляется за раз
    int maxPopulation;           // Максимальный размер популяции
    int consumePerCycle;         // Сколько потребляет добытчик за цикл
};
```

### ResourcePopulationSystem (замена TreeSystem)
Вместо 5 полей для деревьев — массив популяций:

```cpp
struct ResourcePopulation {
    ResourceType resource;
    int currentCount;
    int maxCount;
};
```

WorldModel получает `resourcePopulations[]` и `resourcePopulationCount`.

TreeSystem остаётся как частный случай (деревья имеют стадии роста), но Hunter/Animals и Fisher/Fish — простые популяции (count регенерирует, нет стадий).

## 5. Рефакторинг TreeSystem → ResourcePopulationSystem

```cpp
void ResourcePopulationSystem::Tick(WorldModel& world) {
    for each population:
        if population.currentCount < population.maxCount:
            if tickCount % regrowInterval == 0:
                population.currentCount = min(population.currentCount + regrowAmount, maxCount);
}

bool TryConsumeResource(WorldModel& world, ResourceType type, int amount) {
    ResourcePopulation* pop = FindPopulation(world, type);
    if (!pop || pop->currentCount < amount) return false;
    pop->currentCount -= amount;
    return true;
}
```

Woodcutter больше не вызывает `CanWoodcutterProduce/ConsumeTree` напрямую. Вместо этого Woodcutter вызывает `TryConsumeResource(world, ResourceType_Tree, 1)`.

Forester вызывает `TryAddResource(world, ResourceType_Tree, 1)`.

## 6. SettlementSystem — новые bootstrap правила

После реализации Consumption System SettlementSystem получает новые правила:

| Правило | Условие | Действие | Приоритет |
|---------|---------|----------|-----------|
| BootstrapFisher | Нет Fisher + есть Wood ресурсы | Build Fisher | 350 |
| BootstrapHunter | Нет Hunter + есть Wood ресурсы | Build Hunter | 350 |
| BootstrapFoodChain | Есть Mine + нет Food source | Build Hunter или Fisher | 450 |
| BootstrapMill | Есть Farm + нет Mill | Build Mill | 400 |
| BootstrapBakery | Есть Mill + нет Bakery | Build Bakery | 500 |

Все правила используют Definition Query API — ни одного hardcoded BuildingType.

## 7. Принцип эволюции: prove → accumulate → extract

Этот roadmap строго следует принципу, доказанному всем проектом:
```
простая реализация → вторая реализация → третья → выделение абстракции
```

**Деревья** — первая реализация (TreeSystem ✅).
**Животные** — докажут, что модель применима к другому ресурсу.
**Рыба** — третья реализация, после которой выделяется ResourcePopulationSystem.

Никакого преждевременного обобщения.

## 8. Roadmap

```
Phase 4:   Hunter + Animals
               ↓
Phase 5:   Fisher + Fish
               ↓
Phase 6:   ConsumptionDefinition + ConsumptionSystem + Mines
               ↓
Phase 7:   Farm → Mill → Bakery → Bread (Food chain)
               ↓
Phase 8:   ResourcePopulationSystem (обобщение после 3 реализаций)
               ↓
Phase 9:   Military (Smith, Mint, Barracks, Harbor)
```

### Phase 4 — Hunter + Animals

**Цель**: доказать, что модель деревьев применима ко второму возобновляемому ресурсу.

**Что делается**:
- TreeSystem расширяется: `CanHunterProduce()`, `ConsumeAnimal()`, `RegenerateAnimals()`
- Animals — простая популяция (одно число, без стадий роста)
- Минимальные изменения: Animals — это второй `ResourcePopulationBehavior`, но пока без общего интерфейса
- Hunter получает через ProductionDefinition: `{ PT_Hunter, {}, { Meat, 1 }, 30 }`
- WorldModel: `animalCount` (int) + `maxAnimalCount`
- `SeedAnimals(world, count)` — инициализация
- `AdvanceAnimalGrowth(world)` — каждые 100 тиков: animalCount = min(animalCount + 1, maxAnimalCount)
- ProductionSystem::ProcessProduction: `if (Hunter && CanHunterProduce) ConsumeAnimal()`
- Settlement правило: BootstrapHunter (приоритет 350)

**Проверка**: T43 — Hunter производит Meat, популяция регенерирует, без животных — 0 Meat.

### Phase 5 — Fisher + Fish

**Цель**: получить третью реализацию, после которой становится видно, что общего.

**Что делается**:
- `CanFisherProduce()`, `ConsumeFish()`, `RegenerateFish()`
- WorldModel: `fishCount` + `maxFishCount`
- `SeedFish(world, count)`
- ProductionSystem: `if (Fisher && CanFisherProduce) ConsumeFish()`

**Проверка**: T44 — Fisher производит Fish, популяция регенерирует.

### Phase 6 — ConsumptionDefinition + ConsumptionSystem + Mines

**Цель**: добавить новый тип экономических связей (потребление).

**Что делается**:
- `ConsumptionDefinition.h/.cpp` — таблица { mineType, foodOptions[], baseRate, fedRate, foodPerCycle }
- `ConsumptionSystem` (новый ISimulationSystem) — проверяет наличие Food у шахт, ставит `mine.fed`
- `ProductionBuilding.fed` (bool) — флаг, что шахта обеспечена едой
- `ProductionSystem::ProcessProduction` — для шахт проверяет `pb.fed`; если false → добыча 0
- ConsumptionSystem вызывает `DemandManager::SetDemand` для Food
- Settlement правила: BootstrapMiningExpanded — если есть Mine и нет Food source → Build Hunter/Fisher

**Проверка**: T45 — шахта без еды стоит, с едой добывает; T46 — ConsumptionSystem выбирает лучший доступный Food.

### Phase 7 — Food chain (Farm → Mill → Bakery → Bread)

**Цель**: построить первую длинную цепочку, которая снабжает ConsumptionSystem дорогим Food.

**Что делается**:
- Farm, Mill, Bakery уже есть в ProductionDefinition (cycleTime=30)
- Settlement правила: BootstrapFarm, BootstrapMill, BootstrapBakery (приоритеты: 400, 450, 500)
- Bread в ConsumptionDefinition: efficiency=3 (1 Bread = 3 единицы добычи)
- ConsumptionSystem предпочитает Bread если доступен

**Проверка**: T47 — полная цепочка Farm→Mill→Bakery→Bread, шахта использует Bread.

### Phase 8 — ResourcePopulationSystem

**Цель**: выделить общий механизм после трёх реализаций.

**Что делается**:
- Анализ Trees vs Animals vs Fish → общие и специфичные части
- `RenewableResourceDefinition` — data-driven описание популяции
- `ResourcePopulationSystem` — единственная система вместо разрозненных проверок в ProductionSystem
- ProductionSystem теряет специальные проверки на Woodcutter/Hunter/Fisher

**Проверка**: T48 — все три ресурса работают через единый механизм, поведение не изменилось.

## 9. Варианты Food для шахт (ConsumptionDefinition)

У шахты будет несколько альтернативных Food:

| Food | Efficiency | Цепочка |
|------|-----------|---------|
| Meat | 1 | Hunter ← Animals (возобновляемый) |
| Fish | 1 | Fisher ← Fish (возобновляемый) |
| Bread | 3 | Farm → Mill → Bakery (3 здания, дорогой) |

ConsumptionSystem выбирает лучший доступный Food (максимальный efficiency), запрашивает через DemandManager.

## 10. Ключевые архитектурные решения

1. **ProductionSystem не рефакторится заранее**. Woodcutter, Hunter, Fisher проверки добавляются как специальные случаи (как сейчас Woodcutter + Forester). После трёх реализаций — выделение общего механизма.

2. **ConsumptionSystem — отдельная система**, не часть ProductionSystem. Шахта — producer (руда через ProductionSystem) + consumer (еда через ConsumptionSystem). Два ортогональных аспекта.

3. **Definition Query API расширяется**: `GetConsumptionDefinition()`, `IsFoodResource()`.

4. **ConsumptionDefinition data-driven**: шахты, виды еды, efficiency — в таблице, не в коде. Добавление новой шахты или еды — только данные.

5. **SettlementSystem не меняет логику**: новые правила следуют тому же паттерну BootstrapXxx с Definition Query API.

6. **Здание — композиция аспектов**:
```
BuildingDefinition
    ├── ProductionDefinition   (трансформация A → B)
    ├── ConsumptionDefinition  (потребление E → enables B)
    ├── ConstructionDefinition (строительство — BuildResourceSlot)
    └── EmploymentDefinition   (рабочие места — будущее)
```
