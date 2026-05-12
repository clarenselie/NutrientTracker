#include "DatabaseManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

namespace {
constexpr int kSchemaVersion = 1;
constexpr const char *kSourceName = "USDA FoodData Central";

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

void bindNullableDouble(QSqlQuery &query, int index, double value)
{
    if (value == 0.0) {
        query.bindValue(index, QVariant());
    } else {
        query.bindValue(index, value);
    }
}

QString canonicalKeyForNutrient(int usdaNutrientId, const QString &name, const QString &unit)
{
    switch (usdaNutrientId) {
    case 1008:
        return "calories";
    case 1003:
        return "protein";
    case 1005:
        return "carbs";
    case 1004:
        return "fat";
    case 1079:
        return "fiber";
    case 1063:
    case 2000:
        return "sugar";
    case 1051:
        return "water";
    case 1087:
        return "calcium";
    case 1088:
        return "chloride";
    case 1089:
        return "iron";
    case 1090:
        return "magnesium";
    case 1091:
        return "phosphorus";
    case 1092:
        return "potassium";
    case 1093:
        return "sodium";
    case 1095:
        return "zinc";
    case 1096:
        return "chromium";
    case 1098:
        return "copper";
    case 1100:
        return "iodine";
    case 1101:
        return "manganese";
    case 1102:
        return "molybdenum";
    case 1103:
        return "selenium";
    case 1106:
    case 2067:
        return "vitamin_a";
    case 1109:
    case 2068:
        return "vitamin_e";
    case 1114:
        return "vitamin_d";
    case 1162:
        return "vitamin_c";
    case 1165:
        return "b1";
    case 1166:
        return "b2";
    case 1167:
        return "b3";
    case 1170:
        return "b5";
    case 1175:
        return "b6";
    case 1176:
        return "b7";
    case 1177:
        return "b9";
    case 1178:
        return "b12";
    case 1185:
        return "vitamin_k";
    case 1253:
        return "cholesterol";
    case 1258:
        return "saturated_fat";
    case 1272:
        return "dha";
    case 1278:
        return "epa";
    case 1292:
        return "monounsaturated_fat";
    case 1293:
        return "polyunsaturated_fat";
    case 1316:
    case 1321:
    case 1406:
    case 1408:
        return "omega6";
    case 1404:
        return "ala";
    case 1057:
        return "caffeine";
    default:
        break;
    }

    const QString lowerName = name.toLower();
    const QString normalizedUnit = unit.toLower();
    if (lowerName == "energy" && normalizedUnit == "kcal") {
        return "calories";
    }
    if (lowerName == "protein") {
        return "protein";
    }
    if (lowerName == "carbohydrate, by difference" || lowerName == "carbohydrates") {
        return "carbs";
    }
    if (lowerName == "total lipid (fat)" || lowerName == "fat") {
        return "fat";
    }
    if (lowerName == "fiber, total dietary") {
        return "fiber";
    }

    return QString::number(usdaNutrientId);
}

void applyTrackedNutrientMetadata(NutrientValue *value)
{
    if (!value) {
        return;
    }

    const TrackedNutrientDefinition definition = trackedNutrientDefinitionForKey(value->key);
    if (definition.key.isEmpty()) {
        value->category = "Other";
        value->subcategory.clear();
        value->displayOrder = 100000 + value->displayOrder;
        return;
    }

    value->label = definition.label;
    value->category = definition.category;
    value->subcategory = definition.subcategory;
    value->displayOrder = definition.displayOrder;
    if (value->unit.trimmed().isEmpty()) {
        value->unit = definition.defaultUnit;
    }
}
}

