#include "MealAnalyzer.h"

MealAnalyzer::MealAnalyzer(DatabaseManager &databaseManager)
    : m_databaseManager(databaseManager)
    , m_matcher(databaseManager)
{
}

AnalysisResult MealAnalyzer::analyze(const QString &input) const
{
    AnalysisResult result;
    const QVector<ParsedFoodItem> parsedItems = m_parser.parse(input);

    if (parsedItems.isEmpty()) {
        result.warnings.append("Enter at least one food to analyze.");
        return result;
    }

    for (const ParsedFoodItem &parsed : parsedItems) {
        const std::optional<FoodRecord> matchedFood = m_matcher.match(parsed);
        if (!matchedFood.has_value()) {
            result.unmatchedFoods.append(parsed.foodName);
            continue;
        }

        const double grams = m_converter.toGrams(parsed, matchedFood.value());
        if (grams <= 0.0) {
            result.warnings.append("Skipped " + parsed.foodName + " because its quantity was invalid.");
            continue;
        }

        AnalyzedFoodItem item;
        item.parsed = parsed;
        item.food = matchedFood.value();
        item.grams = grams;
        item.scaledNutrients = m_engine.scaleToGrams(
            m_databaseManager.getFoodNutrients(matchedFood->id), grams);

        result.matchedFoods.append(item);
    }

    result.totals = m_engine.calculateTotals(result.matchedFoods);

    if (result.matchedFoods.isEmpty() && result.unmatchedFoods.isEmpty()) {
        result.warnings.append("No foods could be analyzed.");
    }

    return result;
}

