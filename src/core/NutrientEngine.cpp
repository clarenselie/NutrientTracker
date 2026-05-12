#include "NutrientEngine.h"

#include <QHash>
#include <algorithm>

QVector<NutrientValue> NutrientEngine::scaleToGrams(const QVector<NutrientValue> &per100g, double grams) const
{
    QVector<NutrientValue> scaled;
    const double factor = grams / 100.0;

    for (const NutrientValue &nutrient : per100g) {
        NutrientValue value = nutrient;
        value.amount = nutrient.amount * factor;
        scaled.append(value);
    }

    return scaled;
}

QVector<NutrientValue> NutrientEngine::calculateTotals(const QVector<AnalyzedFoodItem> &foods) const
{
    QHash<QString, NutrientValue> totalsByKey;

    for (const AnalyzedFoodItem &food : foods) {
        for (const NutrientValue &nutrient : food.scaledNutrients) {
            if (!totalsByKey.contains(nutrient.key)) {
                totalsByKey.insert(nutrient.key, nutrient);
            } else {
                totalsByKey[nutrient.key].amount += nutrient.amount;
            }
        }
    }

    QVector<NutrientValue> totals = totalsByKey.values().toVector();
    std::sort(totals.begin(), totals.end(), [](const NutrientValue &left, const NutrientValue &right) {
        if (left.displayOrder != right.displayOrder) {
            return left.displayOrder < right.displayOrder;
        }
        return left.label.toLower() < right.label.toLower();
    });

    return totals;
}
