// cannabis_simulator.cpp
// Qt 5.12 + OpenGL Cannabis Growth & Breeding Simulator
// Step 3: Basic OpenGL Plant Visualization

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
#include <QtMath>

// cannabis_simulator.cpp
// Qt 5.12 + OpenGL Cannabis Growth & Breeding Simulator
// Step 5.1: Add curved/randomized branching & denser leaves

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
#include <QtMath>

// Enhanced genome structure
struct PlantGenome {
    QString strain = "Unnamed";
    QColor startColor = Qt::green;
    QColor endColor = Qt::darkGreen;

    float budDensity = 0.8f; // 0–1
    float rootPriority = 0.5f; // 0=root, 1=canopy
    float tipSpeed = 1.0f;     // tip growth multiplier
    float recoveryRate = 0.75f;

    float cloneSuccessRate = 0.85f;
    float nutrientUseRate = 1.0f;
    float waterUseRate = 1.0f;

    float foxTailingChance = 0.05f;
    float hermieChance = 0.03f;
    float coldShockThreshold = 12.0f;

    int seedYield = 50;
    float sativaRatio = 0.5f; // 0 = indica, 1 = sativa

    float plantDensity = 1.0f; // 1 = default spacing, >1 = more branches
    float maxHeight = 300.0f;  // max height for full maturity

    bool twinNode = false;
    bool triploid = false;
    bool wasSTSConverted = false;
};

struct Plant {
    PlantGenome genome;
    int age = 0;
    float hydration = 1.0f; // 0–1
    float nutrients = 1.0f;
    float health = 1.0f;    // 0–1
    bool isClone = false;
    bool isFemale = true;
    bool isHermie = false;
};

class PlantGLWidget : public QOpenGLWidget {
public:
    QList<Plant>* plants;
    PlantGLWidget(QList<Plant>* p, QWidget* parent = nullptr) : QOpenGLWidget(parent), plants(p) {}

protected:
    void drawBranch(QPainter& painter, QPointF start, float angle, int depth, float length, const Plant& plant) {
        if (depth <= 0) return;

        float curve = QRandomGenerator::global()->bounded(10.0f); // curve factor
        float dx = length * qCos(qDegreesToRadians(angle + curve));
        float dy = -length * qSin(qDegreesToRadians(angle + curve));
        QPointF end(start.x() + dx, start.y() + dy);

        QPen thinPen(painter.pen().color());
        thinPen.setWidthF(0.8);
        painter.setPen(thinPen);
        painter.drawLine(start, end);

        // Dense leaf along branch
        painter.setBrush(Qt::darkGreen);
        for (int i = 1; i <= 3; ++i) {
            float t = i / 4.0f;
            QPointF mid(start.x() + t * dx, start.y() + t * dy);
            painter.drawEllipse(mid, 4, 2);
        }

        // Budsite at tip
        if (depth == 1) {
            painter.setBrush(Qt::magenta);
            float budSize = plant.genome.budDensity * 5.0f;
            painter.drawEllipse(end, budSize, budSize);
        }

        float nextLen = length * 0.7f;
        drawBranch(painter, end, angle - 20 + QRandomGenerator::global()->bounded(10), depth - 1, nextLen, plant);
        drawBranch(painter, end, angle + 20 + QRandomGenerator::global()->bounded(10), depth - 1, nextLen, plant);
    }

    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);

        int plantIndex = 0;
        for (const Plant& plant : *plants) {
            float stemHeight = qMin(plant.age * plant.genome.tipSpeed, plant.genome.maxHeight);
            float baseY = this->height() - 20;
            float baseX = 50 + plantIndex * 120;

            QColor interpColor = QColor(
                plant.genome.startColor.red()   + (plant.genome.endColor.red()   - plant.genome.startColor.red()) * (plant.age / 90.0f),
                plant.genome.startColor.green() + (plant.genome.endColor.green() - plant.genome.startColor.green()) * (plant.age / 90.0f),
                plant.genome.startColor.blue()  + (plant.genome.endColor.blue()  - plant.genome.startColor.blue()) * (plant.age / 90.0f)
            );
            painter.setPen(interpColor);
            painter.setBrush(interpColor);

            // Draw main stem
            painter.drawRect(baseX, baseY - stemHeight, 6, stemHeight);

            // Draw curved, random branches
            int spacing = qRound(20 / plant.genome.plantDensity);
            for (int y = spacing, i = 0; y < stemHeight; y += spacing, i++) {
                QPointF node(baseX + 3, baseY - y);
                float branchLen = 30 + (1.0f - float(y) / stemHeight) * 40;
                float upwardAngle = -70 + QRandomGenerator::global()->bounded(15);
                float mirrorAngle = 45 - upwardAngle + QRandomGenerator::global()->bounded(15);
                if (i % 2 == 0) {
                    drawBranch(painter, node, -upwardAngle, 3, branchLen, plant);
                } else {
                    drawBranch(painter, node, mirrorAngle, 3, branchLen, plant);
                }
            }

            // Top bud
            painter.setBrush(Qt::magenta);
            float budSize = plant.genome.budDensity * 10.0f;
            painter.drawEllipse(QPointF(baseX + 3, baseY - stemHeight), budSize, budSize);

            // Draw label
            QString label = QString("%1 (%2d)%3")
                .arg(plant.genome.strain)
                .arg(plant.age)
                .arg(plant.isHermie ? " H" : "");
            painter.setPen(Qt::white);
            painter.drawText(baseX - 10, baseY - stemHeight - 10, label);

            plantIndex++;
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
            for (Plant& p : plants) {
                p.age = val;
                float growthStress = 0.0f;
                if (p.hydration < 0.5f) growthStress += 0.2f;
                if (p.nutrients < 0.5f) growthStress += 0.2f;
                if (QRandomGenerator::global()->generateDouble() < p.genome.hermieChance * growthStress)
                    p.isHermie = true;
            }
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
        obj["sativaRatio"] = plants[0].genome.sativaRatio;
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
            p.genome.sativaRatio = obj["sativaRatio"].toDouble();
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
