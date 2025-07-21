// main.cpp
#include <QApplication>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimeEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QComboBox>
#include <QMessageBox>
#include <QMediaPlayer>
#include <QFileDialog>
#include <QTimer>
#include <QLabel>
#include <qcalendarwidget.h>
#include <QCloseEvent>

class GardenDemo : public QMainWindow {
    Q_OBJECT

public:
    GardenDemo() {
        setupDB();
        setupUI();
        setupTray();
        loadTents();
        checkSchedules();
    }

private:
    QSystemTrayIcon* trayIcon;
    QListWidget* tentList;
    QListWidget* plantList;
    QLineEdit* tentNameEdit;
    QLineEdit* plantNameEdit;
    QComboBox* tentSelector;
    QTimeEdit* feed1Time, *feed2Time;
    QCheckBox* feed2xCheck;
    QCheckBox* waterDays[7], *feedDays[7];
    QPushButton* soundSelectBtn;
    QString soundPath;
    QMediaPlayer* player = new QMediaPlayer;
    QTimer* timer;
    QCalendarWidget* calendar;
    QLineEdit* flowerInput;
    QPushButton* saveFlowerBtn;

    void setupDB() {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(QApplication::applicationDirPath() + "/garden_demo.db");
        if (!db.open()) {
            qDebug() << "DB open error:" << db.lastError();
        }
        QSqlQuery q;
        q.exec("CREATE TABLE IF NOT EXISTS tents (id INTEGER PRIMARY KEY, name TEXT, feed2x INT, feed1 TEXT, feed2 TEXT, water_days TEXT, feed_days TEXT, sound TEXT)");
        q.exec("CREATE TABLE IF NOT EXISTS plants (id INTEGER PRIMARY KEY, name TEXT, tent_id INT, flower_time_days INT,flower_start_date TEXT,start_date TEXT)");
    }

    void reassignPlantToTent() {
        QString plantName = plantNameEdit->text().trimmed();
        if (plantName.isEmpty()) return;

        QVariant tentData = tentSelector->currentData();
        if (!tentData.isValid()) return;

        int tentId = tentData.toInt();

        QSqlQuery q;
        q.prepare("UPDATE plants SET tent_id = ? WHERE name = ?");
        q.addBindValue(tentId);
        q.addBindValue(plantName);
        if (!q.exec()) {
            qDebug() << "Failed to reassign tent:" << q.lastError().text();
        }

        loadPlants(); // refresh UI with new assignment
    }

    QMap<int, QColor> tentColorMap = {
        {1, Qt::red}, {2, Qt::green}, {3, Qt::blue}, {4, Qt::yellow}
    };

    void checkHarvestAlerts() {
        QDate today = QDate::currentDate();
        QSqlQuery q("SELECT name, flower_start_date, flower_time_days FROM plants");
        while (q.next()) {
            QString name = q.value(0).toString();
            QDate start = QDate::fromString(q.value(1).toString(), "yyyy-MM-dd");
            int flowerDays = q.value(2).toInt();
            QDate harvest = start.addDays(flowerDays);
            if (harvest == today) {
                qApp->setQuitOnLastWindowClosed(false);
                QMessageBox::information(this, "Harvest Alert", name + " is ready to harvest today!");
                // You can also play a sound here
                 qApp->setQuitOnLastWindowClosed(true);
            }
        }
    }

    void showPlantCalendarTimeline(QListWidgetItem* item) {
        QString plantName = item->data(Qt::UserRole).toString();
        QSqlQuery q;
        q.prepare("SELECT start_date, flower_time_days, tent_id FROM plants WHERE name=?");
        q.addBindValue(plantName);
        if (q.exec() && q.next()) {
            QDate start = QDate::fromString(q.value(0).toString(), "yyyy-MM-dd");
            int flowerDays = q.value(1).toInt();
            int tentId = q.value(2).toInt();

            // Clear all calendar marks first
            calendar->setDateTextFormat(QDate(), QTextCharFormat());

            // Draw just this plant’s dates
            QColor color = tentColorMap.value(tentId, Qt::magenta);
            QTextCharFormat format;
            format.setBackground(color);

            for (int i = 0; i < flowerDays; ++i) {
                calendar->setDateTextFormat(start.addDays(i), format);
            }
        }
    }

