#pragma once

#include <QHash>
#include <QPair>
#include <QString>
#include <QStringList>
#include <functional>

#include "DatabaseManager.h"

// Imports USDA FoodData Central CSV export folders.
// Expected files include food.csv, nutrient.csv, food_nutrient.csv,
// branded_food.csv, food_portion.csv, measure_unit.csv, food_category.csv,
// and food_nutrient_derivation.csv. Missing optional files are skipped safely.
class USDAImporter
{
public:
    explicit USDAImporter(DatabaseManager &databaseManager);

    bool importCsvDirectory(const QString &directoryPath, QString *errorMessage = nullptr);

private:
    using CsvRow = QHash<QString, QString>;

    bool importFoodCategories(const QString &filePath, QString *errorMessage);
    bool importMeasureUnits(const QString &filePath, QString *errorMessage);
    bool importDerivations(const QString &filePath, QString *errorMessage);
    bool importFoods(const QString &filePath, QString *errorMessage);
    bool importNutrients(const QString &filePath, QString *errorMessage);
    bool importBrandedFoods(const QString &filePath, QString *errorMessage);
    bool importFoodNutrients(const QString &filePath, QString *errorMessage);
    bool importServingSizes(const QString &filePath, QString *errorMessage);

    bool readCsvRows(const QString &filePath, QVector<CsvRow> *rows, QString *errorMessage) const;
    bool forEachCsvRow(
        const QString &filePath,
        const std::function<bool(const CsvRow &)> &handler,
        QString *errorMessage) const;
    QStringList parseCsvLine(const QString &line) const;
    QString normalizedColumnName(const QString &column) const;
    bool isCompleteCsvRecord(const QString &record) const;
    QString value(const CsvRow &row, const QString &column) const;
    int toInt(const QString &text, int defaultValue = 0) const;
    double toDouble(const QString &text, double defaultValue = 0.0) const;
    QString firstExistingFile(const QString &directoryPath, const QStringList &fileNames) const;
    int foodIdFromFdcId(int fdcId) const;
    int nutrientIdFromUsdaId(int usdaNutrientId) const;

    DatabaseManager &m_databaseManager;
    QHash<int, QString> m_foodCategoriesById;
    QHash<int, QString> m_measureUnitsById;
    QHash<int, QPair<QString, QString>> m_derivationsById;
    mutable QHash<int, int> m_foodIdsByFdcId;
    mutable QHash<int, int> m_nutrientIdsByUsdaId;
};
