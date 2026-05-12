#pragma once

#include <QVector>

#include "DomainTypes.h"

class NutrientEngine
{
public:
    QVector<NutrientValue> scaleToGrams(const QVector<NutrientValue> &per100g, double grams) const;
    QVector<NutrientValue> calculateTotals(const QVector<AnalyzedFoodItem> &foods) const;
};