    void markPlantDatesOnCalendar() {
        if (!plantNameEdit) return;

        QString plantName = plantNameEdit->text().trimmed();
        if (plantName.isEmpty()) return;

        // Clear all previous formatting
        calendar->setDateTextFormat(QDate(), QTextCharFormat());

        QSqlQuery q;
        q.prepare("SELECT start_date, flower_time_days, flower_start_date FROM plants WHERE name = ?");
        q.addBindValue(plantName);

        if (!q.exec()) {
            qDebug() << "SQL error:" << q.lastError().text();
            return;
        }

        if (q.next()) {
            qDebug() << "test";
            QDate startDate = QDate::fromString(q.value(0).toString(), "yyyy-MM-dd");
            QDate flowerstartDate = QDate::fromString(q.value(2).toString(), "yyyy-MM-dd");
            int flowerDays = q.value(1).toInt();
             qDebug() << flowerDays;
              qDebug() <<  QDate::fromString(q.value(0).toString(), "yyyy-MM-dd");
 qDebug() <<  startDate.isValid();
            if (!startDate.isValid() && flowerDays >= 0) return;
  qDebug() << "valid";
            // Estimate veg duration (optional)
            int vegDays = 14; // you can adjust this or store it too

            // Use different colors for veg and flower
            QTextCharFormat vegFormat;
            vegFormat.setBackground(Qt::blue);
            vegFormat.setToolTip("Veg phase");

            QTextCharFormat flowerFormat;
            flowerFormat.setBackground(Qt::green);
            flowerFormat.setToolTip("Flowering phase");

            // Apply veg days
            QDate d = startDate;
            for (int i = 0; i < vegDays; ++i) {
                calendar->setDateTextFormat(d, vegFormat);
                d = d.addDays(1);
            }
           if (flowerstartDate.isValid())
                d = flowerstartDate;

            // Apply flowering days
            for (int i = 0; i < flowerDays; ++i) {
                calendar->setDateTextFormat(d, flowerFormat);
                d = d.addDays(1);
            }

            //calendar->setDateTextFormat(flowerstartDate, vegFormat);
        }
          qDebug() << "update";
        calendar->update();  // force redraw
    }

    void markPlantDatesOnCalendar2() {
        calendar->setDateTextFormat(QDate(), QTextCharFormat()); // Clear all highlights

        QSqlQuery q("SELECT name, start_date, flower_time_days, tent_id FROM plants");
        while (q.next()) {
            QString name = q.value(0).toString();
            QDate start = QDate::fromString(q.value(1).toString(), "yyyy-MM-dd");
            int flowerDays = q.value(2).toInt();
            int tentId = q.value(3).toInt();

            QDate vegEnd = start.addDays(20);
            QDate harvest = start.addDays(flowerDays);

            QColor tentColor = tentColorMap.value(tentId, Qt::gray);

            // Veg phase
            QTextCharFormat vegFormat;
            vegFormat.setBackground(tentColor.lighter(180));
            for (QDate d = start; d <= vegEnd; d = d.addDays(1)) {
                calendar->setDateTextFormat(d, vegFormat);
            }

            // Flower phase
            QTextCharFormat flowerFormat;
            flowerFormat.setBackground(tentColor);
            for (QDate d = vegEnd.addDays(1); d <= harvest; d = d.addDays(1)) {
                calendar->setDateTextFormat(d, flowerFormat);
            }

            // Optional: bold harvest day
            QTextCharFormat harvestFormat;
            harvestFormat.setBackground(Qt::black);
            harvestFormat.setForeground(Qt::white);
            calendar->setDateTextFormat(harvest, harvestFormat);
        }
        calendar->update();  // force refresh

    }

    void loadPlantToEditor(QListWidgetItem* item) {
        QString plantName = item->data(Qt::UserRole).toString();
        plantNameEdit->setText(plantName);

        QSqlQuery q;
        q.prepare("SELECT tent_id, flower_time_days FROM plants WHERE name=?");
        q.addBindValue(plantName);
        if (q.exec() && q.next()) {
            int tentId = q.value(0).toInt();
            int flowerTime = q.value(1).toInt();

            int index = tentSelector->findData(tentId);
            if (index != -1) {
                tentSelector->setCurrentIndex(index);
            }

            flowerInput->setText(QString::number(flowerTime));
        }
        markPlantDatesOnCalendar();
    }

