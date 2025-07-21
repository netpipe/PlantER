#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCalendarWidget>
#include <QDate>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QMap>
#include <QDialog>
#include <QVBoxLayout>
#include <QAction>
#include <QSettings>
#include <QInputDialog>
#include <QTimer>
#include <QComboBox>
#include <qpushbutton.h>

#include "sunset.h"   // Your SunSet class
// Assume getTimeFromSunValue(double) is available to convert minutes to hh:mm string

// Simple time formatter (replace with your getTimeFromSunValue if preferred)
QString formatTime(double minutes) {
    int h = int(minutes) / 60;
    int m = int(minutes) % 60;
    return QString("%1:%2").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0'));
}

struct City {
    QString name;
    double lat;
    double lon;
    int tzOffset;
};

QMap<QString, QMap<QString, City>> loadCities() {
    QMap<QString, QMap<QString, City>> map;
        map["CANADA"]["Edmonton"] = { "New York", 53.5344, 53.5344, 7 };
  //  map["USA"]["New York"] = { "New York", 40.7128, -74.0060, -5 };
   // map["USA"]["Los Angeles"] = { "Los Angeles", 34.0522, -118.2437, -8 };
   // map["UK"]["London"] = { "London", 51.5074, -0.1278, 0 };
   // map["Japan"]["Tokyo"] = { "Tokyo", 35.6895, 139.6917, 9 };
    return map;
}

struct CitySelection {
    QString country;
    QString cityName;
    City city;
};

CitySelection loadSelectedCity(QMap<QString, QMap<QString, City>>& cities) {
    QSettings settings("FloweringTracker", "CalendarApp");
    QString country = settings.value("country", "USA").toString();
    QString cityName = settings.value("city", "New York").toString();
    City city = cities[country].value(cityName, cities.first().first());
    return { country, cityName, city };
}

void saveSelectedCity(const QString& country, const QString& cityName) {
    QSettings settings("FloweringTracker", "CalendarApp");
    settings.setValue("country", country);
    settings.setValue("city", cityName);
}

class FloweringDelegate : public QStyledItemDelegate {
    City city;
public:
    FloweringDelegate(City city, QObject* parent = nullptr) : QStyledItemDelegate(parent), city(city) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QDate date = index.data(Qt::DisplayRole).toDate();
        if (!date.isValid()) return QStyledItemDelegate::paint(painter, option, index);

        SunSet sun;
        sun.setPosition(city.lat, city.lon, city.tzOffset);
        sun.setCurrentDate(date.year(), date.month(), date.day());
        double sr = sun.calcSunrise();
        double ss = sun.calcSunset();
        double daylight = (ss - sr) / 60.0; // in hours
        double diff = fabs(12.0 - daylight);

        QColor color = Qt::white;
        if (diff < 0.5) color = QColor("#66ff66");        // near 12/12: green
        else if (diff < 1.5) color = QColor("#ffff66");   // close: yellow
        else color = QColor("#ff6666");                   // far: red

        painter->fillRect(option.rect, color);
        QStyledItemDelegate::paint(painter, option, index);
    }
};
class CalendarDialog : public QDialog {
public:
    CalendarDialog(City city) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        QCalendarWidget* calendar = new QCalendarWidget(this);
        layout->addWidget(calendar);
        setLayout(layout);
        setWindowTitle(QString("Flowering Calendar - %1").arg(city.name));
        resize(400, 300);

        // Apply date coloring manually
        QTextCharFormat green, yellow, red;
        green.setBackground(QBrush(QColor("#66ff66")));
        yellow.setBackground(QBrush(QColor("#ffff66")));
        red.setBackground(QBrush(QColor("#ff6666")));

        QDate first = calendar->selectedDate().addDays(-calendar->selectedDate().day() + 1);
        QDate last = first.addMonths(1).addDays(-1);

        for (QDate date = first; date <= last; date = date.addDays(1)) {
            SunSet sun;
            sun.setPosition(city.lat, city.lon, city.tzOffset);
            sun.setCurrentDate(date.year(), date.month(), date.day());
            double sr = sun.calcSunrise();
            double ss = sun.calcSunset();
            double daylight = (ss - sr) / 60.0;
            double diff = fabs(12.0 - daylight);

            if (diff < 0.5)
                calendar->setDateTextFormat(date, green);
            else if (diff < 1.5)
                calendar->setDateTextFormat(date, yellow);
            else
                calendar->setDateTextFormat(date, red);
        }
    }
};


