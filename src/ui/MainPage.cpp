#include "MainPage.h"

#include <QCoreApplication>
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHeaderView>
#include <QHash>
#include <QHBoxLayout>
#include <QPainter>
#include <QPen>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QSplitter>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

namespace {
struct NutrientRange {
    QString key;
    double minimum = 0.0;
    double target = 0.0;
    double maximum = 0.0;
    bool hasMinimum = false;
    bool hasTarget = false;
    bool hasMaximum = false;
    bool scaleWithCalories = false;
};

struct NutrientStatus {
    QString label = "Unknown";
    QColor color = QColor("#9CA3AF");
};

QString normalizeFoodText(const QString &text)
{
    QString normalized = text.toLower().trimmed();
    normalized.replace(QRegularExpression("[^a-z0-9\\s]"), " ");
    normalized.replace(QRegularExpression("\\s+"), " ");
    return normalized.trimmed();
}

QString collapseRepeatedLetters(const QString &text)
{
    const QString normalized = normalizeFoodText(text);
    QString collapsed;
    QChar previous;

    for (const QChar character : normalized) {
        if (character != previous || character.isSpace()) {
            collapsed.append(character);
        }
        previous = character;
    }

    return collapsed;
}

QString foodUnavailableMessage()
{
    return "We can't provide information about this food. It may not be in the database, "
           "or there may be a typo. Please try typing it again.";
}

bool isSingleAdjacentTransposition(const QString &left, const QString &right)
{
    const QString a = normalizeFoodText(left);
    const QString b = normalizeFoodText(right);
    if (a.size() != b.size() || a.size() < 2) {
        return false;
    }

    int firstDifference = -1;
    int secondDifference = -1;
    for (int i = 0; i < a.size(); ++i) {
        if (a.at(i) == b.at(i)) {
            continue;
        }
        if (firstDifference == -1) {
            firstDifference = i;
        } else if (secondDifference == -1) {
            secondDifference = i;
        } else {
            return false;
        }
    }

    return firstDifference >= 0
        && secondDifference == firstDifference + 1
        && a.at(firstDifference) == b.at(secondDifference)
        && a.at(secondDifference) == b.at(firstDifference);
}

int levenshteinDistance(const QString &left, const QString &right)
{
    const QString a = normalizeFoodText(left);
    const QString b = normalizeFoodText(right);

    QVector<int> previous(b.size() + 1);
    QVector<int> current(b.size() + 1);
    for (int j = 0; j <= b.size(); ++j) {
        previous[j] = j;
    }

    for (int i = 1; i <= a.size(); ++i) {
        current[0] = i;
        for (int j = 1; j <= b.size(); ++j) {
            const int substitutionCost = a.at(i - 1) == b.at(j - 1) ? 0 : 1;
            current[j] = std::min({
                previous[j] + 1,
                current[j - 1] + 1,
                previous[j - 1] + substitutionCost
            });
        }
        previous = current;
    }

    return previous[b.size()];
}

QVector<NutrientRange> nutrientRanges()
{
    return {
        {"calories", 1800.0, 2000.0, 2200.0, true, true, true, true},
        {"protein", 0.0, 50.0, 0.0, false, true, false, true},
        {"carbs", 0.0, 275.0, 0.0, false, true, false, true},
        {"fat", 44.0, 67.0, 78.0, true, true, true, true},
        {"fiber", 0.0, 28.0, 0.0, false, true, false, true},
        {"sugar", 0.0, 0.0, 50.0, false, false, true, true},
        {"water", 0.0, 3700.0, 0.0, false, true, false, false},

        {"calcium", 1000.0, 1000.0, 2500.0, true, true, true, false},
        {"iron", 8.0, 18.0, 45.0, true, true, true, false},
        {"magnesium", 0.0, 420.0, 0.0, false, true, false, false},
        {"phosphorus", 700.0, 700.0, 4000.0, true, true, true, false},
        {"potassium", 0.0, 3400.0, 0.0, false, true, false, false},
        {"sodium", 500.0, 1500.0, 2300.0, true, true, true, false},
        {"chloride", 0.0, 2300.0, 3600.0, false, true, true, false},
        {"zinc", 0.0, 11.0, 40.0, false, true, true, false},
        {"copper", 0.0, 0.9, 10.0, false, true, true, false},
        {"manganese", 0.0, 2.3, 11.0, false, true, true, false},
        {"selenium", 0.0, 55.0, 400.0, false, true, true, false},
        {"iodine", 0.0, 150.0, 1100.0, false, true, true, false},
        {"chromium", 0.0, 35.0, 0.0, false, true, false, false},
        {"molybdenum", 0.0, 45.0, 2000.0, false, true, true, false},

        {"vitamin_a", 0.0, 900.0, 3000.0, false, true, true, false},
        {"vitamin_d", 0.0, 20.0, 100.0, false, true, true, false},
        {"vitamin_e", 0.0, 15.0, 1000.0, false, true, true, false},
        {"vitamin_k", 0.0, 120.0, 0.0, false, true, false, false},
        {"vitamin_c", 0.0, 90.0, 2000.0, false, true, true, false},
        {"b1", 0.0, 1.2, 0.0, false, true, false, false},
        {"b2", 0.0, 1.3, 0.0, false, true, false, false},
        {"b3", 0.0, 16.0, 35.0, false, true, true, false},
        {"b5", 0.0, 5.0, 0.0, false, true, false, false},
        {"b6", 0.0, 1.7, 100.0, false, true, true, false},
        {"b7", 0.0, 30.0, 0.0, false, true, false, false},
        {"b9", 0.0, 400.0, 1000.0, false, true, true, false},
        {"b12", 0.0, 2.4, 0.0, false, true, false, false},

        {"omega3", 0.0, 1.6, 0.0, false, true, false, false},
        {"ala", 0.0, 1.6, 0.0, false, true, false, false},
        {"epa", 0.0, 0.25, 0.0, false, true, false, false},
        {"dha", 0.0, 0.25, 0.0, false, true, false, false},
        {"omega6", 0.0, 17.0, 0.0, false, true, false, false},
        {"saturated_fat", 0.0, 0.0, 20.0, false, false, true, true},
        {"cholesterol", 0.0, 0.0, 300.0, false, false, true, false},
        {"caffeine", 0.0, 0.0, 400.0, false, false, true, false}
    };
}

NutrientStatus getNutrientStatus(const QString &nutrientKey, double currentAmount, double calorieGoal)
{
    for (const NutrientRange &range : nutrientRanges()) {
        if (range.key != nutrientKey) {
            continue;
        }

        const double scale = range.scaleWithCalories ? calorieGoal / 2000.0 : 1.0;
        const double minimum = range.minimum * scale;
        const double target = range.target * scale;
        const double maximum = range.maximum * scale;

        if (range.hasMinimum && currentAmount < minimum) {
            return {"Low", QColor("#EF4444")};
        }
        if (!range.hasMinimum && range.hasTarget && !range.hasMaximum && currentAmount < target) {
            return {"Low", QColor("#EF4444")};
        }
        if (range.hasMaximum && currentAmount > maximum) {
            return {"High", QColor("#EF4444")};
        }

        return {"Healthy", QColor("#3CB371")};
    }

    return {};
}
}

