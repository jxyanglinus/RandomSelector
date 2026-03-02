#include "mainwindow.h"
#include <QFile>
#include <QMessageBox>
#include <fstream>
#include <sstream>
#include <QDebug>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), rng(std::chrono::system_clock::now().time_since_epoch().count() ^ std::random_device{}()) {
    // 初始化界面
    this->setFont(QFont("Microsoft Yahei", 14));
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // 导入主题
    QFile file(":/qss/style.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QString style = file.readAll();
        this->setStyleSheet(style);
        file.close();
    }

    QString bgStyle =
        "MainWindow {"
        "   background: url(:/images/bk) center/cover no-repeat fixed;"
        "}"
        "MainWindow > QWidget { background: transparent; }"; // 子控件透明

    this->setStyleSheet(this->styleSheet() + bgStyle);

    QPixmap bgPixmap(":/images/bk");
    if(bgPixmap.isNull()) {
        qWarning() << "Failed to load background image!";
    } else {
        // 考虑高DPI缩放
        qreal dpiRatio = devicePixelRatioF();
        QSize actualSize = bgPixmap.size() / dpiRatio;

        // 设置窗口最大尺寸
        setMaximumSize(actualSize);
    }

    // 左边面板
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    nameList = new QListWidget;
    nameInput = new QLineEdit;
    QPushButton *addButton = new QPushButton("添加");
    QPushButton *removeButton = new QPushButton("删除");

    leftLayout->addWidget(nameList);
    leftLayout->addWidget(nameInput);
    leftLayout->addWidget(addButton);
    leftLayout->addWidget(removeButton);

    // 中间面板
    QWidget *centerPanel = new QWidget;
    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);

    // 顶部垂直弹簧
    QSpacerItem *topSpacer = new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    centerLayout->addSpacerItem(topSpacer);

    // 数字显示器
    numberDisplay = new QLabel("0");
    numberDisplay->setAlignment(Qt::AlignCenter);
    numberDisplay->setStyleSheet("font-family: 'Microsoft Yahei'; "
                                 "font-size: 30px; "
                                 "font-weight: bold; "
                                 "border: 3px solid gray; "
                                 "color: #FF7F7F;"
                                 "padding: 5px; "
                                 "border-radius: 10px;");
    numberDisplay->setFixedHeight(60);
    centerLayout->addWidget(numberDisplay);

    // 数字键盘布局
    QGridLayout *numPadLayout = new QGridLayout;
    numPadLayout->setSpacing(5);
    numPadLayout->setContentsMargins(10, 10, 10, 10);

    auto createButton = [](const QString & text) {
        QPushButton* btn = new QPushButton(text);
        btn->setMinimumSize(60, 60);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        return btn;
    };

    // 数字1-9
    for(int i = 1; i <= 9; ++i) {
        QPushButton *btn = createButton(QString::number(i));
        connect(btn, &QPushButton::clicked, this, &MainWindow::numberClicked);
        numPadLayout->addWidget(btn, (i - 1) / 3, (i - 1) % 3);
    }

    // 底部按钮行
    QPushButton *zeroBtn = createButton("0");
    QPushButton *backspaceBtn = createButton("←");
    connect(zeroBtn, &QPushButton::clicked, this, &MainWindow::numberClicked);
    connect(backspaceBtn, &QPushButton::clicked, this, &MainWindow::backspaceNumber);

    numPadLayout->addWidget(zeroBtn, 3, 0, 1, 2);  // 占两列
    numPadLayout->addWidget(backspaceBtn, 3, 2);

    // 添加键盘布局
    centerLayout->addLayout(numPadLayout);

    // 底部垂直弹簧
    QSpacerItem *bottomSpacer = new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    centerLayout->addSpacerItem(bottomSpacer);

    // 设置布局比例
    centerLayout->setStretch(0, 1);  // 顶部弹簧
    centerLayout->setStretch(1, 0);  // 数字显示（固定高度）
    centerLayout->setStretch(2, 3);  // 数字键盘
    centerLayout->setStretch(3, 1);  // 底部弹簧

    // 右边面板
    QWidget *rightPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    resultList = new QListWidget;
    QPushButton *startButton = new QPushButton("开始抽取");

    rightLayout->addWidget(resultList);
    rightLayout->addWidget(startButton);

    // 组合主界面
    mainLayout->addWidget(leftPanel, 3);
    mainLayout->addWidget(centerPanel, 2);
    mainLayout->addWidget(rightPanel, 3);

    setCentralWidget(centralWidget);

    nameList->setMinimumWidth(200);   // 左侧列表最小宽度
    resultList->setMinimumWidth(200); // 右侧列表最小宽度
    centerPanel->setMinimumWidth(250);// 中间面板最小宽度

    // 假设要设置按钮70%透明度
    QString transparencyStyle =
        "QPushButton {"
        "   background-color: rgba(%1, %2, %3, %4) !important;"
        "}"
        "QListWidget {"
        "background-color: rgba(%1, %2, %3, %4) !important;"
        "}"
        "QLineEdit {"
        "background-color: rgba(%1, %2, %3, %4) !important;"
        "}";

    // 获取原有按钮背景色
    QColor originalColor = this->palette().button().color();

    // 生成新样式（保留原有RGB，仅修改Alpha）
    transparencyStyle = transparencyStyle
                        .arg(originalColor.red())
                        .arg(originalColor.green())
                        .arg(originalColor.blue())
                        .arg(qBound(0.0, 70 / 100.0, 1.0));  // 70%透明度

    // 追加到现有样式表
    this->setStyleSheet(this->styleSheet() + transparencyStyle);

    // 连接信号槽
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addName);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeName);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startSelection);

    // 随机种子
    std::srand(time(NULL));

    // 加载数据
    QFile nameFile(NAMES_FILE);
    if(nameFile.open(QIODevice::ReadOnly)) {
        while(!nameFile.atEnd()) {
            QString name = QString::fromUtf8(nameFile.readLine()).trimmed();
            if(!name.isEmpty()) {
                nameList->addItem(name);
            }
        }
    }
    loadHistory();
}