DatabaseManager::DatabaseManager(const QString &databasePath)
    : m_databasePathOverride(databasePath)
{
    m_connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

DatabaseManager::~DatabaseManager()
{
    const QString connectionName = m_connectionName;
    if (m_db.isValid()) {
        m_db.close();
    }
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool DatabaseManager::initDatabase(QString *errorMessage)
{
    if (m_db.isValid() && m_db.isOpen()) {
        return true;
    }

    if (!m_db.isValid()) {
        m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    }
    m_db.setDatabaseName(databasePath());

    if (!m_db.open()) {
        setError(errorMessage, m_db.lastError().text());
        return false;
    }

    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA foreign_keys = ON");

    return ensureSchema(errorMessage);
}

bool DatabaseManager::initialize(QString *errorMessage)
{
    return initDatabase(errorMessage);
}

QString DatabaseManager::databaseFilePath() const
{
    return databasePath();
}

bool DatabaseManager::installDatabaseFromFile(const QString &sourcePath, QString *errorMessage)
{
    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        setError(errorMessage, "Bundled database file does not exist: " + sourcePath);
        return false;
    }

    const QString targetPath = databasePath();
    const QFileInfo targetInfo(targetPath);
    QDir targetDirectory(targetInfo.absolutePath());
    if (!targetDirectory.exists() && !targetDirectory.mkpath(".")) {
        setError(errorMessage, "Could not create database directory: " + targetInfo.absolutePath());
        return false;
    }

    if (m_db.isValid() && m_db.isOpen()) {
        m_db.close();
    }

    const QStringList databaseSidecars = {
        targetPath,
        targetPath + "-wal",
        targetPath + "-shm"
    };
    for (const QString &path : databaseSidecars) {
        if (QFile::exists(path) && !QFile::remove(path)) {
            setError(errorMessage, "Could not replace existing database file: " + path);
            return false;
        }
    }

    if (!QFile::copy(sourcePath, targetPath)) {
        setError(errorMessage, "Could not copy bundled food database into app data.");
        return false;
    }

    QFile::setPermissions(
        targetPath,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner
            | QFileDevice::ReadUser | QFileDevice::WriteUser
            | QFileDevice::ReadGroup | QFileDevice::ReadOther);

    return initDatabase(errorMessage);
}

bool DatabaseManager::upsertFood(const USDAFood &food, int *foodId, QString *errorMessage)
{
    if (food.fdcId <= 0 || food.description.trimmed().isEmpty()) {
        setError(errorMessage, "Food requires a positive FDC ID and non-empty description.");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO foods (fdc_id, description, data_type, food_category, publication_date, source) "
        "VALUES (?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(fdc_id) DO UPDATE SET "
        "description = excluded.description, "
        "data_type = excluded.data_type, "
        "food_category = excluded.food_category, "
        "publication_date = excluded.publication_date, "
        "source = excluded.source");
    query.addBindValue(food.fdcId);
    query.addBindValue(food.description);
    query.addBindValue(food.dataType);
    query.addBindValue(food.foodCategory);
    query.addBindValue(food.publicationDate);
    query.addBindValue(food.source.isEmpty() ? kSourceName : food.source);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    if (foodId) {
        *foodId = findFoodIdByFdcId(food.fdcId);
    }
    return true;
}

bool DatabaseManager::upsertBrandedFood(const USDABrandedFood &brandedFood, QString *errorMessage)
{
    if (brandedFood.foodId <= 0) {
        setError(errorMessage, "Branded food metadata requires an internal food ID.");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO branded_foods (food_id, brand_owner, brand_name, gtin_upc, ingredients, "
        "serving_size, serving_size_unit, household_serving_full_text) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(food_id) DO UPDATE SET "
        "brand_owner = excluded.brand_owner, "
        "brand_name = excluded.brand_name, "
        "gtin_upc = excluded.gtin_upc, "
        "ingredients = excluded.ingredients, "
        "serving_size = excluded.serving_size, "
        "serving_size_unit = excluded.serving_size_unit, "
        "household_serving_full_text = excluded.household_serving_full_text");
    query.addBindValue(brandedFood.foodId);
    query.addBindValue(brandedFood.brandOwner);
    query.addBindValue(brandedFood.brandName);
    query.addBindValue(brandedFood.gtinUpc);
    query.addBindValue(brandedFood.ingredients);
    bindNullableDouble(query, 5, brandedFood.servingSize);
    query.addBindValue(brandedFood.servingSizeUnit);
    query.addBindValue(brandedFood.householdServingFullText);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }
    return true;
}

bool DatabaseManager::upsertNutrient(const USDANutrient &nutrient, int *nutrientId, QString *errorMessage)
{
    if (nutrient.usdaNutrientId <= 0 || nutrient.name.trimmed().isEmpty()) {
        setError(errorMessage, "Nutrient requires a USDA nutrient ID and non-empty name.");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO nutrients (usda_nutrient_id, name, unit_name, nutrient_number, rank) "
        "VALUES (?, ?, ?, ?, ?) "
        "ON CONFLICT(usda_nutrient_id) DO UPDATE SET "
        "name = excluded.name, "
        "unit_name = excluded.unit_name, "
        "nutrient_number = excluded.nutrient_number, "
        "rank = excluded.rank");
    query.addBindValue(nutrient.usdaNutrientId);
    query.addBindValue(nutrient.name);
    query.addBindValue(nutrient.unitName);
    query.addBindValue(nutrient.nutrientNumber);
    query.addBindValue(nutrient.rank);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    if (nutrientId) {
        *nutrientId = findNutrientIdByUsdaNutrientId(nutrient.usdaNutrientId);
    }
    return true;
}

bool DatabaseManager::upsertFoodNutrient(const USDAFoodNutrient &foodNutrient, QString *errorMessage)
{
    if (foodNutrient.foodId <= 0 || foodNutrient.nutrientId <= 0) {
        setError(errorMessage, "Food nutrient value requires internal food and nutrient IDs.");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO food_nutrients (food_id, nutrient_id, amount, derivation_code, derivation_description) "
        "VALUES (?, ?, ?, ?, ?) "
        "ON CONFLICT(food_id, nutrient_id) DO UPDATE SET "
        "amount = excluded.amount, "
        "derivation_code = excluded.derivation_code, "
        "derivation_description = excluded.derivation_description");
    query.addBindValue(foodNutrient.foodId);
    query.addBindValue(foodNutrient.nutrientId);
    query.bindValue(2, foodNutrient.amount);
    query.addBindValue(foodNutrient.derivationCode);
    query.addBindValue(foodNutrient.derivationDescription);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }
    return true;
}

