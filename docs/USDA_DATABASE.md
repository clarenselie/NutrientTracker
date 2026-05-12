# USDA FoodData Central Database Layer

The app stores USDA FoodData Central as normalized SQLite data. Foods keep USDA `fdc_id` as the stable external ID, while the app keeps its own internal `foods.id` so relationships remain stable even when records are updated.

## Schema

```sql
CREATE TABLE foods (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    fdc_id INTEGER UNIQUE,
    description TEXT NOT NULL,
    data_type TEXT,
    food_category TEXT,
    publication_date TEXT,
    source TEXT DEFAULT 'USDA FoodData Central'
);

CREATE TABLE branded_foods (
    food_id INTEGER PRIMARY KEY REFERENCES foods(id) ON DELETE CASCADE,
    brand_owner TEXT,
    brand_name TEXT,
    gtin_upc TEXT,
    ingredients TEXT,
    serving_size REAL,
    serving_size_unit TEXT,
    household_serving_full_text TEXT
);

CREATE TABLE nutrients (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    usda_nutrient_id INTEGER UNIQUE,
    name TEXT NOT NULL,
    unit_name TEXT,
    nutrient_number TEXT,
    rank INTEGER
);

CREATE TABLE food_nutrients (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    food_id INTEGER NOT NULL REFERENCES foods(id) ON DELETE CASCADE,
    nutrient_id INTEGER NOT NULL REFERENCES nutrients(id) ON DELETE CASCADE,
    amount REAL,
    derivation_code TEXT,
    derivation_description TEXT,
    UNIQUE(food_id, nutrient_id)
);

CREATE TABLE serving_sizes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    food_id INTEGER NOT NULL REFERENCES foods(id) ON DELETE CASCADE,
    amount REAL,
    unit TEXT,
    gram_weight REAL,
    description TEXT
);
```

## Indexes

```sql
CREATE INDEX idx_foods_description ON foods(description);
CREATE INDEX idx_foods_fdc_id ON foods(fdc_id);
CREATE INDEX idx_foods_data_type ON foods(data_type);
CREATE INDEX idx_branded_foods_gtin_upc ON branded_foods(gtin_upc);
CREATE INDEX idx_branded_foods_brand_owner ON branded_foods(brand_owner);
CREATE INDEX idx_nutrients_usda_nutrient_id ON nutrients(usda_nutrient_id);
CREATE INDEX idx_food_nutrients_food_id ON food_nutrients(food_id);
```

## Example Usage

```cpp
DatabaseManager database;
QString error;

if (!database.initDatabase(&error)) {
    qWarning() << error;
    return 1;
}

auto foods = database.searchFoods("chicken breast");
if (!foods.isEmpty()) {
    const int foodId = foods.first().id;

    auto nutrientsPer100g = database.getFoodNutrients(foodId);
    auto nutrientsFor150g = database.getFoodNutrientsForGrams(foodId, 150.0);
}
```

## Developer Build Command

Build the USDA-backed SQLite database once, outside normal app startup:

```bash
./build/NutrientTracker.app/Contents/MacOS/NutrientTracker --build-food-db data/usda data/nutrient_tracker.sqlite
```

Then rebuild the app. CMake bundles `data/nutrient_tracker.sqlite`, and the app
silently copies that database into the user's app data folder on first run.

## Import Example

```cpp
DatabaseManager database;
database.initDatabase();

USDAImporter importer(database);
QString error;
if (!importer.importCsvDirectory("/path/to/FoodData_Central_csv", &error)) {
    qWarning() << error;
}
```
