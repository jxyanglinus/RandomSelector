#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>
#include <QApplication>
#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <map>
#include <vector>
#include <random>
#include <cmath>
#include <chrono>
#include <ctime>

// #define __DEBUG__

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addName();
    void removeName();
    void numberClicked();
    void backspaceNumber();
    void startSelection();

private:
    void loadHistory();
    void saveHistory();
    bool checkHistory();
    void resetHistory();
    std::vector<std::string> weightedSelection(int m);

    QListWidget *nameList;
    QListWidget *resultList;
    QLineEdit *nameInput;
    QLabel *numberDisplay;

    std::map<std::string, int> history;
    std::mt19937 rng;

    const QString HISTORY_FILE = QApplication::applicationDirPath() + "/history";
    const QString NAMES_FILE = QApplication::applicationDirPath() + "/names";

    const double ALPHA = 1.2;
    const double BETA = 0.5;
    const double MIN_WEIGHT = 0.05;
};

#endif // MAINWINDOW_H