CalorieRing::CalorieRing(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(210, 210);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

int CalorieRing::progress() const
{
    return m_progress;
}

void CalorieRing::setProgress(int progress)
{
    m_progress = qBound(0, progress, 100);
    update();
}

void CalorieRing::setCalories(int remaining, int target)
{
    m_remaining = std::max(0, remaining);
    m_target = std::max(1, target);
    update();
}

void CalorieRing::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int margin = 18;
    const QRectF ringRect = rect().adjusted(margin, margin, -margin, -margin);
    const int penWidth = 14;

    QPen backgroundPen(QColor("#273244"), penWidth, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(backgroundPen);
    painter.drawArc(ringRect, 0, 360 * 16);

    QPen progressPen(QColor("#3CB371"), penWidth, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(progressPen);
    painter.drawArc(ringRect, 90 * 16, -static_cast<int>(m_progress / 100.0 * 360.0 * 16.0));

    painter.setPen(QColor("#F9FAFB"));
    QFont valueFont = painter.font();
    valueFont.setPointSize(26);
    valueFont.setBold(true);
    painter.setFont(valueFont);
    painter.drawText(ringRect.adjusted(0, 18, 0, -34), Qt::AlignCenter, QString::number(m_remaining));

    QFont labelFont = painter.font();
    labelFont.setPointSize(10);
    labelFont.setBold(false);
    painter.setFont(labelFont);
    painter.setPen(QColor("#9CA3AF"));
    painter.drawText(ringRect.adjusted(0, 54, 0, 0), Qt::AlignCenter, "calories left");
}

MainPage::MainPage(QWidget *parent)
    : QWidget(parent)
    , m_mealAnalyzer(m_databaseManager)
{
    setObjectName("dashboardPage");

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *dashboardScrollArea = new QScrollArea(this);
    dashboardScrollArea->setObjectName("dashboardScrollArea");
    dashboardScrollArea->setFrameShape(QFrame::NoFrame);
    dashboardScrollArea->setWidgetResizable(true);
    dashboardScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    dashboardScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    dashboardScrollArea->viewport()->setAutoFillBackground(false);

    auto *dashboardContent = new QWidget(dashboardScrollArea);
    dashboardContent->setObjectName("dashboardContent");

    auto *layout = new QVBoxLayout(dashboardContent);
    layout->setContentsMargins(34, 28, 34, 28);
    layout->setSpacing(22);

    auto *topLayout = new QHBoxLayout();
    auto *headerBlock = new QVBoxLayout();

    auto *titleLabel = new QLabel("Today's Summary", this);
    titleLabel->setObjectName("dashboardTitle");

    m_dateLabel = new QLabel(QDate::currentDate().toString("dddd, MMMM d"), this);
    m_dateLabel->setObjectName("dashboardDate");

    headerBlock->addWidget(titleLabel);
    headerBlock->addWidget(m_dateLabel);

    m_statusLabel = new QLabel("Ready to log food.", this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_statusLabel->setWordWrap(true);

    topLayout->addLayout(headerBlock);
    topLayout->addStretch();
    topLayout->addWidget(m_statusLabel);

    auto *summarySplitter = new QSplitter(Qt::Horizontal, this);
    summarySplitter->setHandleWidth(8);
    summarySplitter->setChildrenCollapsible(false);

    auto *calorieContent = new QWidget(this);
    auto *calorieLayout = new QVBoxLayout(calorieContent);
    calorieLayout->setSpacing(10);
    calorieLayout->setAlignment(Qt::AlignCenter);

    auto *calorieTitle = new QLabel("Calories remaining", calorieContent);
    calorieTitle->setObjectName("cardTitle");
    calorieTitle->setAlignment(Qt::AlignCenter);

    m_calorieRing = new CalorieRing(calorieContent);

    m_caloriesRemainingLabel = new QLabel(calorieContent);
    m_caloriesRemainingLabel->setObjectName("cardSubtext");
    m_caloriesRemainingLabel->setAlignment(Qt::AlignCenter);

    m_calorieProgressMessageLabel = new QLabel(calorieContent);
    m_calorieProgressMessageLabel->setObjectName("positiveLabel");
    m_calorieProgressMessageLabel->setAlignment(Qt::AlignCenter);
    m_calorieProgressMessageLabel->setVisible(false);

    m_calorieWarningLabel = new QLabel("You have surpassed your daily calorie goal.", calorieContent);
    m_calorieWarningLabel->setObjectName("warningLabel");
    m_calorieWarningLabel->setAlignment(Qt::AlignCenter);
    m_calorieWarningLabel->setVisible(false);

    calorieLayout->addWidget(calorieTitle);
    calorieLayout->addWidget(m_calorieRing, 0, Qt::AlignCenter);
    calorieLayout->addWidget(m_caloriesRemainingLabel);
    calorieLayout->addWidget(m_calorieProgressMessageLabel);
    calorieLayout->addWidget(m_calorieWarningLabel);

    auto *macroContent = new QWidget(this);
    auto *macroLayout = new QVBoxLayout(macroContent);
    macroLayout->setSpacing(14);

    auto *macroTitle = new QLabel("Macros summary", macroContent);
    macroTitle->setObjectName("cardTitle");
    macroLayout->addWidget(macroTitle);
    macroLayout->addWidget(createMacroRow("Protein", "g", &m_proteinBar, &m_proteinValueLabel));
    macroLayout->addWidget(createMacroRow("Carbs", "g", &m_carbsBar, &m_carbsValueLabel));
    macroLayout->addWidget(createMacroRow("Fat", "g", &m_fatBar, &m_fatValueLabel));
    macroLayout->addWidget(createMacroRow("Fiber", "g", &m_fiberBar, &m_fiberValueLabel));
    macroLayout->addStretch();

    summarySplitter->addWidget(createCard(calorieContent));
    summarySplitter->addWidget(createCard(macroContent));
    summarySplitter->setStretchFactor(0, 1);
    summarySplitter->setStretchFactor(1, 2);

    auto *quoteContent = new QWidget(this);
    auto *quoteLayout = new QVBoxLayout(quoteContent);
    quoteLayout->setSpacing(0);
    m_healthQuoteLabel = new QLabel(quoteContent);
    m_healthQuoteLabel->setObjectName("quoteLabel");
    m_healthQuoteLabel->setAlignment(Qt::AlignCenter);
    m_healthQuoteLabel->setWordWrap(true);
    quoteLayout->addWidget(m_healthQuoteLabel);

    auto *foodContent = new QWidget(this);
    auto *foodLayout = new QVBoxLayout(foodContent);
    foodLayout->setSpacing(12);

    auto *foodTitle = new QLabel("Food input", foodContent);
    foodTitle->setObjectName("cardTitle");

    auto *foodInputRow = new QHBoxLayout();
    m_foodSearchEdit = new QLineEdit(foodContent);
    m_foodSearchEdit->setPlaceholderText("Search or enter food (e.g. egg, rice, chicken)");
    m_addButton = new QPushButton("Add Food", foodContent);
    m_addButton->setObjectName("primaryButton");
    m_addButton->setMinimumWidth(112);
    m_previewButton = new QPushButton("Preview Nutrients", foodContent);
    m_previewButton->setObjectName("primaryButton");
    m_previewButton->setMinimumWidth(150);

    foodInputRow->addWidget(m_foodSearchEdit, 1);
    foodInputRow->addWidget(m_addButton);
    foodInputRow->addWidget(m_previewButton);

    auto *confirmationRow = new QHBoxLayout();
    m_inputConfirmationLabel = new QLabel(foodContent);
    m_inputConfirmationLabel->setObjectName("statusLabel");
    m_inputConfirmationLabel->setWordWrap(true);
    m_inputConfirmationLabel->setVisible(false);
    m_confirmInputYesButton = new QPushButton("Yes", foodContent);
    m_confirmInputYesButton->setObjectName("primaryButton");
    m_confirmInputYesButton->setVisible(false);
    m_confirmInputNoButton = new QPushButton("No", foodContent);
    m_confirmInputNoButton->setObjectName("primaryButton");
    m_confirmInputNoButton->setVisible(false);

    confirmationRow->addWidget(m_inputConfirmationLabel, 1);
    confirmationRow->addWidget(m_confirmInputYesButton);
    confirmationRow->addWidget(m_confirmInputNoButton);

    auto *suggestionRow = new QHBoxLayout();
    m_suggestionLabel = new QLabel(foodContent);
    m_suggestionLabel->setObjectName("statusLabel");
    m_suggestionLabel->setWordWrap(true);
    m_suggestionLabel->setVisible(false);
    m_useSuggestionButton = new QPushButton("Use suggestion", foodContent);
    m_useSuggestionButton->setObjectName("primaryButton");
    m_useSuggestionButton->setVisible(false);

    suggestionRow->addWidget(m_suggestionLabel, 1);
    suggestionRow->addWidget(m_useSuggestionButton);

    auto *recentLabel = new QLabel("Recent foods", foodContent);
    recentLabel->setObjectName("sectionLabel");

    m_recentFoodsList = new QListWidget(foodContent);
    m_recentFoodsList->setMaximumHeight(115);
    m_recentFoodsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_recentFoodsList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    foodLayout->addWidget(foodTitle);
    foodLayout->addLayout(foodInputRow);
    foodLayout->addLayout(confirmationRow);
    foodLayout->addLayout(suggestionRow);
    foodLayout->addWidget(recentLabel);
    foodLayout->addWidget(m_recentFoodsList);

    auto *logContent = new QWidget(this);
    auto *logLayout = new QVBoxLayout(logContent);
    logLayout->setSpacing(12);

    auto *logHeaderLayout = new QHBoxLayout();
    auto *logTitle = new QLabel("Daily log", logContent);
    logTitle->setObjectName("cardTitle");
    m_deleteSelectedButton = new QPushButton("Delete Selected", logContent);
    m_deleteSelectedButton->setObjectName("primaryButton");

    logHeaderLayout->addWidget(logTitle);
    logHeaderLayout->addStretch();
    logHeaderLayout->addWidget(m_deleteSelectedButton);

    m_dailyLogTable = new QTableWidget(logContent);
    m_dailyLogTable->setColumnCount(6);
    m_dailyLogTable->setHorizontalHeaderLabels({"Food name", "Quantity (g)", "Calories", "Protein", "Carbs", "Fat"});
    m_dailyLogTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < m_dailyLogTable->columnCount(); ++column) {
        m_dailyLogTable->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    }
    m_dailyLogTable->verticalHeader()->setVisible(false);
    m_dailyLogTable->setAlternatingRowColors(true);
    m_dailyLogTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dailyLogTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dailyLogTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_dailyLogTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    logLayout->addLayout(logHeaderLayout);
    logLayout->addWidget(m_dailyLogTable, 1);

    auto *nutrientContent = new QWidget(this);
    auto *nutrientLayout = new QVBoxLayout(nutrientContent);
    nutrientLayout->setSpacing(12);

    m_nutrientBreakdownTitle = new QLabel("Nutrient breakdown - Daily totals", nutrientContent);
    m_nutrientBreakdownTitle->setObjectName("cardTitle");

    m_nutrientBreakdownTable = new QTableWidget(nutrientContent);
    m_nutrientBreakdownTable->setColumnCount(4);
    m_nutrientBreakdownTable->setHorizontalHeaderLabels({"Nutrient", "Amount", "Unit", "Status"});
    m_nutrientBreakdownTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_nutrientBreakdownTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_nutrientBreakdownTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_nutrientBreakdownTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_nutrientBreakdownTable->verticalHeader()->setVisible(false);
    m_nutrientBreakdownTable->setAlternatingRowColors(true);
    m_nutrientBreakdownTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_nutrientBreakdownTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_nutrientBreakdownTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_nutrientBreakdownTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_nutrientBreakdownTable->setMinimumHeight(220);

    nutrientLayout->addWidget(m_nutrientBreakdownTitle);
    nutrientLayout->addWidget(m_nutrientBreakdownTable);

    auto *dashboardSplitter = new QSplitter(Qt::Vertical, this);
    dashboardSplitter->setHandleWidth(8);
    dashboardSplitter->setChildrenCollapsible(false);
    dashboardSplitter->addWidget(summarySplitter);
    dashboardSplitter->addWidget(createCard(quoteContent));
    dashboardSplitter->addWidget(createCard(foodContent));
    dashboardSplitter->addWidget(createCard(logContent));
    dashboardSplitter->addWidget(createCard(nutrientContent));
    dashboardSplitter->setStretchFactor(0, 2);
    dashboardSplitter->setStretchFactor(1, 1);
    dashboardSplitter->setStretchFactor(2, 2);
    dashboardSplitter->setStretchFactor(3, 3);
    dashboardSplitter->setStretchFactor(4, 2);

    layout->addLayout(topLayout);
    layout->addWidget(dashboardSplitter, 1);
    dashboardScrollArea->setWidget(dashboardContent);
    rootLayout->addWidget(dashboardScrollArea);

    setStyleSheet(
        "QWidget#dashboardPage { background: #111827; color: #F9FAFB; "
        "font-family: 'Inter', 'Segoe UI', 'Helvetica Neue', sans-serif; }"
        "QWidget#dashboardContent { background: #111827; }"
        "QScrollArea#dashboardScrollArea { background: #111827; border: none; }"
        "QLabel#dashboardTitle { font-size: 28px; font-weight: 800; color: #F9FAFB; }"
        "QLabel#dashboardDate, QLabel#cardSubtext, QLabel#statusLabel { color: #9CA3AF; font-size: 13px; }"
        "QLabel#positiveLabel { color: #3CB371; font-size: 13px; font-weight: 700; }"
        "QLabel#warningLabel { color: #EF4444; font-size: 13px; font-weight: 700; }"
        "QLabel#quoteLabel { color: #D1D5DB; font-size: 15px; font-weight: 650; padding: 2px 4px; }"
        "QLabel#cardTitle { color: #F9FAFB; font-size: 17px; font-weight: 750; }"
        "QLabel#sectionLabel { color: #D1D5DB; font-size: 13px; font-weight: 650; }"
        "QWidget#card { background: #1F2937; border: 1px solid #2D3748; border-radius: 12px; }"
        "QLineEdit { background: #111827; color: #F9FAFB; border: 1px solid #374151; "
        "border-radius: 10px; padding: 11px 13px; font-size: 14px; }"
        "QLineEdit:focus { border: 1px solid #3CB371; }"
        "QPushButton#primaryButton { background: #3CB371; color: #08111F; border: none; "
        "border-radius: 10px; padding: 11px 18px; font-weight: 750; }"
        "QPushButton#primaryButton:hover { background: #49C27F; }"
        "QPushButton#primaryButton:disabled { background: #2A5740; color: #9CA3AF; }"
        "QProgressBar { background: #111827; border: none; border-radius: 6px; height: 10px; }"
        "QProgressBar::chunk { background: #3CB371; border-radius: 6px; }"
        "QSplitter::handle { background: #172033; border-radius: 3px; }"
        "QListWidget, QTableWidget { background: #111827; color: #F9FAFB; border: 1px solid #2D3748; "
        "border-radius: 10px; gridline-color: #2D3748; }"
        "QListWidget::item { padding: 8px 10px; }"
        "QTableWidget::item { padding: 8px; }"
        "QHeaderView::section { background: #172033; color: #D1D5DB; border: none; "
        "border-bottom: 1px solid #2D3748; padding: 8px; font-weight: 700; }");

    connect(m_addButton, &QPushButton::clicked, this, [this]() { addFoodInput(); });
    connect(m_previewButton, &QPushButton::clicked, this, [this]() { previewFoodInput(); });
    connect(m_confirmInputYesButton, &QPushButton::clicked, this, [this]() { confirmFoodInput(); });
    connect(m_confirmInputNoButton, &QPushButton::clicked, this, [this]() { cancelFoodInputConfirmation(); });
    connect(m_useSuggestionButton, &QPushButton::clicked, this, [this]() { acceptFoodSuggestion(); });
    connect(m_deleteSelectedButton, &QPushButton::clicked, this, [this]() { deleteSelectedFood(); });
    connect(m_foodSearchEdit, &QLineEdit::returnPressed, this, [this]() { addFoodInput(); });
    connect(m_foodSearchEdit, &QLineEdit::textChanged, this, [this]() {
        clearFoodSuggestion();
        clearFoodInputConfirmation();
    });
    connect(m_dailyLogTable, &QTableWidget::itemSelectionChanged, this, [this]() { refreshNutrientBreakdown(); });

    setupHealthQuotes();
    setupEndOfDayGoalReview();

    QString errorMessage;
    const bool initialized = m_databaseManager.initialize(&errorMessage);
    if (initialized) {
        refreshImportState();
        refreshDailyLog();
        refreshDashboard();
        refreshNutrientBreakdown();
        if (!m_hasUSDAData) {
            QTimer::singleShot(0, this, [this]() { setupFoodDatabase(); });
        }
    } else {
        m_statusLabel->setText("Database initialization failed: " + errorMessage);
        m_addButton->setEnabled(false);
        m_previewButton->setEnabled(false);
        m_confirmInputYesButton->setEnabled(false);
        m_confirmInputNoButton->setEnabled(false);
        m_useSuggestionButton->setEnabled(false);
        m_deleteSelectedButton->setEnabled(false);
        m_foodSearchEdit->setEnabled(false);
    }
}

QWidget *MainPage::createCard(QWidget *content)
{
    auto *card = new QWidget(this);
    card->setObjectName("card");
    card->setMinimumSize(220, 120);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->addWidget(content);

    applyShadow(card);
    return card;
}

QWidget *MainPage::createMacroRow(const QString &label, const QString &unit, QProgressBar **bar, QLabel **valueLabel)
{
    auto *row = new QWidget(this);
    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *topLine = new QHBoxLayout();
    auto *nameLabel = new QLabel(label, row);
    nameLabel->setStyleSheet("color: #D1D5DB; font-weight: 650;");

    *valueLabel = new QLabel("0 " + unit, row);
    (*valueLabel)->setStyleSheet("color: #9CA3AF;");
    (*valueLabel)->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    topLine->addWidget(nameLabel);
    topLine->addStretch();
    topLine->addWidget(*valueLabel);

    *bar = new QProgressBar(row);
    (*bar)->setRange(0, 100);
    (*bar)->setValue(0);
    (*bar)->setTextVisible(false);

    layout->addLayout(topLine);
    layout->addWidget(*bar);

    return row;
}

void MainPage::setupFoodDatabase()
{
    if (m_hasUSDAData || m_usdaImportInProgress) {
        return;
    }

    m_databaseSetupError.clear();
    m_usdaImportInProgress = true;
    refreshImportState();

    const QString databaseFile = findBundledDatabaseFile();
    if (databaseFile.isEmpty()) {
        m_usdaImportInProgress = false;
        m_databaseSetupError = "Food database package is missing. Bundle data/nutrient_tracker.sqlite before building.";
        refreshImportState();
        return;
    }

    QString errorMessage;
    const bool installed = m_databaseManager.installDatabaseFromFile(databaseFile, &errorMessage);
    m_usdaImportInProgress = false;

    if (!installed) {
        m_databaseSetupError = "Food database setup failed: " + errorMessage;
        refreshImportState();
        return;
    }

    m_hasUSDAData = m_databaseManager.hasImportedUSDAData();
    if (m_hasUSDAData) {
        m_statusLabel->setText("Food database ready. Search a food to log it.");
        refreshImportState();
    } else {
        m_databaseSetupError = "Bundled food database is empty or invalid.";
        refreshImportState();
    }
}

QString MainPage::findBundledDatabaseFile() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString currentDir = QDir::currentPath();
    const QStringList candidates = {
        appDir + "/data/nutrient_tracker.sqlite",
        appDir + "/../Resources/nutrient_tracker.sqlite",
        appDir + "/../Resources/data/nutrient_tracker.sqlite",
        appDir + "/../data/nutrient_tracker.sqlite",
        appDir + "/../../../../data/nutrient_tracker.sqlite",
        currentDir + "/data/nutrient_tracker.sqlite"
    };

    for (const QString &candidate : candidates) {
        const QString normalizedPath = QDir::cleanPath(candidate);
        const QFileInfo fileInfo(normalizedPath);
        if (fileInfo.exists() && fileInfo.isFile() && fileInfo.size() > 0) {
            return normalizedPath;
        }
    }

    return {};
}

void MainPage::addFoodInput()
{
    showInputConfirmation(normalizeFoodInput(m_foodSearchEdit->text()), FoodInputAction::AddFood);
}

void MainPage::previewFoodInput()
{
    showInputConfirmation(normalizeFoodInput(m_foodSearchEdit->text()), FoodInputAction::TryFood);
}

QString MainPage::normalizeFoodInput(const QString &rawInput) const
{
    QString normalized = rawInput.trimmed().toLower();
    normalized.replace(QRegularExpression("\\s+"), " ");

    if (normalized.startsWith("1 ")) {
        normalized.replace(0, 1, "one");
    }

    const QStringList singularUnits = {
        "cups", "tablespoons", "teaspoons", "ounces", "servings", "pieces", "slices"
    };
    for (const QString &unit : singularUnits) {
        const QString singular = unit.left(unit.size() - 1);
        normalized.replace(QRegularExpression("\\bone\\s+" + unit + "\\b"), "one " + singular);
    }

    return normalized;
}

void MainPage::showInputConfirmation(const QString &normalizedInput, FoodInputAction action)
{
    if (!m_hasUSDAData) {
        refreshImportState();
        return;
    }

    if (normalizedInput.isEmpty()) {
        clearFoodInputConfirmation();
        m_statusLabel->setText("Enter a food to add.");
        return;
    }

    clearFoodSuggestion();
    m_pendingConfirmedInput = normalizedInput;
    m_pendingFoodInputAction = action;

    m_inputConfirmationLabel->setText("Do you mean: " + normalizedInput + "?");
    m_inputConfirmationLabel->setVisible(true);
    m_confirmInputYesButton->setVisible(true);
    m_confirmInputNoButton->setVisible(true);
    m_statusLabel->setText("Please confirm the food before I calculate it.");
}

void MainPage::confirmFoodInput()
{
    if (m_pendingFoodInputAction == FoodInputAction::None || m_pendingConfirmedInput.isEmpty()) {
        clearFoodInputConfirmation();
        return;
    }

    const QString confirmedInput = m_pendingConfirmedInput;
    const bool shouldLog = m_pendingFoodInputAction == FoodInputAction::AddFood;

    clearFoodInputConfirmation();
    m_foodSearchEdit->setText(confirmedInput);
    handleFoodInput(shouldLog);
}

void MainPage::cancelFoodInputConfirmation()
{
    clearFoodInputConfirmation();
    m_statusLabel->setText("No problem. Edit the food and try again.");
}

void MainPage::clearFoodInputConfirmation()
{
    m_pendingConfirmedInput.clear();
    m_pendingFoodInputAction = FoodInputAction::None;
    if (m_inputConfirmationLabel) {
        m_inputConfirmationLabel->clear();
        m_inputConfirmationLabel->setVisible(false);
    }
    if (m_confirmInputYesButton) {
        m_confirmInputYesButton->setVisible(false);
    }
    if (m_confirmInputNoButton) {
        m_confirmInputNoButton->setVisible(false);
    }
}

void MainPage::setupHealthQuotes()
{
    m_healthQuotes = {
        "Take care of your body. It's the only place you have to live.",
        "Small choices become healthy habits.",
        "Eat well, live well.",
        "Your health is an investment, not an expense.",
        "Progress, not perfection.",
        "Healthy habits start with one meal."
    };

    if (!m_healthQuoteLabel || m_healthQuotes.isEmpty()) {
        return;
    }

    m_currentHealthQuoteIndex = 0;
    m_healthQuoteLabel->setText(m_healthQuotes.first());
    m_healthQuoteOpacityEffect = new QGraphicsOpacityEffect(m_healthQuoteLabel);
    m_healthQuoteOpacityEffect->setOpacity(1.0);
    m_healthQuoteLabel->setGraphicsEffect(m_healthQuoteOpacityEffect);

    auto *quoteTimer = new QTimer(this);
    connect(quoteTimer, &QTimer::timeout, this, [this]() { rotateHealthQuote(); });
    quoteTimer->start(12000);
}

void MainPage::rotateHealthQuote()
{
    if (m_healthQuotes.size() < 2 || !m_healthQuoteLabel || !m_healthQuoteOpacityEffect) {
        return;
    }

    fadeToNextQuote();
}

void MainPage::fadeToNextQuote()
{
    auto *fadeOut = new QPropertyAnimation(m_healthQuoteOpacityEffect, "opacity", this);
    fadeOut->setDuration(350);
    fadeOut->setStartValue(m_healthQuoteOpacityEffect->opacity());
    fadeOut->setEndValue(0.0);

    connect(fadeOut, &QPropertyAnimation::finished, this, [this]() {
        m_currentHealthQuoteIndex = (m_currentHealthQuoteIndex + 1) % m_healthQuotes.size();
        m_healthQuoteLabel->setText(m_healthQuotes.at(m_currentHealthQuoteIndex));

        auto *fadeIn = new QPropertyAnimation(m_healthQuoteOpacityEffect, "opacity", this);
        fadeIn->setDuration(350);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    });

    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainPage::setupEndOfDayGoalReview()
{
    m_currentReviewDate = QDate::currentDate();
    m_endOfDayReviewTimer = new QTimer(this);
    connect(m_endOfDayReviewTimer, &QTimer::timeout, this, [this]() {
        checkForEndOfDayGoalReview();
    });
    m_endOfDayReviewTimer->start(60000);
}

void MainPage::checkForEndOfDayGoalReview()
{
    const QDate today = QDate::currentDate();
    if (!m_currentReviewDate.isValid() || today == m_currentReviewDate) {
        return;
    }

    const QDate completedDate = m_currentReviewDate;
    const QVector<EndOfDayGoalReviewMessage> messages =
        generateEndOfDayGoalReview(dailyTotalNutrients());
    m_currentReviewDate = today;
    m_dateLabel->setText(today.toString("dddd, MMMM d"));
    showEndOfDayGoalReview(completedDate, messages);
}

QVector<MainPage::EndOfDayGoalReviewMessage> MainPage::generateEndOfDayGoalReview(
    const QVector<NutrientValue> &dailyTotals) const
{
    struct GoalDefinition {
        QString key;
        QString label;
        double target = 0.0;
        bool maximumOnly = false;
    };

    const QVector<GoalDefinition> goals = {
        {"calories", "calorie", m_calorieTarget, false},
        {"protein", "protein", m_proteinTarget, false},
        {"carbs", "carbohydrate", m_carbsTarget, false},
        {"fat", "fat", m_fatTarget, false},
        {"fiber", "fiber", m_fiberTarget, false},
        {"sugar", "sugar", 50.0, true}
    };

    QVector<EndOfDayGoalReviewMessage> messages;
    for (const GoalDefinition &goal : goals) {
        const double amount = displayAmountForKey(dailyTotals, goal.key);
        if (goal.maximumOnly) {
            if (amount <= goal.target) {
                messages.append({"Great job, you stayed within your sugar goal.", true});
            } else {
                messages.append({"Yesterday you passed your sugar goal.", false});
            }
            continue;
        }

        const double lowerBound = goal.target * 0.95;
        const double upperBound = goal.target * 1.05;
        if (amount < lowerBound) {
            if (goal.key == "calories") {
                messages.append({"Yesterday you were below your average calorie goal.", false});
            } else {
                messages.append({"You did not meet your " + goal.label + " goal yesterday.", false});
            }
        } else if (amount > upperBound) {
            messages.append({"Yesterday you passed your " + goal.label + " goal.", false});
        } else {
            messages.append({"Great job, you met your " + goal.label + " goal. Keep going.", true});
        }
    }

    return messages;
}

void MainPage::showEndOfDayGoalReview(
    const QDate &completedDate,
    const QVector<EndOfDayGoalReviewMessage> &messages)
{
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setObjectName("reviewDialog");
    dialog->setWindowTitle("End-of-day review");
    dialog->resize(460, 420);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(14);

    auto *titleLabel = new QLabel("End-of-day goal review", dialog);
    titleLabel->setObjectName("reviewTitle");

    auto *dateLabel = new QLabel("Review for " + completedDate.toString("dddd, MMMM d"), dialog);
    dateLabel->setObjectName("reviewSubtitle");

    auto *scrollArea = new QScrollArea(dialog);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *messageContent = new QWidget(scrollArea);
    auto *messageLayout = new QVBoxLayout(messageContent);
    messageLayout->setContentsMargins(0, 0, 0, 0);
    messageLayout->setSpacing(8);

    for (const EndOfDayGoalReviewMessage &message : messages) {
        auto *messageLabel = new QLabel(message.text, messageContent);
        messageLabel->setObjectName(message.positive ? "reviewPositive" : "reviewWarning");
        messageLabel->setWordWrap(true);
        messageLayout->addWidget(messageLabel);
    }
    messageLayout->addStretch();
    scrollArea->setWidget(messageContent);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, dialog);
    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);

    layout->addWidget(titleLabel);
    layout->addWidget(dateLabel);
    layout->addWidget(scrollArea, 1);
    layout->addWidget(buttons);

    dialog->setStyleSheet(
        "QDialog#reviewDialog { background: #111827; color: #F9FAFB; "
        "font-family: 'Inter', 'Segoe UI', 'Helvetica Neue', sans-serif; }"
        "QLabel#reviewTitle { color: #F9FAFB; font-size: 20px; font-weight: 800; }"
        "QLabel#reviewSubtitle { color: #9CA3AF; font-size: 13px; }"
        "QLabel#reviewPositive { color: #3CB371; font-size: 14px; font-weight: 700; "
        "background: #10261C; border-radius: 8px; padding: 10px; }"
        "QLabel#reviewWarning { color: #EF4444; font-size: 14px; font-weight: 700; "
        "background: #2B151A; border-radius: 8px; padding: 10px; }"
        "QScrollArea { background: transparent; border: none; }"
        "QDialogButtonBox QPushButton { background: #3CB371; color: #08111F; border: none; "
        "border-radius: 8px; padding: 9px 18px; font-weight: 750; }");

    dialog->show();
}