bool DatabaseManager::upsertServingSize(const USDAServingSize &servingSize, int *servingId, QString *errorMessage)
{
    if (servingSize.foodId <= 0) {
        setError(errorMessage, "Serving size requires an internal food ID.");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO serving_sizes (food_id, amount, unit, gram_weight, description) "
        "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(servingSize.foodId);
    bindNullableDouble(query, 1, servingSize.amount);
    query.addBindValue(servingSize.unit);
    bindNullableDouble(query, 3, servingSize.gramWeight);
    query.addBindValue(servingSize.description);

    if (!query.exec()) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    if (servingId) {
        *servingId = query.lastInsertId().toInt();
    }
    return true;
}

QVector<FoodRecord> DatabaseManager::searchFoods(const QString &term, const QString &brand) const
{
    QVector<FoodRecord> results;
    if (!m_db.isOpen() || term.trimmed().isEmpty()) {
        return results;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT f.id, f.fdc_id, f.description, COALESCE(b.brand_name, b.brand_owner, ''), "
        "f.data_type, f.food_category, f.publication_date, f.source, "
        "COALESCE((SELECT gram_weight FROM serving_sizes s WHERE s.food_id = f.id AND s.gram_weight IS NOT NULL LIMIT 1), "
        "CASE WHEN LOWER(b.serving_size_unit) = 'g' THEN b.serving_size ELSE NULL END, 100.0) AS grams_per_unit "
        "FROM foods f "
        "LEFT JOIN branded_foods b ON b.food_id = f.id "
        "WHERE f.description LIKE ? "
        "AND (? = '' OR LOWER(COALESCE(b.brand_owner, '') || ' ' || COALESCE(b.brand_name, '')) LIKE ?) "
        "ORDER BY CASE WHEN LOWER(f.description) = LOWER(?) THEN 0 ELSE 1 END, f.description ASC "
        "LIMIT 50");
    query.addBindValue("%" + term.trimmed() + "%");
    const QString normalizedBrand = brand.toLower().trimmed();
    query.addBindValue(normalizedBrand);
    query.addBindValue("%" + normalizedBrand + "%");
    query.addBindValue(term.trimmed());

    if (!query.exec()) {
        return results;
    }

    while (query.next()) {
        results.append(recordFromQuery(query));
    }

    return results;
}

std::optional<FoodRecord> DatabaseManager::searchBrandedFoodByBarcode(const QString &barcode) const
{
    if (!m_db.isOpen() || barcode.trimmed().isEmpty()) {
        return std::nullopt;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT f.id, f.fdc_id, f.description, COALESCE(b.brand_name, b.brand_owner, ''), "
        "f.data_type, f.food_category, f.publication_date, f.source, "
        "COALESCE(CASE WHEN LOWER(b.serving_size_unit) = 'g' THEN b.serving_size ELSE NULL END, 100.0) AS grams_per_unit "
        "FROM branded_foods b "
        "JOIN foods f ON f.id = b.food_id "
        "WHERE b.gtin_upc = ? "
        "LIMIT 1");
    query.addBindValue(barcode.trimmed());

    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }

    return recordFromQuery(query);
}