    void saveFlowerTime() {
        if (!plantNameEdit || !flowerInput || !calendar) {
            qDebug() << "UI elements not initialized!";
            return;
        }

        QString plantName = plantNameEdit->text().trimmed();
        if (plantName.isEmpty()) {
            QMessageBox::warning(this, "Error", "No plant selected!");
            return;
        }

        bool ok = false;
        int flowerTime = flowerInput->text().toInt(&ok);
        if (!ok || flowerTime < 30 || flowerTime > 120) {
            QMessageBox::warning(this, "Invalid Input", "Please enter a valid flowering time between 30 and 120 days.");
            return;
        }

        QDate selectedDate = calendar->selectedDate();
        QString flowerStartDateStr = selectedDate.toString("yyyy-MM-dd");

        QSqlQuery q;
        q.prepare("UPDATE plants SET flower_time_days = ?, flower_start_date = ? WHERE name = ?");
        q.addBindValue(flowerTime);
        q.addBindValue(flowerStartDateStr);
        q.addBindValue(plantName);

        if (!q.exec()) {
            qDebug() << "SQL error:" << q.lastError().text();
            QMessageBox::critical(this, "Database Error", "Failed to save flowering time.");
            return;
        }

        QMessageBox::information(this, "Saved", "Flowering time and start date saved!");
        markPlantDatesOnCalendar();  // Refresh view
    }


    void setupUI() {
        QWidget* central = new QWidget;
        QHBoxLayout* mainLayout = new QHBoxLayout;

        // Tent side
        QVBoxLayout* tentLayout = new QVBoxLayout;
        tentList = new QListWidget;
        tentNameEdit = new QLineEdit;
        QPushButton* addTent = new QPushButton("Add Tent");
        QPushButton* removeTent = new QPushButton("Remove Tent");
        tentLayout->addWidget(new QLabel("Tents"));
        tentLayout->addWidget(tentList);
        tentLayout->addWidget(tentNameEdit);
        tentLayout->addWidget(addTent);
        tentLayout->addWidget(removeTent);

        // Tent config
        QGroupBox* configBox = new QGroupBox("Tent Schedule");
        QVBoxLayout* configLayout = new QVBoxLayout;

        feed2xCheck = new QCheckBox("Feed 2x a day");
        feed1Time = new QTimeEdit(QTime(9,0));
        feed2Time = new QTimeEdit(QTime(18,0));
        soundSelectBtn = new QPushButton("Choose Sound");
        configLayout->addWidget(feed2xCheck);
        configLayout->addWidget(new QLabel("Feed Time 1"));
        configLayout->addWidget(feed1Time);
        configLayout->addWidget(new QLabel("Feed Time 2"));
        configLayout->addWidget(feed2Time);
        configLayout->addWidget(soundSelectBtn);

        QHBoxLayout* waterLayout = new QHBoxLayout;
        QHBoxLayout* feedLayout = new QHBoxLayout;
        QStringList days = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        for (int i = 0; i < 7; ++i) {
            waterDays[i] = new QCheckBox(days[i]);
            feedDays[i] = new QCheckBox(days[i]);
            waterLayout->addWidget(waterDays[i]);
            feedLayout->addWidget(feedDays[i]);
        }
        configLayout->addWidget(new QLabel("Water Days"));
        configLayout->addLayout(waterLayout);
        configLayout->addWidget(new QLabel("Feed Days"));
        configLayout->addLayout(feedLayout);

        QPushButton* saveConfig = new QPushButton("Save Tent Config");
        configLayout->addWidget(saveConfig);
        configBox->setLayout(configLayout);
        tentLayout->addWidget(configBox);

        mainLayout->addLayout(tentLayout);


        // Plant side
        QVBoxLayout* plantLayout = new QVBoxLayout;
        plantList = new QListWidget;
        plantNameEdit = new QLineEdit;
        tentSelector = new QComboBox;
        QPushButton* addPlant = new QPushButton("Add Plant");
        QPushButton* removePlant = new QPushButton("Remove Plant");
        plantLayout->addWidget(new QLabel("Plants"));
        plantLayout->addWidget(plantList);
        plantLayout->addWidget(new QLabel("Plant Name"));
        plantLayout->addWidget(plantNameEdit);
        plantLayout->addWidget(new QLabel("Assign to Tent"));
        plantLayout->addWidget(tentSelector);
        plantLayout->addWidget(addPlant);
        plantLayout->addWidget(removePlant);
        mainLayout->addLayout(plantLayout);
        calendar = new QCalendarWidget;
        calendar->setGridVisible(true);
        mainLayout->addWidget(calendar);

        // Flowering time input
        QLabel* flowerLabel = new QLabel("Flowering Time (days):");
        flowerInput = new QLineEdit;
        flowerInput->setValidator(new QIntValidator(30, 120, this)); // reasonable range

        // Save button
        QPushButton* saveFlowerBtn = new QPushButton("Save Flower Time");

        // Add to layout
        plantLayout->addWidget(flowerLabel);
        plantLayout->addWidget(flowerInput);
        plantLayout->addWidget(saveFlowerBtn);

        connect(plantList, &QListWidget::itemClicked, this, [=](QListWidgetItem* item) {
            plantNameEdit->setText(item->text());
            markPlantDatesOnCalendar();
        });

        connect(saveFlowerBtn, &QPushButton::clicked, this, &GardenDemo::saveFlowerTime);

       // connect(plantList, &QListWidget::itemClicked, this, &GardenDemo::showPlantCalendarTimeline);

        connect(tentSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &GardenDemo::reassignPlantToTent);

        connect(plantList, &QListWidget::itemClicked, this, &GardenDemo::loadPlantToEditor);
        connect(addTent, &QPushButton::clicked, this, &GardenDemo::addTent);
        connect(removeTent, &QPushButton::clicked, this, &GardenDemo::removeTent);
        connect(addPlant, &QPushButton::clicked, this, &GardenDemo::addPlant);
        connect(removePlant, &QPushButton::clicked, this, &GardenDemo::removePlant);
        connect(tentList, &QListWidget::itemClicked, this, &GardenDemo::loadTentConfig);
        connect(saveConfig, &QPushButton::clicked, this, &GardenDemo::saveTentConfig);
        connect(soundSelectBtn, &QPushButton::clicked, this, [&]() {
            soundPath = QFileDialog::getOpenFileName(this, "Select sound");
        });

        central->setLayout(mainLayout);
        setCentralWidget(central);
        resize(800, 400);
    }
    void showWindow() {
        this->show();
    }
    void setupTray() {
        trayIcon = new QSystemTrayIcon(QIcon::fromTheme("media-playback-start"), this);
        trayIcon = new QSystemTrayIcon(QIcon(QApplication::applicationDirPath() + "/mac128.png"), this);
        QMenu* menu = new QMenu;
        QAction* quitAction = menu->addAction("Quit");
                menu->addAction("Show Window", this, &GardenDemo::showWindow);
        connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
        trayIcon->setContextMenu(menu);
        trayIcon->show();
        trayIcon->setVisible(true);
    }