void MainPage::acceptFoodSuggestion()
{
    if (m_suggestedInput.isEmpty()) {
        return;
    }

    const QString suggestedInput = m_suggestedInput;
    m_foodSearchEdit->setText(suggestedInput);
    clearFoodSuggestion();
    m_statusLabel->setText("Using suggestion: " + suggestedInput);
}

void MainPage::clearFoodSuggestion()
{
    m_suggestedFoodName.clear();
    m_suggestedInput.clear();
    if (m_suggestionLabel) {
        m_suggestionLabel->clear();
        m_suggestionLabel->setVisible(false);
    }
    if (m_useSuggestionButton) {
        m_useSuggestionButton->setVisible(false);
    }
}

void MainPage::showFoodSuggestion(const QString &input, const QVector<QString> &unmatchedFoods)
{
    if (unmatchedFoods.isEmpty()) {
        showFoodUnavailableMessage();
        return;
    }

    const QString misspelledFood = unmatchedFoods.first();
    const QString suggestion = findClosestFoodMatch(misspelledFood);
    if (suggestion.isEmpty()) {
        showFoodUnavailableMessage();
        return;
    }

    m_suggestedFoodName = suggestion;
    m_suggestedInput = buildSuggestedInput(input, misspelledFood, suggestion);
    m_suggestionLabel->setText("Food not found. Did you mean " + suggestion + "?");
    m_suggestionLabel->setVisible(true);
    m_useSuggestionButton->setVisible(true);
}

