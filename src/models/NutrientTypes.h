#pragma once

#include <QString>
#include <QVector>

// Shared nutrition/domain structs live here so headers do not redefine
// the same food or nutrient shapes in multiple modules.

struct NutrientValue {
    QString key;
    QString label;
    QString category;
    QString subcategory;
    double amount = 0.0;
    QString unit;
    int displayOrder = 0;
    int usdaNutrientId = 0;
};

struct TrackedNutrientDefinition {
    QString key;
    QString label;
    QString category;
    QString subcategory;
    QString defaultUnit;
    int displayOrder = 0;
};

inline QVector<TrackedNutrientDefinition> trackedNutrientDefinitions()
{
    return {
        {"calories", "Calories", "Energy", "", "kcal", 100},

        {"protein", "Protein", "Macronutrients", "", "g", 200},
        {"carbs", "Carbohydrates", "Macronutrients", "", "g", 210},
        {"fat", "Fat", "Macronutrients", "", "g", 220},
        {"fiber", "Fiber", "Macronutrients", "", "g", 230},
        {"sugar", "Sugar", "Macronutrients", "", "g", 240},
        {"water", "Water", "Macronutrients", "", "g", 250},

        {"calcium", "Calcium", "Minerals", "Major minerals", "mg", 300},
        {"iron", "Iron", "Minerals", "Major minerals", "mg", 310},
        {"magnesium", "Magnesium", "Minerals", "Major minerals", "mg", 320},
        {"phosphorus", "Phosphorus", "Minerals", "Major minerals", "mg", 330},
        {"potassium", "Potassium", "Minerals", "Major minerals", "mg", 340},
        {"sodium", "Sodium", "Minerals", "Major minerals", "mg", 350},
        {"chloride", "Chloride", "Minerals", "Major minerals", "mg", 360},
        {"zinc", "Zinc", "Minerals", "Trace minerals", "mg", 370},
        {"copper", "Copper", "Minerals", "Trace minerals", "mg", 380},
        {"manganese", "Manganese", "Minerals", "Trace minerals", "mg", 390},
        {"selenium", "Selenium", "Minerals", "Trace minerals", "ug", 400},
        {"iodine", "Iodine", "Minerals", "Trace minerals", "ug", 410},
        {"chromium", "Chromium", "Minerals", "Trace minerals", "ug", 420},
        {"molybdenum", "Molybdenum", "Minerals", "Trace minerals", "ug", 430},

        {"vitamin_a", "Vitamin A", "Vitamins", "Fat-soluble vitamins", "ug", 500},
        {"vitamin_d", "Vitamin D", "Vitamins", "Fat-soluble vitamins", "ug", 510},
        {"vitamin_e", "Vitamin E", "Vitamins", "Fat-soluble vitamins", "mg", 520},
        {"vitamin_k", "Vitamin K", "Vitamins", "Fat-soluble vitamins", "ug", 530},
        {"vitamin_c", "Vitamin C", "Vitamins", "Water-soluble vitamins", "mg", 540},
        {"b1", "Vitamin B1 (Thiamine)", "Vitamins", "Water-soluble vitamins", "mg", 550},
        {"b2", "Vitamin B2 (Riboflavin)", "Vitamins", "Water-soluble vitamins", "mg", 560},
        {"b3", "Vitamin B3 (Niacin)", "Vitamins", "Water-soluble vitamins", "mg", 570},
        {"b5", "Vitamin B5 (Pantothenic acid)", "Vitamins", "Water-soluble vitamins", "mg", 580},
        {"b6", "Vitamin B6", "Vitamins", "Water-soluble vitamins", "mg", 590},
        {"b7", "Vitamin B7 (Biotin)", "Vitamins", "Water-soluble vitamins", "ug", 600},
        {"b9", "Vitamin B9 (Folate)", "Vitamins", "Water-soluble vitamins", "ug", 610},
        {"b12", "Vitamin B12", "Vitamins", "Water-soluble vitamins", "ug", 620},

        {"omega3", "Omega-3 fatty acids", "Advanced nutrients", "", "g", 700},
        {"ala", "ALA", "Advanced nutrients", "Omega-3 fatty acids", "g", 710},
        {"epa", "EPA", "Advanced nutrients", "Omega-3 fatty acids", "g", 720},
        {"dha", "DHA", "Advanced nutrients", "Omega-3 fatty acids", "g", 730},
        {"omega6", "Omega-6 fatty acids", "Advanced nutrients", "", "g", 740},
        {"saturated_fat", "Saturated fat", "Advanced nutrients", "", "g", 750},
        {"monounsaturated_fat", "Monounsaturated fat", "Advanced nutrients", "", "g", 760},
        {"polyunsaturated_fat", "Polyunsaturated fat", "Advanced nutrients", "", "g", 770},
        {"cholesterol", "Cholesterol", "Advanced nutrients", "", "mg", 780},
        {"caffeine", "Caffeine", "Advanced nutrients", "", "mg", 790}
    };
}

inline TrackedNutrientDefinition trackedNutrientDefinitionForKey(const QString &key)
{
    for (const TrackedNutrientDefinition &definition : trackedNutrientDefinitions()) {
        if (definition.key == key) {
            return definition;
        }
    }
    return {};
}

struct FoodRecord {
    int id = -1;
    int fdcId = 0;
    QString name;
    QString brand;
    QString searchName;
    QString dataType;
    QString foodCategory;
    QString publicationDate;
    QString source;
    double gramsPerUnit = 100.0;
    double gramsPerCup = 240.0;
};

struct USDAFood {
    int id = -1;
    int fdcId = 0;
    QString description;
    QString dataType;
    QString foodCategory;
    QString publicationDate;
    QString source = "USDA FoodData Central";
};

struct USDABrandedFood {
    int foodId = -1;
    QString brandOwner;
    QString brandName;
    QString gtinUpc;
    QString ingredients;
    double servingSize = 0.0;
    QString servingSizeUnit;
    QString householdServingFullText;
};

struct USDANutrient {
    int id = -1;
    int usdaNutrientId = 0;
    QString name;
    QString unitName;
    QString nutrientNumber;
    int rank = 0;
};

struct USDAFoodNutrient {
    int foodId = -1;
    int nutrientId = -1;
    double amount = 0.0;
    QString derivationCode;
    QString derivationDescription;
};

struct USDAServingSize {
    int id = -1;
    int foodId = -1;
    double amount = 0.0;
    QString unit;
    double gramWeight = 0.0;
    QString description;
};