QString currentSunAndMoonInfo(const City& city) {
    SunSet sun;
    auto now = std::time(nullptr);
    struct tm *tad = std::localtime(&now);

    sun.setPosition(city.lat, city.lon, city.tzOffset);
    sun.setCurrentDate(tad->tm_year + 1900, tad->tm_mon + 1, tad->tm_mday);

    double sr = sun.calcSunrise();
    double ss = sun.calcSunset();
    QString sunriseStr = formatTime(sr);
    QString sunsetStr = formatTime(ss);

    int moon = sun.moonPhase();
    QString moonStr;
    if (moon == 14) moonStr = "Full Moon";
    else if (moon > 14) moonStr = QString("%1 waning").arg(moon);
    else moonStr = QString("%1 waxing").arg(moon);

    return QString("🌅 Sunrise: %1\n🌇 Sunset: %2\n🌕 Moon: %3")
            .arg(sunriseStr)
            .arg(sunsetStr)
            .arg(moonStr);
}

class CitySelectDialog : public QDialog {
public:
    CitySelectDialog(QMap<QString, QMap<QString, City>>& data, QString currentCountry, QString currentCity, QWidget* parent = nullptr)
        : QDialog(parent), cities(data) {
        setWindowTitle("Select City");
        QVBoxLayout* layout = new QVBoxLayout(this);

        countryCombo = new QComboBox(this);
        countryCombo->addItems(cities.keys());
        countryCombo->setCurrentText(currentCountry);
        layout->addWidget(countryCombo);

        cityCombo = new QComboBox(this);
        layout->addWidget(cityCombo);
        updateCityCombo(currentCountry);
        cityCombo->setCurrentText(currentCity);

        QPushButton* okBtn = new QPushButton("OK", this);
        layout->addWidget(okBtn);

        connect(countryCombo, &QComboBox::currentTextChanged, this, &CitySelectDialog::updateCityCombo);
        connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    }

    QString selectedCountry() const { return countryCombo->currentText(); }
    QString selectedCity() const { return cityCombo->currentText(); }

private slots:
    void updateCityCombo(const QString& country) {
        cityCombo->clear();
        cityCombo->addItems(cities[country].keys());
    }

private:
    QMap<QString, QMap<QString, City>>& cities;
    QComboBox *countryCombo, *cityCombo;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(QIcon::fromTheme("calendar"));

    QMenu* menu = new QMenu;
    QMap<QString, QMap<QString, City>> citiesByCountry = loadCities();
    CitySelection selection = loadSelectedCity(citiesByCountry);
    City city = selection.city;

    QAction* showCalendarAction = new QAction("Show Calendar");
    QAction* changeCityAction = new QAction("Select City...");
    QAction* quitAction = new QAction("Quit");

    QObject::connect(showCalendarAction, &QAction::triggered, [&]() {
        CalendarDialog dialog(city);
        dialog.exec();
    });

    QObject::connect(changeCityAction, &QAction::triggered, [&]() {
        CitySelectDialog dlg(citiesByCountry, selection.country, selection.cityName);
        if (dlg.exec() == QDialog::Accepted) {
            QString selectedCountry = dlg.selectedCountry();
            QString selectedCity = dlg.selectedCity();
            if (citiesByCountry[selectedCountry].contains(selectedCity)) {
                city = citiesByCountry[selectedCountry][selectedCity];
                selection = { selectedCountry, selectedCity, city };
                saveSelectedCity(selectedCountry, selectedCity);
            }
        }
    });


    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    menu->addAction(showCalendarAction);
    menu->addAction(changeCityAction);
    menu->addSeparator();
    menu->addAction(quitAction);

    trayIcon.setContextMenu(menu);
    trayIcon.show();

    // Update tray tooltip with current sunrise/sunset/moon phase every minute
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        trayIcon.setToolTip(currentSunAndMoonInfo(city));
    });
    timer.start(60000); // every 60 seconds
    trayIcon.setToolTip(currentSunAndMoonInfo(city)); // initial load


   // CitySelectDialog* cdialog = new CitySelectDialog(city);
   // cdialog->show();

    CalendarDialog* dialog = new CalendarDialog(city);
    dialog->show();


    return app.exec();
}