void MainPage::showFoodUnavailableMessage()
{
    m_suggestedFoodName.clear();
    m_suggestedInput.clear();
    if (m_suggestionLabel) {
        m_suggestionLabel->setText(foodUnavailableMessage());
        m_suggestionLabel->setVisible(true);
    }
    if (m_useSuggestionButton) {
        m_useSuggestionButton->setVisible(false);
    }
}

QString MainPage::findClosestFoodMatch(const QString &foodName) const
{
    const QString normalizedFood = normalizeFoodText(foodName);
    if (normalizedFood.size() < 3) {
        return {};
    }

    QSet<QString> searchTerms;
    auto addSearchTerms = [&searchTerms](const QString &text) {
        const QString normalized = normalizeFoodText(text);
        if (normalized.size() >= 3) {
            searchTerms.insert(normalized);
            searchTerms.insert(normalized.left(5));
        }

        const QStringList words = normalized.split(' ', Qt::SkipEmptyParts);
        for (const QString &word : words) {
            if (word.size() >= 3) {
                searchTerms.insert(word.left(3));
                searchTerms.insert(word.left(5));
            }
        }
    };

    addSearchTerms(normalizedFood);
    const QString collapsedFood = collapseRepeatedLetters(normalizedFood);
    if (collapsedFood != normalizedFood) {
        addSearchTerms(collapsedFood);
    }

    if (searchTerms.isEmpty()) {
        return {};
    }

    QVector<FoodRecord> candidates;
    QSet<int> seenIds;
    QStringList orderedTerms = searchTerms.values();
    orderedTerms.sort();
    for (const QString &term : orderedTerms) {
        const QVector<FoodRecord> matches = m_databaseManager.searchFoods(term);
        for (const FoodRecord &match : matches) {
            if (!seenIds.contains(match.id)) {
                seenIds.insert(match.id);
                candidates.append(match);
            }
        }
    }

    QString bestName;
    int bestScore = 0;

    for (const FoodRecord &candidate : candidates) {
        QString candidateName = candidate.name;
        const int commaIndex = candidateName.indexOf(',');
        if (commaIndex > 0) {
            candidateName = candidateName.left(commaIndex);
        }
        candidateName = candidateName.toLower().trimmed();

        const int score = getFoodMatchConfidence(normalizedFood, candidateName);
        if (score > bestScore || (score == bestScore && candidateName.size() < bestName.size())) {
            bestScore = score;
            bestName = candidateName;
        }
    }

    if (isReliableFoodSuggestion(bestScore)) {
        return bestName;
    }

    return {};
}

