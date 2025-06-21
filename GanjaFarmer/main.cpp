#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QSoundEffect>
#include <QTimer>
#include <QtMath>
#include <QTime>
#include <qdebug.h>

const int WIDTH = 800, HEIGHT = 600;
const int TURRET_LIMIT = 45;

class GameScene : public QGraphicsScene {
    Q_OBJECT
public:
    GameScene() : QGraphicsScene(0,0,WIDTH,HEIGHT) {
        setBackgroundBrush(Qt::black);
        hud = addText(QString(), QFont{"Arial",14});
        hud->setDefaultTextColor(Qt::white);
        hud->setPos(10,10);

        turretBase = addPixmap(QPixmap(":/assets/images/turret_base.png"));
        turretBase->setPos(WIDTH/2 - 25, HEIGHT - 100);

        turretBarrel = addPixmap(QPixmap(":/assets/images/turret_barrel.png").scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        turretBarrel->setTransformOriginPoint(0, 15);  // left-center, matches new image
        turretBarrel->setPos(WIDTH/2, HEIGHT - 100);   // adjust Y to match turret base


        // Sounds
        shootSound.setSource(QUrl("qrc:/assets/sounds/shoot.wav"));
        explosionSound.setSource(QUrl("qrc:/assets/sounds/explosion.wav"));
        failSound.setSource(QUrl("qrc:/assets/sounds/failure.wav"));

      //  connect(&gameTimer, &QTimer::timeout, this, &GameScene::gameLoop);
     //   gameTimer.start(16);

        connect(&spawnTimer, &QTimer::timeout, this, &GameScene::spawnAircraft);
        spawnTimer.start(3000);

        updateHUD();
    }

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e) override {
        QPointF pivot = turretBarrel->mapToScene(turretBarrel->transformOriginPoint());
        QLineF line(pivot, e->scenePos());
        qreal angle = -line.angle();

        // Limit turret rotation: only allow -90° to +90°
       // if (angle > 90) angle = 90;
       // if (angle < -90) angle = -90;

        turretBarrel->setRotation(angle);
    }




    void mousePressEvent(QGraphicsSceneMouseEvent*) override { shoot(); }

private slots:
    void gameLoop() {
        for (auto it = items().begin(); it!=items().end(); ) {
            QGraphicsItem* itItem = *it++;
            if (itItem->data(0)=="troop") {
                itItem->moveBy(1,0);
                if (itItem->x() > WIDTH) {
                    removeItem(itItem);
                    delete itItem;
                    score -= 10;
                    failSound.play();
                    updateHUD();
                }
            }
        }
    }

    void spawnAircraft() {
        spawnCount++;
        if (spawnCount %10==0 && level<10) {
            level++;
            speed++;
            dropRate = qMin(dropRate+2, 10);
            updateHUD();
        }

        bool isPlane = (qrand()%2)==0;
        auto pix = QPixmap(isPlane ? ":/assets/images/airplane.png" : ":/assets/images/helicopter.png").scaled(90, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        auto ac = addPixmap(pix);
        ac->setData(0, isPlane ? "plane" : "heli");
        ac->setPos(0, qrand()%200 + 30);

        QTimer* t = new QTimer(this);
        connect(t, &QTimer::timeout, this, [=]() mutable {
            if (!ac->scene()) { t->stop(); t->deleteLater(); return; }
            ac->moveBy(speed, 0);
            if (qrand()%100 < dropRate) dropParatrooper(ac->pos());
            if (ac->x() > WIDTH) { removeItem(ac); delete ac; t->stop(); t->deleteLater(); }
        });
        t->start(40);
    }

    void dropParatrooper(const QPointF& p) {
        QGraphicsPixmapItem* trooper = addPixmap(QPixmap(":/assets/images/paratrooper.png").scaled(30, 30, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        trooper->setData(0, "falling");
        trooper->setPos(p.x(), p.y());

        QTimer* fallTimer = new QTimer(this);
        connect(fallTimer, &QTimer::timeout, this, [=]() mutable {
            if (!trooper->scene()) {
                fallTimer->stop();
                fallTimer->deleteLater();
                return;
            }

            trooper->moveBy(0, 3);  // falling speed
     trooper->setZValue(1);
            if (trooper->y() >= HEIGHT - 100) { // ground level
                trooper->setData(0, "troop");
                fallTimer->stop();
                fallTimer->deleteLater();
            }
        });
        fallTimer->start(40);
    }

    void shoot()
    {
        shootSound.play();

        auto turretTipPos = turretBarrel->mapToScene(QPointF(25,0));
        qreal turretAngle = turretBarrel->rotation();

        // Create bullet shape (adjust size and color as needed)
        QGraphicsEllipseItem* bullet = new QGraphicsEllipseItem(-2, -2, 4, 4);
        bullet->setBrush(Qt::yellow);
        bullet->setPen(Qt::NoPen);
        bullet->setPos(turretTipPos);
        bullet->setData(0, "bullet");
        addItem(bullet);

        bullet->setZValue(2);

        // Store the angle at time of firing
        qreal angle = -turretAngle;

        QTimer* bulletTimer = new QTimer(this);

        connect(bulletTimer, &QTimer::timeout, this, [=]() {
            // Move bullet in its own angle direction
            bullet->moveBy(qCos(qDegreesToRadians(angle)) * 10,
                           -qSin(qDegreesToRadians(angle)) * 10);

            // Check bounds
            if (!sceneRect().contains(bullet->pos())) {
                removeItem(bullet);
                delete bullet;
                bulletTimer->stop();
                bulletTimer->deleteLater();
                return;
            }

            // Handle collisions
            for (QGraphicsItem* item : bullet->collidingItems()) {
                QString type = item->data(0).toString();

                for (auto item : bullet->collidingItems()) {
                    qDebug() << "Bullet hit:" << item->data(0).toString();
                }


                if (type == "falling" || type == "plane" || type == "heli") {
                    if (type != "falling") {
                        explosions++;
                        explosionSound.play(); // <-- Your explosion function
                    } else {
                        kills++;
                    }

                    updateHUD(); // <- Your HUD update logic

                    removeItem(item);
                   // delete item;

                    removeItem(bullet);
                    delete bullet;

                    bulletTimer->stop();
                    bulletTimer->deleteLater();
                    return;
                }
            }
        });

        bulletTimer->start(16); // ~60 FPS
    }



private:
    QGraphicsPixmapItem *turretBase, *turretBarrel;
    QGraphicsTextItem *hud;
    QSoundEffect shootSound, explosionSound, failSound;
    QTimer gameTimer, spawnTimer;
    int score=0, kills=0, explosions=0, level=1, speed=1, dropRate=1, spawnCount=0;

    void updateHUD() {
        hud->setPlainText(QString("Score: %1   Kills: %2   Explosions: %3   Level: %4")
                          .arg(score).arg(kills).arg(explosions).arg(level));
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    qsrand(QTime::currentTime().msec());

    GameScene scene;
    QGraphicsView view(&scene);

    QPixmap backgroundPixmap(":/assets/images/background.png");  // Or your file path
    QGraphicsPixmapItem* background = new QGraphicsPixmapItem(backgroundPixmap);
    background->setZValue(-1000); // Very low so everything draws on top
    scene.addItem(background);

    view.setRenderHint(QPainter::Antialiasing);
    view.setMouseTracking(true);
    view.setFixedSize(WIDTH, HEIGHT);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.show();
    return app.exec();
}

#include "main.moc"
