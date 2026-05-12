#pragma once

#include "DomainTypes.h"

class UnitConverter
{
public:
    double toGrams(const ParsedFoodItem &item, const FoodRecord &food) const;
};

