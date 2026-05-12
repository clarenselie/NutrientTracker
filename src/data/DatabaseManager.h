#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <optional>

#include "NutrientTypes.h"

class DatabaseManager
{
public:
    explicit DatabaseManager(const QString &databasePath = QString());
    ~DatabaseManager();

    // Opens the SQLite connection and creates/migrates the USDA schema.
    bool initDatabase(QString *errorMessage = nullptr);
    bool initialize(QString *errorMessage = nullptr);
    QString databaseFilePath() const;
    bool installDatabaseFromFile(const QString &sourcePath, QString *errorMessage = nullptr);

    // USDA upsert methods preserve stable internal IDs while keeping FDC IDs.
    bool upsertFood(const USDAFood &food, int *foodId = nullptr, QString *errorMessage = nullptr);
    bool upsertBrandedFood(const USDABrandedFood &brandedFood, QString *errorMessage = nullptr);
    bool upsertNutrient(const USDANutrient &nutrient, int *nutrientId = nullptr, QString *errorMessage = nullptr);
    bool upsertFoodNutrient(const USDAFoodNutrient &foodNutrient, QString *errorMessage = nullptr);
    bool upsertServingSize(const USDAServingSize &servingSize, int *servingId = nullptr, QString *errorMessage = nullptr);

    // App-facing retrieval methods.
    QVector<FoodRecord> searchFoods(const QString &term, const QString &brand = QString()) const;
    std::optional<FoodRecord> searchBrandedFoodByBarcode(const QString &barcode) const;
    QVector<NutrientValue> getFoodNutrients(int foodId) const;
    QVector<NutrientValue> getFoodNutrientsForGrams(int foodId, double grams) const;
    QVector<NutrientValue> getFoodNutrientsForServing(int servingId, double servings = 1.0) const;
    QVector<USDAServingSize> getServingSizes(int foodId) const;

    bool hasImportedUSDAData() const;
    int findFoodIdByFdcId(int fdcId) const;
    int findNutrientIdByUsdaNutrientId(int usdaNutrientId) const;

    bool beginTransaction(QString *errorMessage = nullptr);
    bool commitTransaction(QString *errorMessage = nullptr);
    void rollbackTransaction();

private:
    QString databasePath() const;
    bool createTables(QString *errorMessage) const;
    bool createIndexes(QString *errorMessage) const;
    bool ensureSchema(QString *errorMessage);
    bool dropKnownTables(QString *errorMessage) const;
    bool tableExists(const QString &tableName) const;
    bool tableHasColumn(const QString &tableName, const QString &columnName) const;

    FoodRecord recordFromQuery(const QSqlQuery &query) const;
    QVector<NutrientValue> scaleNutrients(const QVector<NutrientValue> &nutrients, double grams) const;

    mutable QSqlDatabase m_db;
    QString m_connectionName;
    QString m_databasePathOverride;
};
