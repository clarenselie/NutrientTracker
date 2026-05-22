#include "FoodMatcher.h"

#include <QRegularExpression>
#include <QSet>
#include <limits>

FoodMatcher::FoodMatcher(DatabaseManager &databaseManager)
    : m_databaseManager(databaseManager)
{
}

QString FoodMatcher::componentName() const
{
    return "Food Matcher";
}

std::optional<FoodRecord> FoodMatcher::match(const ParsedFoodItem &item) const
{
    const QString requested = normalize(item.foodName);
    QSet<QString> searchTerms = {requested};
    if (requested.endsWith("ies") && requested.size() > 3) {
        searchTerms.insert(requested.left(requested.size() - 3) + "y");
    }
    if (requested.endsWith("es") && requested.size() > 2) {
        searchTerms.insert(requested.left(requested.size() - 2));
    }
    if (requested.endsWith('s') && requested.size() > 1) {
        searchTerms.insert(requested.left(requested.size() - 1));
    }

    QVector<FoodRecord> candidates;
    QSet<int> seenIds;
    for (const QString &term : searchTerms) {
        const QVector<FoodRecord> matches = m_databaseManager.searchFoods(term, item.brand);
        for (const FoodRecord &match : matches) {
            if (!seenIds.contains(match.id)) {
                seenIds.insert(match.id);
                candidates.append(match);
            }
        }
    }

    if (candidates.isEmpty()) {
        return std::nullopt;
    }

    int bestScore = std::numeric_limits<int>::min();
    std::optional<FoodRecord> bestMatch;

    for (const FoodRecord &candidate : candidates) {
        const int candidateScore = scoreCandidate(item, candidate);
        if (!bestMatch.has_value() || candidateScore > bestScore) {
            bestScore = candidateScore;
            bestMatch = candidate;
        }
    }

    return bestMatch;
}

QString FoodMatcher::normalize(const QString &text) const
{
    QString value = text.toLower().trimmed();
    value.replace("-", " ");
    value.replace(QRegularExpression("\\s+"), " ");
    return value;
}

int FoodMatcher::scoreCandidate(const ParsedFoodItem &item, const FoodRecord &candidate) const
{
    const QString requested = normalize(item.foodName);
    const QString candidateName = normalize(candidate.name);
    const QString candidateBrand = normalize(candidate.brand);
    const QString candidateFull = normalize(candidate.name + " " + candidate.brand);

    int score = 0;

    if (candidateName == requested || candidateFull == requested) {
        score += 100;
    }
    if (candidateName.contains(requested) || candidateFull.contains(requested)) {
        score += 40;
    }

    const QStringList tokens = requested.split(' ', Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        if (candidateName.contains(token) || candidateBrand.contains(token)) {
            score += 12;
        }
    }

    if (!item.brand.isEmpty() && candidateBrand.contains(normalize(item.brand))) {
        score += 25;
    }

    return score;
}
