.bail on
.timer on
.mode csv
PRAGMA journal_mode = OFF;
PRAGMA synchronous = OFF;
PRAGMA temp_store = MEMORY;
PRAGMA locking_mode = EXCLUSIVE;
PRAGMA cache_size = -500000;
PRAGMA foreign_keys = OFF;

DROP TABLE IF EXISTS food_nutrients;
DROP TABLE IF EXISTS serving_sizes;
DROP TABLE IF EXISTS branded_foods;
DROP TABLE IF EXISTS nutrients;
DROP TABLE IF EXISTS foods;
DROP TABLE IF EXISTS app_metadata;
DROP TABLE IF EXISTS raw_food;
DROP TABLE IF EXISTS raw_nutrient;
DROP TABLE IF EXISTS raw_food_nutrient;
DROP TABLE IF EXISTS raw_branded_food;
DROP TABLE IF EXISTS raw_food_portion;
DROP TABLE IF EXISTS raw_measure_unit;
DROP TABLE IF EXISTS raw_food_category;
DROP TABLE IF EXISTS raw_derivation;

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

CREATE TABLE app_metadata (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE TABLE raw_food (
    fdc_id TEXT,
    data_type TEXT,
    description TEXT,
    food_category_id TEXT,
    publication_date TEXT
);

CREATE TABLE raw_nutrient (
    id TEXT,
    name TEXT,
    unit_name TEXT,
    nutrient_nbr TEXT,
    rank TEXT
);

CREATE TABLE raw_food_nutrient (
    id TEXT,
    fdc_id TEXT,
    nutrient_id TEXT,
    amount TEXT,
    data_points TEXT,
    derivation_id TEXT,
    min TEXT,
    max TEXT,
    median TEXT,
    loq TEXT,
    footnote TEXT,
    min_year_acquired TEXT,
    percent_daily_value TEXT
);

CREATE TABLE raw_branded_food (
    fdc_id TEXT,
    brand_owner TEXT,
    brand_name TEXT,
    subbrand_name TEXT,
    gtin_upc TEXT,
    ingredients TEXT,
    not_a_significant_source_of TEXT,
    serving_size TEXT,
    serving_size_unit TEXT,
    household_serving_fulltext TEXT,
    branded_food_category TEXT,
    data_source TEXT,
    package_weight TEXT,
    modified_date TEXT,
    available_date TEXT,
    market_country TEXT,
    discontinued_date TEXT,
    preparation_state_code TEXT,
    trade_channel TEXT,
    short_description TEXT,
    material_code TEXT
);

CREATE TABLE raw_food_portion (
    id TEXT,
    fdc_id TEXT,
    seq_num TEXT,
    amount TEXT,
    measure_unit_id TEXT,
    portion_description TEXT,
    modifier TEXT,
    gram_weight TEXT,
    data_points TEXT,
    footnote TEXT,
    min_year_acquired TEXT
);

CREATE TABLE raw_measure_unit (
    id TEXT,
    name TEXT
);

CREATE TABLE raw_food_category (
    id TEXT,
    code TEXT,
    description TEXT
);

CREATE TABLE raw_derivation (
    id TEXT,
    code TEXT,
    description TEXT
);

.print Importing lookup tables...
.import --skip 1 data/usda/measure_unit.csv raw_measure_unit
.import --skip 1 data/usda/food_category.csv raw_food_category
.import --skip 1 data/usda/food_nutrient_derivation.csv raw_derivation

.print Importing foods...
.import --skip 1 data/usda/food.csv raw_food

INSERT INTO foods (fdc_id, description, data_type, food_category, publication_date, source)
SELECT
    CAST(raw_food.fdc_id AS INTEGER),
    raw_food.description,
    raw_food.data_type,
    COALESCE(raw_food_category.description, NULLIF(raw_food.food_category_id, '')),
    raw_food.publication_date,
    'USDA FoodData Central'
FROM raw_food
LEFT JOIN raw_food_category ON raw_food_category.id = raw_food.food_category_id
WHERE CAST(raw_food.fdc_id AS INTEGER) > 0
  AND TRIM(raw_food.description) <> '';

DROP TABLE raw_food;
DROP TABLE raw_food_category;

.print Importing nutrients...
.import --skip 1 data/usda/nutrient.csv raw_nutrient

INSERT INTO nutrients (usda_nutrient_id, name, unit_name, nutrient_number, rank)
SELECT
    CAST(id AS INTEGER),
    name,
    unit_name,
    nutrient_nbr,
    CAST(rank AS INTEGER)
FROM raw_nutrient
WHERE CAST(id AS INTEGER) > 0
  AND TRIM(name) <> '';

DROP TABLE raw_nutrient;

.print Importing branded food metadata...
.import --skip 1 data/usda/branded_food.csv raw_branded_food

INSERT OR REPLACE INTO branded_foods (
    food_id,
    brand_owner,
    brand_name,
    gtin_upc,
    ingredients,
    serving_size,
    serving_size_unit,
    household_serving_full_text
)
SELECT
    foods.id,
    raw_branded_food.brand_owner,
    raw_branded_food.brand_name,
    raw_branded_food.gtin_upc,
    raw_branded_food.ingredients,
    CAST(NULLIF(raw_branded_food.serving_size, '') AS REAL),
    raw_branded_food.serving_size_unit,
    raw_branded_food.household_serving_fulltext
FROM raw_branded_food
JOIN foods ON foods.fdc_id = CAST(raw_branded_food.fdc_id AS INTEGER);

INSERT INTO serving_sizes (food_id, amount, unit, gram_weight, description)
SELECT
    foods.id,
    1.0,
    'serving',
    CAST(raw_branded_food.serving_size AS REAL),
    raw_branded_food.household_serving_fulltext
FROM raw_branded_food
JOIN foods ON foods.fdc_id = CAST(raw_branded_food.fdc_id AS INTEGER)
WHERE LOWER(raw_branded_food.serving_size_unit) = 'g'
  AND CAST(raw_branded_food.serving_size AS REAL) > 0;

DROP TABLE raw_branded_food;

.print Importing serving sizes...
.import --skip 1 data/usda/food_portion.csv raw_food_portion

INSERT INTO serving_sizes (food_id, amount, unit, gram_weight, description)
SELECT
    foods.id,
    CAST(NULLIF(raw_food_portion.amount, '') AS REAL),
    raw_measure_unit.name,
    CAST(NULLIF(raw_food_portion.gram_weight, '') AS REAL),
    COALESCE(NULLIF(raw_food_portion.portion_description, ''), raw_food_portion.modifier)
FROM raw_food_portion
JOIN foods ON foods.fdc_id = CAST(raw_food_portion.fdc_id AS INTEGER)
LEFT JOIN raw_measure_unit ON raw_measure_unit.id = raw_food_portion.measure_unit_id
WHERE CAST(raw_food_portion.gram_weight AS REAL) > 0;

DROP TABLE raw_food_portion;
DROP TABLE raw_measure_unit;

.print Importing food nutrient values...
.import --skip 1 data/usda/food_nutrient.csv raw_food_nutrient

INSERT OR REPLACE INTO food_nutrients (
    food_id,
    nutrient_id,
    amount,
    derivation_code,
    derivation_description
)
SELECT
    foods.id,
    nutrients.id,
    CAST(NULLIF(raw_food_nutrient.amount, '') AS REAL),
    raw_derivation.code,
    raw_derivation.description
FROM raw_food_nutrient
JOIN foods ON foods.fdc_id = CAST(raw_food_nutrient.fdc_id AS INTEGER)
JOIN nutrients ON nutrients.usda_nutrient_id = CAST(raw_food_nutrient.nutrient_id AS INTEGER)
LEFT JOIN raw_derivation ON raw_derivation.id = raw_food_nutrient.derivation_id;

DROP TABLE raw_food_nutrient;
DROP TABLE raw_derivation;

.print Creating indexes...
CREATE INDEX idx_foods_description ON foods(description);
CREATE INDEX idx_foods_fdc_id ON foods(fdc_id);
CREATE INDEX idx_foods_data_type ON foods(data_type);
CREATE INDEX idx_branded_foods_gtin_upc ON branded_foods(gtin_upc);
CREATE INDEX idx_branded_foods_brand_owner ON branded_foods(brand_owner);
CREATE INDEX idx_nutrients_usda_nutrient_id ON nutrients(usda_nutrient_id);
CREATE INDEX idx_food_nutrients_food_id ON food_nutrients(food_id);

INSERT OR REPLACE INTO app_metadata (key, value) VALUES ('schema_version', '1');
PRAGMA foreign_keys = ON;
PRAGMA locking_mode = NORMAL;
PRAGMA journal_mode = DELETE;
PRAGMA synchronous = NORMAL;

.print USDA SQLite database build complete.
