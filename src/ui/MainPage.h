#pragma once

#include <QDate>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

#include "MealAnalyzer.h"

class QGraphicsOpacityEffect;
class QTimer;

class CalorieRing : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int progress READ progress WRITE setProgress)

public:
    explicit CalorieRing(QWidget *parent = nullptr);

    int progress() const;
    void setProgress(int progress);
    void setCalories(int remaining, int target);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_progress = 0;
    int m_remaining = 0;
    int m_target = 2200;
};

class MainPage : public QWidget
{
public:
    explicit MainPage(QWidget *parent = nullptr);

private:
    enum class FoodInputAction {
        None,
        AddFood,
        TryFood
    };

    struct EndOfDayGoalReviewMessage {
        QString text;
        bool positive = false;
    };

    QWidget *createCard(QWidget *content);
    QWidget *createMacroRow(const QString &label, const QString &unit, QProgressBar **bar, QLabel **valueLabel);
    void setupFoodDatabase();
    QString findBundledDatabaseFile() const;
    void addFoodInput();
    void previewFoodInput();
    void handleFoodInput(bool shouldLog);
    QString normalizeFoodInput(const QString &rawInput) const;
    void showInputConfirmation(const QString &normalizedInput, FoodInputAction action);
    void confirmFoodInput();
    void cancelFoodInputConfirmation();
    void clearFoodInputConfirmation();
    void setupHealthQuotes();
    void rotateHealthQuote();
    void fadeToNextQuote();
    void setupEndOfDayGoalReview();
    void checkForEndOfDayGoalReview();
    QVector<EndOfDayGoalReviewMessage> generateEndOfDayGoalReview(
        const QVector<NutrientValue> &dailyTotals) const;
    void showEndOfDayGoalReview(
        const QDate &completedDate,
        const QVector<EndOfDayGoalReviewMessage> &messages);
    void acceptFoodSuggestion();
    void clearFoodSuggestion();
    void showFoodSuggestion(const QString &input, const QVector<QString> &unmatchedFoods);
    void showFoodUnavailableMessage();
    QString findClosestFoodMatch(const QString &foodName) const;
    int getFoodMatchConfidence(const QString &input, const QString &candidate) const;
    bool isReliableFoodSuggestion(int score) const;
    QString buildSuggestedInput(const QString &input, const QString &misspelledFood, const QString &suggestedFood) const;
    void deleteSelectedFood();
    void addLoggedFood(const AnalyzedFoodItem &food);
    void refreshDashboard();
    void refreshDailyLog();
    void refreshNutrientBreakdown();
    void refreshImportState();
    QVector<NutrientValue> dailyTotalNutrients() const;
    QVector<NutrientValue> currentBreakdownNutrients() const;
    double nutrientAmount(const QVector<NutrientValue> &nutrients, const QString &key) const;
    double displayAmountForKey(const QVector<NutrientValue> &nutrients, const QString &key) const;
    QString displayUnitForKey(const QVector<NutrientValue> &nutrients, const TrackedNutrientDefinition &definition) const;
    double totalFor(const QString &key) const;
    QString formatAmount(double amount) const;
    int percentOf(double value, double target) const;
    QString getCalorieProgressMessage(double totalCalories, double calorieGoal) const;
    bool isOverCalorieGoal(double totalCalories, double calorieGoal) const;
    void animateProgress(QProgressBar *bar, int value);
    void applyShadow(QWidget *widget);

    DatabaseManager m_databaseManager;
    MealAnalyzer m_mealAnalyzer;
    QVector<AnalyzedFoodItem> m_loggedFoods;

    QLineEdit *m_foodSearchEdit = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_previewButton = nullptr;
    QLabel *m_inputConfirmationLabel = nullptr;
    QPushButton *m_confirmInputYesButton = nullptr;
    QPushButton *m_confirmInputNoButton = nullptr;
    QLabel *m_suggestionLabel = nullptr;
    QPushButton *m_useSuggestionButton = nullptr;
    CalorieRing *m_calorieRing = nullptr;
    QLabel *m_dateLabel = nullptr;
    QLabel *m_caloriesRemainingLabel = nullptr;
    QLabel *m_calorieProgressMessageLabel = nullptr;
    QLabel *m_calorieWarningLabel = nullptr;
    QLabel *m_healthQuoteLabel = nullptr;
    QGraphicsOpacityEffect *m_healthQuoteOpacityEffect = nullptr;
    QLabel *m_proteinValueLabel = nullptr;
    QLabel *m_carbsValueLabel = nullptr;
    QLabel *m_fatValueLabel = nullptr;
    QLabel *m_fiberValueLabel = nullptr;
    QProgressBar *m_proteinBar = nullptr;
    QProgressBar *m_carbsBar = nullptr;
    QProgressBar *m_fatBar = nullptr;
    QProgressBar *m_fiberBar = nullptr;
    QListWidget *m_recentFoodsList = nullptr;
    QPushButton *m_deleteSelectedButton = nullptr;
    QTableWidget *m_dailyLogTable = nullptr;
    QLabel *m_nutrientBreakdownTitle = nullptr;
    QTableWidget *m_nutrientBreakdownTable = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTimer *m_endOfDayReviewTimer = nullptr;
    QVector<NutrientValue> m_previewNutrients;
    QVector<QString> m_healthQuotes;
    QString m_previewTitle;
    QString m_pendingConfirmedInput;
    QString m_suggestedFoodName;
    QString m_suggestedInput;
    QDate m_currentReviewDate;
    int m_currentHealthQuoteIndex = 0;
    FoodInputAction m_pendingFoodInputAction = FoodInputAction::None;
    bool m_hasPreview = false;
    bool m_hasUSDAData = false;
    bool m_usdaImportInProgress = false;
    QString m_databaseSetupError;

    const double m_calorieTarget = 2200.0;
    const double m_proteinTarget = 150.0;
    const double m_carbsTarget = 250.0;
    const double m_fatTarget = 70.0;
    const double m_fiberTarget = 30.0;
};
