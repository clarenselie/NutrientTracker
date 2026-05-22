#include "UnitConverter.h"

#include <stdexcept>

QString UnitConverter::componentName() const
{
    return "Unit Converter";
}

double UnitConverter::toGrams(const ParsedFoodItem &item, const FoodRecord &food) const
{
    if (item.quantity <= 0.0) {
        throw std::invalid_argument("food quantity must be greater than zero");
    }

    const QString unit = item.unit.toLower();

    if (unit == "g") {
        return item.quantity;
    }
    if (unit == "kg") {
        return item.quantity * 1000.0;
    }
    if (unit == "oz") {
        return item.quantity * 28.3495;
    }
    if (unit == "lb") {
        return item.quantity * 453.592;
    }
    if (unit == "cup") {
        return item.quantity * food.gramsPerCup;
    }
    if (unit == "serving" || unit == "unit") {
        return item.quantity * food.gramsPerUnit;
    }

    return item.quantity * food.gramsPerUnit;
}
