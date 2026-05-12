#pragma once

#include <QList>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>

class MainPage;
class WelcomePage;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    enum PageIndex {
        DashboardIndex = 0,
        FoodSearchIndex = 1,
        MealsIndex = 2,
        DailyLogIndex = 3,
        SettingsIndex = 4
    };

    QPushButton *createNavButton(const QString &text, const QIcon &icon, int pageIndex);
    QWidget *createApplicationShell();
    QWidget *createPlaceholderPage(const QString &title, const QString &description);
    void showPage(int pageIndex);
    void showApplication();
    void updateActiveNavigation(int pageIndex);

    QStackedWidget *m_rootPages = nullptr;
    QStackedWidget *m_pages = nullptr;
    WelcomePage *m_welcomePage = nullptr;
    MainPage *m_mainPage = nullptr;
    QList<QPushButton *> m_navButtons;
};
