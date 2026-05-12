#include "USDAImporter.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

namespace {
void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}
}

USDAImporter::USDAImporter(DatabaseManager &databaseManager)
    : m_databaseManager(databaseManager)
{
}

bool USDAImporter::importCsvDirectory(const QString &directoryPath, QString *errorMessage)
{
    if (!m_databaseManager.initDatabase(errorMessage)) {
        return false;
    }

    QDir directory(directoryPath);
    if (!directory.exists()) {
        setError(errorMessage, "USDA import directory does not exist: " + directoryPath);
        return false;
    }

    if (!m_databaseManager.beginTransaction(errorMessage)) {
        return false;
    }

    const QString foodCategoryFile = firstExistingFile(directoryPath, {"food_category.csv"});
    const QString measureUnitFile = firstExistingFile(directoryPath, {"measure_unit.csv"});
    const QString derivationFile = firstExistingFile(directoryPath, {"food_nutrient_derivation.csv"});
    const QString foodFile = firstExistingFile(directoryPath, {"food.csv"});
    const QString nutrientFile = firstExistingFile(directoryPath, {"nutrient.csv"});
    const QString brandedFile = firstExistingFile(directoryPath, {"branded_food.csv"});
    const QString foodNutrientFile = firstExistingFile(directoryPath, {"food_nutrient.csv"});
    const QString foodPortionFile = firstExistingFile(directoryPath, {"food_portion.csv"});

    if (foodFile.isEmpty() || nutrientFile.isEmpty() || foodNutrientFile.isEmpty()) {
        m_databaseManager.rollbackTransaction();
        setError(errorMessage, "Selected folder must contain food.csv, nutrient.csv, and food_nutrient.csv.");
        return false;
    }

    bool ok = true;
    if (!foodCategoryFile.isEmpty()) {
        ok = importFoodCategories(foodCategoryFile, errorMessage);
    }
    if (ok && !measureUnitFile.isEmpty()) {
        ok = importMeasureUnits(measureUnitFile, errorMessage);
    }
    if (ok && !derivationFile.isEmpty()) {
        ok = importDerivations(derivationFile, errorMessage);
    }
    if (ok && !foodFile.isEmpty()) {
        ok = importFoods(foodFile, errorMessage);
    }
    if (ok && !nutrientFile.isEmpty()) {
        ok = importNutrients(nutrientFile, errorMessage);
    }
    if (ok && !brandedFile.isEmpty()) {
        ok = importBrandedFoods(brandedFile, errorMessage);
    }
    if (ok && !foodNutrientFile.isEmpty()) {
        ok = importFoodNutrients(foodNutrientFile, errorMessage);
    }
    if (ok && !foodPortionFile.isEmpty()) {
        ok = importServingSizes(foodPortionFile, errorMessage);
    }

    if (!ok) {
        m_databaseManager.rollbackTransaction();
        return false;
    }

    return m_databaseManager.commitTransaction(errorMessage);
}

bool USDAImporter::importFoodCategories(const QString &filePath, QString *errorMessage)
{
    QVector<CsvRow> rows;
    if (!readCsvRows(filePath, &rows, errorMessage)) {
        return false;
    }

    for (const CsvRow &row : rows) {
        m_foodCategoriesById.insert(toInt(value(row, "id")), value(row, "description"));
    }
    return true;
}

bool USDAImporter::importMeasureUnits(const QString &filePath, QString *errorMessage)
{
    QVector<CsvRow> rows;
    if (!readCsvRows(filePath, &rows, errorMessage)) {
        return false;
    }

    for (const CsvRow &row : rows) {
        m_measureUnitsById.insert(toInt(value(row, "id")), value(row, "name"));
    }
    return true;
}

bool USDAImporter::importDerivations(const QString &filePath, QString *errorMessage)
{
    QVector<CsvRow> rows;
    if (!readCsvRows(filePath, &rows, errorMessage)) {
        return false;
    }

    for (const CsvRow &row : rows) {
        m_derivationsById.insert(
            toInt(value(row, "id")),
            qMakePair(value(row, "code"), value(row, "description")));
    }
    return true;
}

bool USDAImporter::importFoods(const QString &filePath, QString *errorMessage)
{
    int importedCount = 0;
    int skippedCount = 0;

    const bool ok = forEachCsvRow(filePath, [&](const CsvRow &row) {
        const int fdcId = toInt(value(row, "fdc_id"));
        const QString description = value(row, "description");
        if (fdcId <= 0 || description.isEmpty()) {
            ++skippedCount;
            return true;
        }

        USDAFood food;
        food.fdcId = fdcId;
        food.description = description;
        food.dataType = value(row, "data_type");
        food.publicationDate = value(row, "publication_date");

        const QString categoryValue = value(row, "food_category_id");
        const int categoryId = toInt(categoryValue);
        food.foodCategory = m_foodCategoriesById.value(categoryId, categoryValue);

        int foodId = -1;
        if (!m_databaseManager.upsertFood(food, &foodId, errorMessage)) {
            return false;
        }
        m_foodIdsByFdcId.insert(food.fdcId, foodId);
        ++importedCount;
        return true;
    }, errorMessage);

    if (!ok) {
        return false;
    }

    if (importedCount == 0) {
        setError(
            errorMessage,
            QString("No valid foods were found in %1. Skipped %2 row(s).")
                .arg(filePath)
                .arg(skippedCount));
        return false;
    }

    return true;
}

