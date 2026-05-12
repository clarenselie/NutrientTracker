#pragma once

#include "DatabaseManager.h"
#include "FoodMatcher.h"
#include "InputParser.h"
#include "NutrientEngine.h"
#include "UnitConverter.h"

class MealAnalyzer
{
public:
    explicit MealAnalyzer(DatabaseManager &databaseManager);

    AnalysisResult analyze(const QString &input) const;

private:
    DatabaseManager &m_databaseManager;
    InputParser m_parser;
    FoodMatcher m_matcher;
    UnitConverter m_converter;
    NutrientEngine m_engine;
};

