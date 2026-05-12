#pragma once

#include <QString>
#include <QVector>

#include "NutrientTypes.h"

struct ParsedFoodItem {
    QString rawText;
    QString foodName;
    QString brand;
    double quantity = 1.0;
    QString unit;
};

struct AnalyzedFoodItem {
    ParsedFoodItem parsed;
    FoodRecord food;
    double grams = 0.0;
    QVector<NutrientValue> scaledNutrients;
};

struct AnalysisResult {
    QVector<AnalyzedFoodItem> matchedFoods;
    QVector<QString> unmatchedFoods;
    QVector<QString> warnings;
    QVector<NutrientValue> totals;
};
