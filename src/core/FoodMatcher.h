#pragma once

#include <optional>

#include "DatabaseManager.h"
#include "DomainTypes.h"

class FoodMatcher
{
public:
    explicit FoodMatcher(DatabaseManager &databaseManager);

    std::optional<FoodRecord> match(const ParsedFoodItem &item) const;

private:
    QString normalize(const QString &text) const;
    int scoreCandidate(const ParsedFoodItem &item, const FoodRecord &candidate) const;

    DatabaseManager &m_databaseManager;
};