    void loadTents() {
        tentList->clear();
        tentSelector->clear();
        QSqlQuery q("SELECT id, name FROM tents");
        while (q.next()) {
            int id = q.value(0).toInt();
            QString name = q.value(1).toString();
            QListWidgetItem* item = new QListWidgetItem(name);
            item->setData(Qt::UserRole, id);
            tentList->addItem(item);
            tentSelector->addItem(name, id);
        }
        loadPlants();
    }

    void loadPlants() {
        plantList->clear();
        QSqlQuery q("SELECT p.name, t.name FROM plants p LEFT JOIN tents t ON p.tent_id = t.id");
        while (q.next()) {
            QString plantName = q.value(0).toString();
            QString tentName = q.value(1).toString();
            QString displayName = plantName + " [" + (tentName.isEmpty() ? "No Tent" : tentName) + "]";
            QListWidgetItem* item = new QListWidgetItem(displayName);
            item->setData(Qt::UserRole, plantName); // keep actual name for logic
            plantList->addItem(item);
        }
    }


    void addTent() {
        QString name = tentNameEdit->text();
        if (name.isEmpty()) return;
        QSqlQuery q;
        q.prepare("INSERT INTO tents (name, feed2x, feed1, feed2, water_days, feed_days, sound) VALUES (?, 0, '09:00', '18:00', '0000000', '0000000', '')");
        q.addBindValue(name);
        q.exec();
        loadTents();
    }

    void removeTent() {
        auto item = tentList->currentItem();
        if (!item) return;
        int id = item->data(Qt::UserRole).toInt();
        QSqlQuery q;
        q.prepare("DELETE FROM tents WHERE id=?");
        q.addBindValue(id);
        q.exec();
        loadTents();
    }

