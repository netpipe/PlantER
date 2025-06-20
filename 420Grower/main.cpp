// cannabis_simulator.cpp
// Qt 5.12 + OpenGL Cannabis Growth & Breeding Simulator
// Step 1: UI Shell + Base Structure

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QTextEdit>
#include <QListWidget>
#include <QSplitter>
#include <QOpenGLWidget>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileDialog>
#include <QFile>
#include <QPainter>
#include <QDateTime>
#include <QRandomGenerator>

// Placeholder genome structure
struct PlantGenome {
    QString strain = "Unnamed";
    QColor startColor = Qt::green;
    QColor endColor = Qt::darkGreen;
    float budDensity = 0.8f;
    float rootPriority = 0.5f;
    float cloneSuccessRate = 0.85f;
    float tipSpeed = 1.0f;
    bool twinNode = false;
    bool triploid = false;
    bool hermieRisk = false;
    bool wasSTSConverted = false;
    int seedYield = 50;
};

// Placeholder plant structure
struct Plant {
    PlantGenome genome;
    int age = 0;
    float hydration = 1.0f;
    float nutrients = 1.0f;
    bool isClone = false;
    bool isFemale = true;
};

class PlantGLWidget : public QOpenGLWidget {
public:
    QList<Plant>* plants;
    PlantGLWidget(QList<Plant>* p, QWidget* parent = nullptr) : QOpenGLWidget(parent), plants(p) {}
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);
        int y = 10;
        for (const Plant& plant : *plants) {
            QColor c = plant.genome.startColor;
            if (plant.isClone) c = Qt::cyan;
            if (plant.genome.wasSTSConverted) c = Qt::yellow;
            painter.setPen(c);
            painter.drawText(10, y += 15, QString("%1 (%2d) [%3]")
                             .arg(plant.genome.strain)
                             .arg(plant.age)
                             .arg(plant.isClone ? "Clone" : "Seed"));
        }
    }
};

class MainWindow : public QMainWindow {
    QList<Plant> plants;
    QTextEdit* console;
    PlantGLWidget* glWidget;
    QSlider* daySlider;
    int currentDay = 0;
    QTimer* timer;

public:
    MainWindow() {
        QWidget* central = new QWidget;
        QHBoxLayout* mainLayout = new QHBoxLayout(central);

        // Toolbox
        QVBoxLayout* tools = new QVBoxLayout;
        QPushButton* btnWater = new QPushButton("Water");
        QPushButton* btnFeed = new QPushButton("Feed");
        QPushButton* btnSTS = new QPushButton("Apply STS");
        QPushButton* btnClone = new QPushButton("Clone");
        QPushButton* btnBreed = new QPushButton("Breed");
        QPushButton* btnSave = new QPushButton("Save Plant");
        QPushButton* btnLoad = new QPushButton("Load Plant");
        tools->addWidget(btnWater);
        tools->addWidget(btnFeed);
        tools->addWidget(btnSTS);
        tools->addWidget(btnClone);
        tools->addWidget(btnBreed);
        tools->addWidget(btnSave);
        tools->addWidget(btnLoad);
        tools->addStretch();

        // OpenGL view + console
        QVBoxLayout* view = new QVBoxLayout;
        glWidget = new PlantGLWidget(&plants);
        glWidget->setMinimumHeight(300);
        daySlider = new QSlider(Qt::Horizontal);
        daySlider->setRange(0, 90);
        console = new QTextEdit;
        console->setReadOnly(true);
        view->addWidget(glWidget);
        view->addWidget(daySlider);
        view->addWidget(console);

        mainLayout->addLayout(tools, 1);
        mainLayout->addLayout(view, 3);
        setCentralWidget(central);
        resize(800, 600);

        // Demo data
        Plant p;
        p.genome.strain = "AK-47";
        plants.append(p);
        glWidget->update();

        connect(btnWater, &QPushButton::clicked, this, [this]() {
            for (Plant& p : plants) p.hydration = 1.0f;
            log("Watered all plants.");
        });
        connect(btnFeed, &QPushButton::clicked, this, [this]() {
            for (Plant& p : plants) p.nutrients = 1.0f;
            log("Fed all plants.");
        });
        connect(daySlider, &QSlider::valueChanged, this, [this](int val) {
            currentDay = val;
            for (Plant& p : plants) p.age = val;
            glWidget->update();
        });
        connect(btnSave, &QPushButton::clicked, this, &MainWindow::savePlant);
        connect(btnLoad, &QPushButton::clicked, this, &MainWindow::loadPlant);
    }

    void log(const QString& text) {
        console->append(QDateTime::currentDateTime().toString("hh:mm:ss ") + text);
    }

    void savePlant() {
        QString file = QFileDialog::getSaveFileName(this, "Save Plant", "plant.json");
        if (file.isEmpty()) return;
        QJsonObject obj;
        obj["strain"] = plants[0].genome.strain;
        obj["age"] = plants[0].age;
        obj["wasSTS"] = plants[0].genome.wasSTSConverted;
        QJsonDocument doc(obj);
        QFile f(file);
        if (f.open(QFile::WriteOnly)) {
            f.write(doc.toJson());
            f.close();
            log("Saved plant to " + file);
        }
    }

    void loadPlant() {
        QString file = QFileDialog::getOpenFileName(this, "Load Plant", "plant.json");
        if (file.isEmpty()) return;
        QFile f(file);
        if (f.open(QFile::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            QJsonObject obj = doc.object();
            Plant p;
            p.genome.strain = obj["strain"].toString();
            p.age = obj["age"].toInt();
            p.genome.wasSTSConverted = obj["wasSTS"].toBool();
            plants.append(p);
            log("Loaded plant: " + p.genome.strain);
            glWidget->update();
        }
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