bool USDAImporter::importNutrients(const QString &filePath, QString *errorMessage)
{
    int importedCount = 0;
    int skippedCount = 0;

    const bool ok = forEachCsvRow(filePath, [&](const CsvRow &row) {
        const int usdaNutrientId = toInt(value(row, "id"));
        const QString name = value(row, "name");
        if (usdaNutrientId <= 0 || name.isEmpty()) {
            ++skippedCount;
            return true;
        }

        USDANutrient nutrient;
        nutrient.usdaNutrientId = usdaNutrientId;
        nutrient.name = name;
        nutrient.unitName = value(row, "unit_name");
        nutrient.nutrientNumber = value(row, "nutrient_nbr");
        nutrient.rank = toInt(value(row, "rank"));

        int nutrientId = -1;
        if (!m_databaseManager.upsertNutrient(nutrient, &nutrientId, errorMessage)) {
            return false;
        }
        m_nutrientIdsByUsdaId.insert(nutrient.usdaNutrientId, nutrientId);
        ++importedCount;
        return true;
    }, errorMessage);

    if (!ok) {
        return false;
    }

    if (importedCount == 0) {
        setError(
            errorMessage,
            QString("No valid nutrients were found in %1. Skipped %2 row(s).")
                .arg(filePath)
                .arg(skippedCount));
        return false;
    }

    return true;
}

bool USDAImporter::importBrandedFoods(const QString &filePath, QString *errorMessage)
{
    return forEachCsvRow(filePath, [&](const CsvRow &row) {
        const int foodId = foodIdFromFdcId(toInt(value(row, "fdc_id")));
        if (foodId <= 0) {
            return true;
        }

        USDABrandedFood brandedFood;
        brandedFood.foodId = foodId;
        brandedFood.brandOwner = value(row, "brand_owner");
        brandedFood.brandName = value(row, "brand_name");
        brandedFood.gtinUpc = value(row, "gtin_upc");
        brandedFood.ingredients = value(row, "ingredients");
        brandedFood.servingSize = toDouble(value(row, "serving_size"));
        brandedFood.servingSizeUnit = value(row, "serving_size_unit");
        brandedFood.householdServingFullText = value(row, "household_serving_full_text");

        if (!m_databaseManager.upsertBrandedFood(brandedFood, errorMessage)) {
            return false;
        }

        if (brandedFood.servingSize > 0.0 && brandedFood.servingSizeUnit.toLower() == "g") {
            USDAServingSize serving;
            serving.foodId = foodId;
            serving.amount = 1.0;
            serving.unit = "serving";
            serving.gramWeight = brandedFood.servingSize;
            serving.description = brandedFood.householdServingFullText;
            if (!m_databaseManager.upsertServingSize(serving, nullptr, errorMessage)) {
                return false;
            }
        }
        return true;
    }, errorMessage);
}

bool USDAImporter::importFoodNutrients(const QString &filePath, QString *errorMessage)
{
    return forEachCsvRow(filePath, [&](const CsvRow &row) {
        const int foodId = foodIdFromFdcId(toInt(value(row, "fdc_id")));
        const int nutrientId = nutrientIdFromUsdaId(toInt(value(row, "nutrient_id")));
        if (foodId <= 0 || nutrientId <= 0) {
            return true;
        }

        USDAFoodNutrient foodNutrient;
        foodNutrient.foodId = foodId;
        foodNutrient.nutrientId = nutrientId;
        foodNutrient.amount = toDouble(value(row, "amount"));

        const int derivationId = toInt(value(row, "derivation_id"));
        const auto derivation = m_derivationsById.value(derivationId);
        foodNutrient.derivationCode = derivation.first;
        foodNutrient.derivationDescription = derivation.second;

        if (!m_databaseManager.upsertFoodNutrient(foodNutrient, errorMessage)) {
            return false;
        }
        return true;
    }, errorMessage);
}

bool USDAImporter::importServingSizes(const QString &filePath, QString *errorMessage)
{
    return forEachCsvRow(filePath, [&](const CsvRow &row) {
        const int foodId = foodIdFromFdcId(toInt(value(row, "fdc_id")));
        if (foodId <= 0) {
            return true;
        }

        const int measureUnitId = toInt(value(row, "measure_unit_id"));
        const QString portionDescription = value(row, "portion_description");
        const QString modifier = value(row, "modifier");

        USDAServingSize serving;
        serving.foodId = foodId;
        serving.amount = toDouble(value(row, "amount"), 1.0);
        serving.unit = m_measureUnitsById.value(measureUnitId);
        serving.gramWeight = toDouble(value(row, "gram_weight"));
        serving.description = portionDescription.isEmpty() ? modifier : portionDescription;

        if (!m_databaseManager.upsertServingSize(serving, nullptr, errorMessage)) {
            return false;
        }
        return true;
    }, errorMessage);
}