QVector<NutrientValue> DatabaseManager::getFoodNutrients(int foodId) const
{
    QVector<NutrientValue> nutrients;
    if (!m_db.isOpen() || foodId <= 0) {
        return nutrients;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT n.usda_nutrient_id, n.name, n.unit_name, n.rank, fn.amount "
        "FROM food_nutrients fn "
        "JOIN nutrients n ON n.id = fn.nutrient_id "
        "WHERE fn.food_id = ? "
        "ORDER BY COALESCE(n.rank, 999999), n.name");
    query.addBindValue(foodId);

    if (!query.exec()) {
        return nutrients;
    }

    while (query.next()) {
        NutrientValue value;
        value.usdaNutrientId = query.value(0).toInt();
        value.label = query.value(1).toString();
        value.unit = query.value(2).toString();
        value.displayOrder = query.value(3).toInt();
        value.key = canonicalKeyForNutrient(value.usdaNutrientId, value.label, value.unit);
        value.amount = query.value(4).isNull() ? 0.0 : query.value(4).toDouble();
        applyTrackedNutrientMetadata(&value);
        nutrients.append(value);
    }

    return nutrients;
}

QVector<NutrientValue> DatabaseManager::getFoodNutrientsForGrams(int foodId, double grams) const
{
    if (grams <= 0.0) {
        return {};
    }
    return scaleNutrients(getFoodNutrients(foodId), grams);
}