int MainPage::getFoodMatchConfidence(const QString &input, const QString &candidate) const
{
    const QString normalizedInput = normalizeFoodText(input);
    const QString normalizedCandidate = normalizeFoodText(candidate);
    if (normalizedInput.isEmpty() || normalizedCandidate.isEmpty()) {
        return 0;
    }

    if (normalizedInput == normalizedCandidate) {
        return 100;
    }

    int score = 0;
    const QString collapsedInput = collapseRepeatedLetters(normalizedInput);
    const QString collapsedCandidate = collapseRepeatedLetters(normalizedCandidate);
    if (collapsedInput == collapsedCandidate) {
        score = 94;
    }
    if (isSingleAdjacentTransposition(normalizedInput, normalizedCandidate)) {
        score = std::max(score, 90);
    }

    const int distance = levenshteinDistance(normalizedInput, normalizedCandidate);
    const int maxLength = static_cast<int>(std::max(normalizedInput.size(), normalizedCandidate.size()));
    const int similarityScore = maxLength > 0
        ? static_cast<int>(((1.0 - static_cast<double>(distance) / maxLength) * 100.0) + 0.5)
        : 0;
    score = std::max(score, similarityScore);

    if ((normalizedInput.startsWith(normalizedCandidate) || normalizedCandidate.startsWith(normalizedInput))
        && (normalizedInput.size() > normalizedCandidate.size()
            ? normalizedInput.size() - normalizedCandidate.size()
            : normalizedCandidate.size() - normalizedInput.size()) <= 2) {
        score = std::max(score, 86);
    }

    return qBound(0, score, 100);
}