    void loadTentConfig() {
        auto item = tentList->currentItem();
        if (!item) return;
        int id = item->data(Qt::UserRole).toInt();
        QSqlQuery q;
        q.prepare("SELECT * FROM tents WHERE id=?");
        q.addBindValue(id);
        q.exec();
        if (q.next()) {
            feed2xCheck->setChecked(q.value("feed2x").toInt());
            feed1Time->setTime(QTime::fromString(q.value("feed1").toString()));
            feed2Time->setTime(QTime::fromString(q.value("feed2").toString()));
            soundPath = q.value("sound").toString();
            QString wdays = q.value("water_days").toString();
            QString fdays = q.value("feed_days").toString();
            for (int i = 0; i < 7; ++i) {
                waterDays[i]->setChecked(wdays[i] == '1');
                feedDays[i]->setChecked(fdays[i] == '1');
            }
        }
    }

    void saveTentConfig() {
        auto item = tentList->currentItem();
        if (!item) return;
        int id = item->data(Qt::UserRole).toInt();
        QString wdayStr, fdayStr;
        for (int i = 0; i < 7; ++i) {
            wdayStr += waterDays[i]->isChecked() ? '1' : '0';
            fdayStr += feedDays[i]->isChecked() ? '1' : '0';
        }
        QSqlQuery q;
        q.prepare("UPDATE tents SET feed2x=?, feed1=?, feed2=?, water_days=?, feed_days=?, sound=? WHERE id=?");
        q.addBindValue(feed2xCheck->isChecked() ? 1 : 0);
        q.addBindValue(feed1Time->time().toString("HH:mm"));
        q.addBindValue(feed2Time->time().toString("HH:mm"));
        q.addBindValue(wdayStr);
        q.addBindValue(fdayStr);
        q.addBindValue(soundPath);
        q.addBindValue(id);
        q.exec();
    }

    void addPlant() {
        QString name = plantNameEdit->text();
        if (name.isEmpty()) return;
        int tentId = tentSelector->currentData().toInt();

        QDate startDate = QDate::currentDate();
        QString dateStr = startDate.toString("yyyy-MM-dd");

        QSqlQuery q;
        q.prepare("INSERT INTO plants (name, tent_id, start_date, flower_time_days) VALUES (?, ?, ?,?)");
        q.addBindValue(name);
        q.addBindValue(tentId);
        q.addBindValue(dateStr);  // properly formatted
        q.addBindValue(60);
        q.exec();

        loadPlants();
    }

    void removePlant() {
        QString item = plantNameEdit->text();
        //if (item = "") return;
        QSqlQuery q;
        q.prepare("DELETE FROM plants WHERE name=?");
        q.addBindValue(item);
        q.exec();
        loadPlants();
    }

    void checkSchedules() {
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [&]() {
            QTime now = QTime::currentTime();
            int dow = QDate::currentDate().dayOfWeek() % 7; // 0=Sun
            QSqlQuery q("SELECT * FROM tents");
            while (q.next()) {
                QString wdays = q.value("water_days").toString();
                QString fdays = q.value("feed_days").toString();
                bool feed2x = q.value("feed2x").toInt();
                QString feed1 = q.value("feed1").toString();
                QString feed2 = q.value("feed2").toString();
                QString sound = q.value("sound").toString();
                QString tentName = q.value("name").toString();

                auto matchTime = [&](const QString& t) {
                    return now.toString("HH:mm") == t;
                };

                if (wdays[dow] == '1' && matchTime(feed1)) {
                    showAlert("Water", tentName, sound);
                }
                if (fdays[dow] == '1' && matchTime(feed1)) {
                    showAlert("Feed", tentName, sound);
                }
                if (feed2x && fdays[dow] == '1' || wdays[dow] == '1' && matchTime(feed2)) {
                    showAlert("Feed (2x)", tentName, sound);
                }
            }
        });
        timer->start(60000); // every 1 min
    }

    void showAlert(const QString& action, const QString& tentName, const QString& soundFile) {
        qApp->setQuitOnLastWindowClosed(false);
        QMessageBox::information(this, "Garden Alert", QString("%1 Tent: %2").arg(action).arg(tentName));
        if (!soundFile.isEmpty()) {
            player->setMedia(QUrl::fromLocalFile(soundFile));
            player->play();
        }
         qApp->setQuitOnLastWindowClosed(true);
    }
protected:
    void closeEvent(QCloseEvent *event) override {
        if (trayIcon->isVisible()) {
            hide();
            event->ignore();
        } else {
            event->accept();
        }
    }
};



int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    GardenDemo g;
    g.show();
    return app.exec();
}
#include "main.moc"