QVector<NutrientValue> DatabaseManager::getFoodNutrientsForServing(int servingId, double servings) const
{
    if (!m_db.isOpen() || servingId <= 0 || servings <= 0.0) {
        return {};
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT food_id, gram_weight FROM serving_sizes WHERE id = ?");
    query.addBindValue(servingId);

    if (!query.exec() || !query.next() || query.value(1).isNull()) {
        return {};
    }

    const int foodId = query.value(0).toInt();
    const double grams = query.value(1).toDouble() * servings;
    return getFoodNutrientsForGrams(foodId, grams);
}

QVector<USDAServingSize> DatabaseManager::getServingSizes(int foodId) const
{
    QVector<USDAServingSize> servings;
    if (!m_db.isOpen() || foodId <= 0) {
        return servings;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT id, food_id, amount, unit, gram_weight, description "
        "FROM serving_sizes WHERE food_id = ? ORDER BY id");
    query.addBindValue(foodId);

    if (!query.exec()) {
        return servings;
    }

    while (query.next()) {
        USDAServingSize serving;
        serving.id = query.value(0).toInt();
        serving.foodId = query.value(1).toInt();
        serving.amount = query.value(2).isNull() ? 0.0 : query.value(2).toDouble();
        serving.unit = query.value(3).toString();
        serving.gramWeight = query.value(4).isNull() ? 0.0 : query.value(4).toDouble();
        serving.description = query.value(5).toString();
        servings.append(serving);
    }

    return servings;
}

bool DatabaseManager::hasImportedUSDAData() const
{
    if (!m_db.isOpen()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM foods WHERE source = ?");
    query.addBindValue(kSourceName);

    if (!query.exec() || !query.next()) {
        return false;
    }

    return query.value(0).toInt() > 0;
}

int DatabaseManager::findFoodIdByFdcId(int fdcId) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM foods WHERE fdc_id = ?");
    query.addBindValue(fdcId);
    if (!query.exec() || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

int DatabaseManager::findNutrientIdByUsdaNutrientId(int usdaNutrientId) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM nutrients WHERE usda_nutrient_id = ?");
    query.addBindValue(usdaNutrientId);
    if (!query.exec() || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

bool DatabaseManager::beginTransaction(QString *errorMessage)
{
    if (!m_db.transaction()) {
        setError(errorMessage, m_db.lastError().text());
        return false;
    }
    return true;
}

bool DatabaseManager::commitTransaction(QString *errorMessage)
{
    if (!m_db.commit()) {
        setError(errorMessage, m_db.lastError().text());
        return false;
    }
    return true;
}

void DatabaseManager::rollbackTransaction()
{
    m_db.rollback();
}

QString DatabaseManager::databasePath() const
{
    if (!m_databasePathOverride.trimmed().isEmpty()) {
        const QFileInfo databaseInfo(m_databasePathOverride);
        QDir directory(databaseInfo.absolutePath());
        directory.mkpath(".");
        return databaseInfo.absoluteFilePath();
    }

    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QCoreApplication::applicationDirPath();
    }

    QDir dir(baseDir);
    dir.mkpath(".");
    return dir.filePath("nutrient_tracker.sqlite");
}

bool DatabaseManager::createTables(QString *errorMessage) const
{
    // Normalized USDA schema:
    // foods = stable food identity by FDC ID.
    // branded_foods = optional branded metadata keyed one-to-one to foods.
    // nutrients = USDA nutrient dictionary preserving USDA nutrient IDs.
    // food_nutrients = per-100g nutrient values joining foods and nutrients.
    // serving_sizes = gram weights used to scale servings into nutrient totals.
    const QStringList statements = {
        "CREATE TABLE IF NOT EXISTS foods ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fdc_id INTEGER UNIQUE,"
        "description TEXT NOT NULL,"
        "data_type TEXT,"
        "food_category TEXT,"
        "publication_date TEXT,"
        "source TEXT DEFAULT 'USDA FoodData Central'"
        ")",
        "CREATE TABLE IF NOT EXISTS branded_foods ("
        "food_id INTEGER PRIMARY KEY REFERENCES foods(id) ON DELETE CASCADE,"
        "brand_owner TEXT,"
        "brand_name TEXT,"
        "gtin_upc TEXT,"
        "ingredients TEXT,"
        "serving_size REAL,"
        "serving_size_unit TEXT,"
        "household_serving_full_text TEXT"
        ")",
        "CREATE TABLE IF NOT EXISTS nutrients ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "usda_nutrient_id INTEGER UNIQUE,"
        "name TEXT NOT NULL,"
        "unit_name TEXT,"
        "nutrient_number TEXT,"
        "rank INTEGER"
        ")",
        "CREATE TABLE IF NOT EXISTS food_nutrients ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "food_id INTEGER NOT NULL REFERENCES foods(id) ON DELETE CASCADE,"
        "nutrient_id INTEGER NOT NULL REFERENCES nutrients(id) ON DELETE CASCADE,"
        "amount REAL,"
        "derivation_code TEXT,"
        "derivation_description TEXT,"
        "UNIQUE(food_id, nutrient_id)"
        ")",
        "CREATE TABLE IF NOT EXISTS serving_sizes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "food_id INTEGER NOT NULL REFERENCES foods(id) ON DELETE CASCADE,"
        "amount REAL,"
        "unit TEXT,"
        "gram_weight REAL,"
        "description TEXT"
        ")",
        "CREATE TABLE IF NOT EXISTS app_metadata ("
        "key TEXT PRIMARY KEY,"
        "value TEXT NOT NULL"
        ")"
    };

    for (const QString &statement : statements) {
        QSqlQuery query(m_db);
        if (!query.exec(statement)) {
            setError(errorMessage, query.lastError().text());
            return false;
        }
    }

    QSqlQuery versionQuery(m_db);
    versionQuery.prepare("INSERT OR REPLACE INTO app_metadata (key, value) VALUES ('schema_version', ?)");
    versionQuery.addBindValue(QString::number(kSchemaVersion));
    if (!versionQuery.exec()) {
        setError(errorMessage, versionQuery.lastError().text());
        return false;
    }

    return true;
}

bool DatabaseManager::createIndexes(QString *errorMessage) const
{
    const QStringList statements = {
        "CREATE INDEX IF NOT EXISTS idx_foods_description ON foods(description)",
        "CREATE INDEX IF NOT EXISTS idx_foods_fdc_id ON foods(fdc_id)",
        "CREATE INDEX IF NOT EXISTS idx_foods_data_type ON foods(data_type)",
        "CREATE INDEX IF NOT EXISTS idx_branded_foods_gtin_upc ON branded_foods(gtin_upc)",
        "CREATE INDEX IF NOT EXISTS idx_branded_foods_brand_owner ON branded_foods(brand_owner)",
        "CREATE INDEX IF NOT EXISTS idx_nutrients_usda_nutrient_id ON nutrients(usda_nutrient_id)",
        "CREATE INDEX IF NOT EXISTS idx_food_nutrients_food_id ON food_nutrients(food_id)"
    };

    for (const QString &statement : statements) {
        QSqlQuery query(m_db);
        if (!query.exec(statement)) {
            setError(errorMessage, query.lastError().text());
            return false;
        }
    }
    return true;
}

bool DatabaseManager::ensureSchema(QString *errorMessage)
{
    bool needsRebuild = tableExists("foods")
        && (!tableHasColumn("foods", "fdc_id") || !tableHasColumn("foods", "description"));

    QSqlQuery schemaQuery(m_db);
    schemaQuery.prepare("SELECT sql FROM sqlite_master WHERE type = 'table' AND name = ?");
    schemaQuery.addBindValue("foods");
    if (schemaQuery.exec() && schemaQuery.next()) {
        const QString foodSchema = schemaQuery.value(0).toString().toLower();
        needsRebuild = !foodSchema.contains("fdc_id") || !foodSchema.contains("description");
    }

    schemaQuery.prepare("SELECT sql FROM sqlite_master WHERE type = 'table' AND name = ?");
    schemaQuery.addBindValue("nutrients");
    if (!needsRebuild && schemaQuery.exec() && schemaQuery.next()) {
        const QString nutrientSchema = schemaQuery.value(0).toString().toLower();
        needsRebuild = !nutrientSchema.contains("usda_nutrient_id");
    }

    if (needsRebuild) {
        if (!dropKnownTables(errorMessage)) {
            return false;
        }
    }

    if (!createTables(errorMessage)) {
        return false;
    }

    return createIndexes(errorMessage);
}

bool DatabaseManager::dropKnownTables(QString *errorMessage) const
{
    const QStringList statements = {
        "DROP TABLE IF EXISTS food_nutrients",
        "DROP TABLE IF EXISTS serving_sizes",
        "DROP TABLE IF EXISTS branded_foods",
        "DROP TABLE IF EXISTS nutrients",
        "DROP TABLE IF EXISTS foods",
        "DROP TABLE IF EXISTS app_metadata"
    };

    for (const QString &statement : statements) {
        QSqlQuery query(m_db);
        if (!query.exec(statement)) {
            setError(errorMessage, query.lastError().text());
            return false;
        }
    }
    return true;
}

bool DatabaseManager::tableExists(const QString &tableName) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ?");
    query.addBindValue(tableName);
    return query.exec() && query.next();
}

bool DatabaseManager::tableHasColumn(const QString &tableName, const QString &columnName) const
{
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA table_info(" + tableName + ")")) {
        return false;
    }

    while (query.next()) {
        if (query.value(1).toString() == columnName) {
            return true;
        }
    }
    return false;
}

FoodRecord DatabaseManager::recordFromQuery(const QSqlQuery &query) const
{
    FoodRecord food;
    food.id = query.value(0).toInt();
    food.fdcId = query.value(1).toInt();
    food.name = query.value(2).toString();
    food.brand = query.value(3).toString();
    food.searchName = (food.name + " " + food.brand).trimmed();
    food.dataType = query.value(4).toString();
    food.foodCategory = query.value(5).toString();
    food.publicationDate = query.value(6).toString();
    food.source = query.value(7).toString();
    food.gramsPerUnit = query.value(8).isNull() ? 100.0 : query.value(8).toDouble();
    food.gramsPerCup = food.gramsPerUnit;
    return food;
}

QVector<NutrientValue> DatabaseManager::scaleNutrients(const QVector<NutrientValue> &nutrients, double grams) const
{
    QVector<NutrientValue> scaled;
    scaled.reserve(nutrients.size());

    const double factor = grams / 100.0;
    for (const NutrientValue &nutrient : nutrients) {
        NutrientValue value = nutrient;
        value.amount = nutrient.amount * factor;
        scaled.append(value);
    }

    return scaled;
}
