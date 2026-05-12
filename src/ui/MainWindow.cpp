#include "MainWindow.h"

#include "MainPage.h"
#include "WelcomePage.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Nutrient Tracker");
    resize(1180, 760);
    setMinimumSize(980, 680);

    m_rootPages = new QStackedWidget(this);
    m_welcomePage = new WelcomePage(m_rootPages);
    m_rootPages->addWidget(m_welcomePage);
    m_rootPages->addWidget(createApplicationShell());
    setCentralWidget(m_rootPages);

    connect(m_welcomePage, &WelcomePage::startRequested, this, [this]() {
        showApplication();
    });

    m_rootPages->setCurrentWidget(m_welcomePage);
}

QWidget *MainWindow::createApplicationShell()
{
    QWidget *central = new QWidget(this);
    central->setObjectName("appRoot");

    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *sidebar = new QWidget(central);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(230);

    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(18, 24, 18, 24);
    sidebarLayout->setSpacing(10);

    auto *appTitle = new QLabel("Nutrient\nTracker", sidebar);
    appTitle->setObjectName("appTitle");
    sidebarLayout->addWidget(appTitle);
    sidebarLayout->addSpacing(22);

    sidebarLayout->addWidget(createNavButton(
        "Dashboard",
        style()->standardIcon(QStyle::SP_ComputerIcon),
        DashboardIndex));
    sidebarLayout->addWidget(createNavButton(
        "Food Search",
        style()->standardIcon(QStyle::SP_FileDialogContentsView),
        FoodSearchIndex));
    sidebarLayout->addWidget(createNavButton(
        "Meals",
        style()->standardIcon(QStyle::SP_DirHomeIcon),
        MealsIndex));
    sidebarLayout->addWidget(createNavButton(
        "Daily Log",
        style()->standardIcon(QStyle::SP_FileDialogDetailedView),
        DailyLogIndex));
    sidebarLayout->addWidget(createNavButton(
        "Settings",
        style()->standardIcon(QStyle::SP_FileDialogInfoView),
        SettingsIndex));
    sidebarLayout->addStretch();

    m_pages = new QStackedWidget(central);
    m_mainPage = new MainPage(m_pages);
    m_pages->addWidget(m_mainPage);
    m_pages->addWidget(createPlaceholderPage(
        "Food Search",
        "Search foods, compare matches, and add items to your log."));
    m_pages->addWidget(createPlaceholderPage(
        "Meals",
        "Saved meals and reusable recipes will appear here."));
    m_pages->addWidget(createPlaceholderPage(
        "Daily Log",
        "A full-day history view can build on the dashboard log."));
    m_pages->addWidget(createPlaceholderPage(
        "Settings",
        "Daily targets, theme, and database preferences can live here."));

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(m_pages, 1);

    setStyleSheet(
        "#appRoot { background: #111827; }"
        "#sidebar { background: #0B1220; }"
        "#appTitle { color: #F9FAFB; font-family: 'Segoe UI', 'Helvetica Neue', sans-serif; "
        "font-size: 26px; font-weight: 800; }"
        "QPushButton#navButton { color: #A7B0C0; background: transparent; border: none; "
        "border-left: 4px solid transparent; border-radius: 8px; padding: 11px 12px; "
        "text-align: left; font-size: 14px; font-weight: 600; }"
        "QPushButton#navButton:hover { background: #172033; color: #F9FAFB; }"
        "QPushButton#navButton:checked { color: #FFFFFF; background: #172033; "
        "border-left: 4px solid #3CB371; }"
        "QWidget#placeholderPage { background: #111827; }"
        "QLabel#placeholderTitle { color: #F9FAFB; font-size: 28px; font-weight: 800; }"
        "QLabel#placeholderText { color: #9CA3AF; font-size: 15px; }");

    showPage(DashboardIndex);
    return central;
}

QPushButton *MainWindow::createNavButton(const QString &text, const QIcon &icon, int pageIndex)
{
    auto *button = new QPushButton(icon, text, this);
    button->setObjectName("navButton");
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setIconSize(QSize(18, 18));
    button->setMinimumHeight(46);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(button, &QPushButton::clicked, this, [this, pageIndex]() {
        showPage(pageIndex);
    });

    m_navButtons.append(button);
    return button;
}

QWidget *MainWindow::createPlaceholderPage(const QString &title, const QString &description)
{
    auto *page = new QWidget(this);
    page->setObjectName("placeholderPage");

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 36, 40, 36);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel(title, page);
    titleLabel->setObjectName("placeholderTitle");

    auto *descriptionLabel = new QLabel(description, page);
    descriptionLabel->setObjectName("placeholderText");
    descriptionLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addStretch();

    return page;
}

void MainWindow::showPage(int pageIndex)
{
    m_pages->setCurrentIndex(pageIndex);
    updateActiveNavigation(pageIndex);
}

void MainWindow::showApplication()
{
    m_rootPages->setCurrentIndex(1);
    showPage(DashboardIndex);
}

void MainWindow::updateActiveNavigation(int pageIndex)
{
    for (int index = 0; index < m_navButtons.size(); ++index) {
        m_navButtons.at(index)->setChecked(index == pageIndex);
    }
}