void MainWindow::addName() {
    QString name = nameInput->text().trimmed();
    if(!name.isEmpty()) {
        nameList->addItem(name);
        nameInput->clear();

        // 保存到文件
        QFile file(NAMES_FILE);
        if(file.open(QIODevice::Append)) {
            file.write(name.toUtf8() + "\n");
        }
    }
}

void MainWindow::removeName() {
    QList<QListWidgetItem*> items = nameList->selectedItems();
    if(items.isEmpty()) return;

    // 删除界面元素
    foreach(QListWidgetItem *item, items) {
        delete item;
    }

    // 更新数据文件
    QFile file(NAMES_FILE);
    if(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        for(int i = 0; i < nameList->count(); ++i) {
            QString name = nameList->item(i)->text();
            file.write(name.toUtf8() + "\n");
        }
        file.close();
    }
}

void MainWindow::numberClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    QString num = btn->text();
    QString current = numberDisplay->text();

    int maxNum = nameList->count();
    QString newNum = (current == "0") ? num : current + num;

    if(newNum.toInt() <= maxNum) {
        numberDisplay->setText(newNum);
    }
}

void MainWindow::backspaceNumber() {
    QString current = numberDisplay->text();

    if(current == "0" || current.isEmpty()) return;

    if(current.length() > 1) {
        QString newNum = current.left(current.length() - 1);
        // 自动去除前导零
        if(newNum.startsWith("0") && newNum.length() > 1) {
            newNum = newNum.mid(1);
        }
        numberDisplay->setText(newNum);
    } else {
        numberDisplay->setText("0");
    }
}

void MainWindow::startSelection() {
    int m = numberDisplay->text().toInt();
    int total = nameList->count();

    if(m <= 0 || m > total) {
        QMessageBox::warning(this, "错误", "<font color=white>请输入有效的抽取人数！</font>");
        return;
    }

    // 执行抽选
    std::vector<std::string> selected = weightedSelection(m);

    // 显示结果
    resultList->clear();
    for(const auto& name : selected) {
        resultList->addItem(QString::fromStdString(name));
    }

    // 检查当前历史记录，如果过多的人（40%）达到最低权重，就重置历史记录
    if (checkHistory()) {
        resetHistory();
    }

    // 保存历史记录
    saveHistory();
}

std::vector<std::string> MainWindow::weightedSelection(int m) {
#ifdef __DEBUG__
    qDebug() << "In MainWindow::weighedSelection() now:";
#endif
    std::vector<std::string> names;
    for(int i = 0; i < nameList->count(); ++i) {
        names.push_back(nameList->item(i)->text().toStdString());
    }

    std::vector<double> weights;
    for(const auto& name : names) {
        double raw_weight = 1.0 / (1 + ALPHA * history[name] + BETA * std::sqrt(history[name]));
        weights.push_back(std::max(raw_weight, MIN_WEIGHT));
    }

    std::vector<std::string> result;
    std::vector<bool> selected(names.size(), false);

    for(int i = 0; i < m; ++i) {
        double total_weight = 0;
        for(int j = 0; j < names.size(); ++j) {
            if(!selected[j]) total_weight += weights[j];
        }

        if(total_weight <= 0) break;

        std::uniform_real_distribution<double> dist(0, total_weight);
        double r = dist(rng);

        double accum = 0;
        for(int j = 0; j < names.size(); ++j) {
            if(!selected[j]) {
                accum += weights[j];
                if(accum >= r) {
                    selected[j] = true;
                    result.push_back(names[j]);
                    history[names[j]]++;
                    break;
                }
            }
        }
    }

#ifdef __DEBUG__
    for(const auto& name : names) {
        double raw_weight = 1.0 / (1 + ALPHA * history[name] + BETA * std::sqrt(history[name]));
        qDebug() << "Name:" << QString::fromStdString(name) << "Raw weight:" << raw_weight;
    }
#endif

    return result;
}

void MainWindow::loadHistory() {
    std::ifstream file(HISTORY_FILE.toStdString());
    if(file) {
        std::string line;
        while(std::getline(file, line)) {
            size_t pos = line.find(',');
            if(pos != std::string::npos) {
                std::string name = line.substr(0, pos);
                int count = std::stoi(line.substr(pos + 1));
                history[name] = count;
            }
        }
    }
}

void MainWindow::saveHistory() {
    std::ofstream file(HISTORY_FILE.toStdString());
    for(int i = 0; i < nameList->count(); ++i) {
        std::string name = nameList->item(i)->text().toStdString();
        file << name << "," << history[name] << "\n";
    }
}

bool MainWindow::checkHistory() {
#ifdef __DEBUG__
    qDebug() << "In MainWindow::checkHistory() now:";
#endif
    int minCount = 0;
    for (const auto &item : history) {
        double raw_weight = 1.0 / (1 + ALPHA * (double)item.second + BETA * std::sqrt(item.second));
#ifdef __DEBUG__
        qDebug() << "Name:" << QString::fromStdString(item.first) << "Raw weight:" << raw_weight;
#endif
        if (raw_weight < MIN_WEIGHT) {
            minCount++;
        }
    }
    return minCount > history.size() * 0.4;
}

void MainWindow::resetHistory() {
    for (auto &item : history) {
        item.second = 0;
    }
}

MainWindow::~MainWindow() {
    saveHistory();
}