bool MainPage::isReliableFoodSuggestion(int score) const
{
    return score >= 78;
}

QString MainPage::buildSuggestedInput(
    const QString &input,
    const QString &misspelledFood,
    const QString &suggestedFood) const
{
    QString suggestedInput = input;
    const QRegularExpression pattern(
        QRegularExpression::escape(misspelledFood),
        QRegularExpression::CaseInsensitiveOption);
    suggestedInput.replace(pattern, suggestedFood);
    return suggestedInput == input ? suggestedFood : suggestedInput;
}

void MainPage::handleFoodInput(bool shouldLog)
{
    if (!m_hasUSDAData) {
        refreshImportState();
        return;
    }

    const QString input = m_foodSearchEdit->text().trimmed();
    if (input.isEmpty()) {
        m_statusLabel->setText("Enter a food to add.");
        return;
    }

    const bool wasOverCalorieGoal = shouldLog && isOverCalorieGoal(totalFor("calories"), m_calorieTarget);
    const AnalysisResult result = m_mealAnalyzer.analyze(input);

    QStringList messages;
    if (!result.unmatchedFoods.isEmpty()) {
        QStringList unmatchedFoods;
        for (const QString &food : result.unmatchedFoods) {
            unmatchedFoods.append(food);
        }
        messages.append("Unmatched: " + unmatchedFoods.join(", "));
    }
    if (!result.warnings.isEmpty()) {
        for (const QString &warning : result.warnings) {
            messages.append(warning);
        }
    }

    const bool shouldShowSuggestion = result.matchedFoods.isEmpty() && !result.unmatchedFoods.isEmpty();
    if (shouldShowSuggestion) {
        showFoodSuggestion(input, result.unmatchedFoods);
    } else {
        clearFoodSuggestion();
    }

    if (result.matchedFoods.isEmpty()) {
        m_hasPreview = false;
        m_previewNutrients.clear();
        m_previewTitle.clear();
        if (!shouldLog) {
            m_dailyLogTable->clearSelection();
        }

        const QString statusText = shouldShowSuggestion && !m_suggestedFoodName.isEmpty()
            ? "Food not found. Did you mean " + m_suggestedFoodName + "?"
            : foodUnavailableMessage();
        m_statusLabel->setText(statusText);
        refreshNutrientBreakdown();
        return;
    }

    if (shouldLog) {
        for (const AnalyzedFoodItem &food : result.matchedFoods) {
            addLoggedFood(food);
        }

        m_hasPreview = false;
        m_previewNutrients.clear();
        m_previewTitle.clear();

        if (messages.isEmpty()) {
            messages.append(QString("Added %1 item(s).").arg(result.matchedFoods.size()));
        }
    } else {
        m_previewNutrients = result.totals;
        m_previewTitle = input;
        m_hasPreview = !result.matchedFoods.isEmpty();
        m_dailyLogTable->clearSelection();

        if (messages.isEmpty()) {
            messages.append(QString("Previewing %1 item(s).").arg(result.matchedFoods.size()));
        }
    }

    QString statusText = messages.join(" ");
    if (shouldShowSuggestion) {
        statusText = m_suggestedFoodName.isEmpty()
            ? foodUnavailableMessage()
            : "Food not found. Did you mean " + m_suggestedFoodName + "?";
    }

    if (shouldLog && !result.matchedFoods.isEmpty()) {
        m_foodSearchEdit->clear();
    }

    if (shouldLog) {
        refreshDailyLog();
        refreshDashboard();
        if (!wasOverCalorieGoal && isOverCalorieGoal(totalFor("calories"), m_calorieTarget)) {
            statusText += " You have surpassed your daily calorie goal.";
        }
    }
    m_statusLabel->setText(statusText);
    refreshNutrientBreakdown();
}