bool USDAImporter::readCsvRows(const QString &filePath, QVector<CsvRow> *rows, QString *errorMessage) const
{
    rows->clear();
    return forEachCsvRow(filePath, [&](const CsvRow &row) {
        rows->append(row);
        return true;
    }, errorMessage);
}

bool USDAImporter::forEachCsvRow(
    const QString &filePath,
    const std::function<bool(const CsvRow &)> &handler,
    QString *errorMessage) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(errorMessage, "Could not open CSV file: " + filePath);
        return false;
    }

    QTextStream stream(&file);
    if (stream.atEnd()) {
        return true;
    }

    QString headerRecord;
    while (!stream.atEnd()) {
        if (!headerRecord.isEmpty()) {
            headerRecord.append('\n');
        }
        headerRecord.append(stream.readLine());
        if (isCompleteCsvRecord(headerRecord)) {
            break;
        }
    }

    QStringList headers;
    const QStringList rawHeaders = parseCsvLine(headerRecord);
    for (const QString &header : rawHeaders) {
        headers.append(normalizedColumnName(header));
    }

    if (headers.isEmpty()) {
        return true;
    }

    QString record;
    while (!stream.atEnd()) {
        if (!record.isEmpty()) {
            record.append('\n');
        }
        record.append(stream.readLine());

        if (!isCompleteCsvRecord(record)) {
            continue;
        }

        if (record.trimmed().isEmpty()) {
            record.clear();
            continue;
        }

        const QStringList columns = parseCsvLine(record);
        CsvRow row;
        for (int index = 0; index < headers.size(); ++index) {
            row.insert(headers.at(index), index < columns.size() ? columns.at(index) : QString());
        }

        if (!handler(row)) {
            return false;
        }

        record.clear();
    }

    if (!record.trimmed().isEmpty()) {
        setError(errorMessage, "CSV file ended inside a quoted record: " + filePath);
        return false;
    }

    return true;
}

QStringList USDAImporter::parseCsvLine(const QString &line) const
{
    QStringList values;
    QString current;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line.at(i + 1) == '"') {
                current.append('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            values.append(current);
            current.clear();
        } else {
            current.append(ch);
        }
    }

    values.append(current);
    return values;
}

QString USDAImporter::normalizedColumnName(const QString &column) const
{
    QString normalized = column;
    normalized.remove(QChar(0xFEFF));
    return normalized.trimmed().toLower();
}

bool USDAImporter::isCompleteCsvRecord(const QString &record) const
{
    bool inQuotes = false;

    for (int i = 0; i < record.size(); ++i) {
        if (record.at(i) != '"') {
            continue;
        }

        if (inQuotes && i + 1 < record.size() && record.at(i + 1) == '"') {
            ++i;
            continue;
        }

        inQuotes = !inQuotes;
    }

    return !inQuotes;
}

QString USDAImporter::value(const CsvRow &row, const QString &column) const
{
    return row.value(normalizedColumnName(column)).trimmed();
}

int USDAImporter::toInt(const QString &text, int defaultValue) const
{
    bool ok = false;
    const int value = text.toInt(&ok);
    return ok ? value : defaultValue;
}

double USDAImporter::toDouble(const QString &text, double defaultValue) const
{
    bool ok = false;
    const double value = text.toDouble(&ok);
    return ok ? value : defaultValue;
}

QString USDAImporter::firstExistingFile(const QString &directoryPath, const QStringList &fileNames) const
{
    QDir directory(directoryPath);
    for (const QString &fileName : fileNames) {
        const QString path = directory.filePath(fileName);
        if (QFile::exists(path)) {
            return path;
        }
    }
    return {};
}

int USDAImporter::foodIdFromFdcId(int fdcId) const
{
    if (fdcId <= 0) {
        return -1;
    }

    auto existing = m_foodIdsByFdcId.constFind(fdcId);
    if (existing != m_foodIdsByFdcId.constEnd()) {
        return existing.value();
    }

    const int foodId = m_databaseManager.findFoodIdByFdcId(fdcId);
    if (foodId > 0) {
        m_foodIdsByFdcId.insert(fdcId, foodId);
    }
    return foodId;
}

int USDAImporter::nutrientIdFromUsdaId(int usdaNutrientId) const
{
    if (usdaNutrientId <= 0) {
        return -1;
    }

    auto existing = m_nutrientIdsByUsdaId.constFind(usdaNutrientId);
    if (existing != m_nutrientIdsByUsdaId.constEnd()) {
        return existing.value();
    }

    const int nutrientId = m_databaseManager.findNutrientIdByUsdaNutrientId(usdaNutrientId);
    if (nutrientId > 0) {
        m_nutrientIdsByUsdaId.insert(usdaNutrientId, nutrientId);
    }
    return nutrientId;
}