void MainPage::addLoggedFood(const AnalyzedFoodItem &food)
{
    m_loggedFoods.append(food);
    m_recentFoodsList->insertItem(0, food.food.name);
    while (m_recentFoodsList->count() > 5) {
        delete m_recentFoodsList->takeItem(m_recentFoodsList->count() - 1);
    }
}

void MainPage::deleteSelectedFood()
{
    const QList<QTableWidgetSelectionRange> selectedRanges = m_dailyLogTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
        m_statusLabel->setText("Please select a food to delete.");
        return;
    }

    const int row = selectedRanges.first().topRow();
    if (row < 0 || row >= m_loggedFoods.size()) {
        m_statusLabel->setText("Please select a food to delete.");
        return;
    }

    const QString foodName = m_loggedFoods.at(row).food.name;
    m_loggedFoods.removeAt(row);
    m_hasPreview = false;
    m_previewNutrients.clear();
    m_previewTitle.clear();

    refreshDailyLog();
    m_dailyLogTable->clearSelection();
    m_dailyLogTable->setCurrentCell(-1, -1);
    refreshDashboard();
    refreshNutrientBreakdown();
    m_statusLabel->setText("Deleted " + foodName + " from today's log.");
}

void MainPage::refreshDashboard()
{
    const double calories = totalFor("calories");
    const bool overCalorieGoal = isOverCalorieGoal(calories, m_calorieTarget);
    const int caloriesRemaining = std::max(0, static_cast<int>(m_calorieTarget - calories));
    const int caloriePercent = percentOf(calories, m_calorieTarget);

    m_caloriesRemainingLabel->setText(
        QString("%1 kcal target | %2 consumed")
            .arg(formatAmount(m_calorieTarget))
            .arg(formatAmount(calories)));
    m_caloriesRemainingLabel->setStyleSheet(overCalorieGoal ? "color: #EF4444; font-size: 13px; font-weight: 700;" : "");
    const QString calorieProgressMessage = getCalorieProgressMessage(calories, m_calorieTarget);
    m_calorieProgressMessageLabel->setText(calorieProgressMessage);
    m_calorieProgressMessageLabel->setVisible(!overCalorieGoal && !calorieProgressMessage.isEmpty());
    m_calorieProgressMessageLabel->setStyleSheet(
        calories < (m_calorieTarget * 0.5)
            ? "color: #9CA3AF; font-size: 13px; font-weight: 650;"
            : "");
    m_calorieWarningLabel->setVisible(overCalorieGoal);
    m_calorieRing->setCalories(caloriesRemaining, static_cast<int>(m_calorieTarget));

    auto *ringAnimation = new QPropertyAnimation(m_calorieRing, "progress", m_calorieRing);
    ringAnimation->setDuration(420);
    ringAnimation->setStartValue(m_calorieRing->progress());
    ringAnimation->setEndValue(caloriePercent);
    ringAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    const double protein = totalFor("protein");
    const double carbs = totalFor("carbs");
    const double fat = totalFor("fat");
    const double fiber = totalFor("fiber");

    m_proteinValueLabel->setText(formatAmount(protein) + " g");
    m_carbsValueLabel->setText(formatAmount(carbs) + " g");
    m_fatValueLabel->setText(formatAmount(fat) + " g");
    m_fiberValueLabel->setText(formatAmount(fiber) + " g");

    animateProgress(m_proteinBar, percentOf(protein, m_proteinTarget));
    animateProgress(m_carbsBar, percentOf(carbs, m_carbsTarget));
    animateProgress(m_fatBar, percentOf(fat, m_fatTarget));
    animateProgress(m_fiberBar, percentOf(fiber, m_fiberTarget));
}

void MainPage::refreshDailyLog()
{
    m_dailyLogTable->setRowCount(m_loggedFoods.size());

    for (int row = 0; row < m_loggedFoods.size(); ++row) {
        const AnalyzedFoodItem &food = m_loggedFoods.at(row);
        const QVector<NutrientValue> &nutrients = food.scaledNutrients;

        m_dailyLogTable->setItem(row, 0, new QTableWidgetItem(food.food.name));
        m_dailyLogTable->setItem(row, 1, new QTableWidgetItem(formatAmount(food.grams)));
        m_dailyLogTable->setItem(row, 2, new QTableWidgetItem(formatAmount(nutrientAmount(nutrients, "calories"))));
        m_dailyLogTable->setItem(row, 3, new QTableWidgetItem(formatAmount(nutrientAmount(nutrients, "protein"))));
        m_dailyLogTable->setItem(row, 4, new QTableWidgetItem(formatAmount(nutrientAmount(nutrients, "carbs"))));
        m_dailyLogTable->setItem(row, 5, new QTableWidgetItem(formatAmount(nutrientAmount(nutrients, "fat"))));
    }

    const int visibleRows = std::max(1, m_dailyLogTable->rowCount());
    const int rowHeight = m_dailyLogTable->verticalHeader()->defaultSectionSize();
    const int tableHeight = m_dailyLogTable->horizontalHeader()->height() + (visibleRows * rowHeight) + 8;
    m_dailyLogTable->setMinimumHeight(tableHeight);
}

void MainPage::refreshNutrientBreakdown()
{
    const QVector<NutrientValue> nutrients = currentBreakdownNutrients();
    const QVector<TrackedNutrientDefinition> definitions = trackedNutrientDefinitions();

    const QList<QTableWidgetSelectionRange> selectedRanges = m_dailyLogTable->selectedRanges();
    const bool showDailyStatus = selectedRanges.isEmpty() && !m_hasPreview;
    if (!selectedRanges.isEmpty()) {
        const int row = selectedRanges.first().topRow();
        if (row >= 0 && row < m_loggedFoods.size()) {
            m_nutrientBreakdownTitle->setText("Nutrient breakdown - " + m_loggedFoods.at(row).food.name);
        }
    } else if (m_hasPreview) {
        m_nutrientBreakdownTitle->setText("Nutrient preview - " + m_previewTitle);
    } else {
        m_nutrientBreakdownTitle->setText("Nutrient breakdown - Daily totals");
    }

    int row = 0;
    QString currentCategory;
    m_nutrientBreakdownTable->clearContents();
    m_nutrientBreakdownTable->setRowCount(0);

    for (const TrackedNutrientDefinition &definition : definitions) {
        if (definition.category != currentCategory) {
            currentCategory = definition.category;
            m_nutrientBreakdownTable->insertRow(row);
            auto *categoryItem = new QTableWidgetItem(currentCategory);
            QFont font = categoryItem->font();
            font.setBold(true);
            categoryItem->setFont(font);
            categoryItem->setForeground(QBrush(QColor("#F9FAFB")));
            categoryItem->setBackground(QBrush(QColor("#172033")));
            m_nutrientBreakdownTable->setItem(row, 0, categoryItem);
            m_nutrientBreakdownTable->setSpan(row, 0, 1, 4);
            ++row;
        }

        const double amount = displayAmountForKey(nutrients, definition.key);
        const QString unit = displayUnitForKey(nutrients, definition);
        const NutrientStatus status = showDailyStatus
            ? getNutrientStatus(definition.key, amount, m_calorieTarget)
            : NutrientStatus{};

        m_nutrientBreakdownTable->insertRow(row);
        auto *nameItem = new QTableWidgetItem(definition.label);
        auto *amountItem = new QTableWidgetItem(formatAmount(amount));
        auto *statusItem = new QTableWidgetItem(status.label);
        statusItem->setForeground(QBrush(status.color));
        if (status.label != "Unknown") {
            amountItem->setForeground(QBrush(status.color));
        }

        m_nutrientBreakdownTable->setItem(row, 0, nameItem);
        m_nutrientBreakdownTable->setItem(row, 1, amountItem);
        m_nutrientBreakdownTable->setItem(row, 2, new QTableWidgetItem(unit));
        m_nutrientBreakdownTable->setItem(row, 3, statusItem);
        ++row;
    }
}

QVector<NutrientValue> MainPage::dailyTotalNutrients() const
{
    QHash<QString, NutrientValue> totalsByKey;
    for (const AnalyzedFoodItem &food : m_loggedFoods) {
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

QVector<NutrientValue> MainPage::currentBreakdownNutrients() const
{
    const QList<QTableWidgetSelectionRange> selectedRanges = m_dailyLogTable->selectedRanges();
    if (!selectedRanges.isEmpty()) {
        const int row = selectedRanges.first().topRow();
        if (row >= 0 && row < m_loggedFoods.size()) {
            return m_loggedFoods.at(row).scaledNutrients;
        }
    }

    if (m_hasPreview) {
        return m_previewNutrients;
    }

    return dailyTotalNutrients();
}

void MainPage::refreshImportState()
{
    m_hasUSDAData = m_databaseManager.hasImportedUSDAData();

    m_addButton->setEnabled(m_hasUSDAData && !m_usdaImportInProgress);
    m_previewButton->setEnabled(m_hasUSDAData && !m_usdaImportInProgress);
    m_confirmInputYesButton->setEnabled(m_hasUSDAData && !m_usdaImportInProgress);
    m_confirmInputNoButton->setEnabled(m_hasUSDAData && !m_usdaImportInProgress);
    m_useSuggestionButton->setEnabled(m_hasUSDAData && !m_usdaImportInProgress);
    m_deleteSelectedButton->setEnabled(m_hasUSDAData && !m_usdaImportInProgress);
    m_foodSearchEdit->setEnabled(m_hasUSDAData && !m_usdaImportInProgress);

    if (m_hasUSDAData) {
        m_foodSearchEdit->setPlaceholderText("Search or enter food (e.g. egg, rice, chicken)");
        if (m_statusLabel->text().isEmpty()
            || m_statusLabel->text().startsWith("Preparing food database")
            || m_statusLabel->text().startsWith("Food database package")
            || m_statusLabel->text().startsWith("Food database setup")) {
            m_statusLabel->setText("Food database ready. Search a food to log it.");
        }
        return;
    }

    if (!m_databaseSetupError.isEmpty()) {
        m_foodSearchEdit->setPlaceholderText("Food database unavailable");
        m_statusLabel->setText(m_databaseSetupError);
        return;
    }

    m_foodSearchEdit->setPlaceholderText("Preparing food database...");
    m_statusLabel->setText("Preparing food database in the background...");
}

double MainPage::nutrientAmount(const QVector<NutrientValue> &nutrients, const QString &key) const
{
    double total = 0.0;
    for (const NutrientValue &nutrient : nutrients) {
        if (nutrient.key == key) {
            total += nutrient.amount;
        }
    }

    return total;
}

double MainPage::displayAmountForKey(const QVector<NutrientValue> &nutrients, const QString &key) const
{
    if (key == "omega3") {
        return nutrientAmount(nutrients, "ala")
            + nutrientAmount(nutrients, "epa")
            + nutrientAmount(nutrients, "dha");
    }

    return nutrientAmount(nutrients, key);
}

QString MainPage::displayUnitForKey(
    const QVector<NutrientValue> &nutrients,
    const TrackedNutrientDefinition &definition) const
{
    for (const NutrientValue &nutrient : nutrients) {
        if (nutrient.key == definition.key && !nutrient.unit.trimmed().isEmpty()) {
            return nutrient.unit.toLower();
        }
    }

    return definition.defaultUnit;
}

double MainPage::totalFor(const QString &key) const
{
    double total = 0.0;
    for (const AnalyzedFoodItem &food : m_loggedFoods) {
        total += nutrientAmount(food.scaledNutrients, key);
    }
    return total;
}

QString MainPage::formatAmount(double amount) const
{
    if (amount >= 100.0) {
        return QString::number(amount, 'f', 0);
    }
    if (amount >= 10.0) {
        return QString::number(amount, 'f', 1);
    }
    return QString::number(amount, 'f', 2);
}

int MainPage::percentOf(double value, double target) const
{
    if (target <= 0.0) {
        return 0;
    }

    return qBound(0, static_cast<int>((value / target) * 100.0), 100);
}

QString MainPage::getCalorieProgressMessage(double totalCalories, double calorieGoal) const
{
    if (calorieGoal <= 0.0 || totalCalories <= 0.0 || totalCalories > calorieGoal) {
        return {};
    }

    const double progress = totalCalories / calorieGoal;
    if (progress >= 0.995) {
        return "You met your daily calorie goal.";
    }
    if (progress >= 0.8) {
        return "You are almost there.";
    }
    if (progress >= 0.5) {
        return "You are halfway there.";
    }

    return "Keep going.";
}

bool MainPage::isOverCalorieGoal(double totalCalories, double calorieGoal) const
{
    return calorieGoal > 0.0 && totalCalories > calorieGoal;
}

void MainPage::animateProgress(QProgressBar *bar, int value)
{
    auto *animation = new QPropertyAnimation(bar, "value", bar);
    animation->setDuration(320);
    animation->setStartValue(bar->value());
    animation->setEndValue(value);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainPage::applyShadow(QWidget *widget)
{
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(22);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 70));
    widget->setGraphicsEffect(shadow);
}
