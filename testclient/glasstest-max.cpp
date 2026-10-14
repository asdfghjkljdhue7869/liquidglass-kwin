#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QLinearGradient>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <KWindowEffects>

class Backdrop : public QWidget {
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        for (int y = 0; y < height(); y += 24) {
            for (int x = 0; x < width(); x += 24) {
                bool odd = ((x / 24) + (y / 24)) % 2;
                p.fillRect(x, y, 24, 24, odd ? QColor(255, 255, 255) : QColor(15, 15, 15));
            }
        }
        QLinearGradient g(0, 0, width(), height());
        g.setColorAt(0.0, QColor(255, 60, 170, 150));
        g.setColorAt(0.5, QColor(40, 220, 255, 150));
        g.setColorAt(1.0, QColor(140, 80, 255, 150));
        p.fillRect(rect(), g);
        QFont f = font();
        f.setPointSize(30);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(255, 240, 60));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("BLUR TEST 0123456789"));
    }
    void keyPressEvent(QKeyEvent *e) override { if (e->key() == Qt::Key_Escape) qApp->quit(); }
};

class Glass : public QWidget {
public:
    Glass() { setAttribute(Qt::WA_TranslucentBackground); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(255, 255, 255, 24));
        p.setPen(QPen(QColor(255, 255, 255, 130), 1));
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 28, 28);
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("click to quit"));
    }
    void mousePressEvent(QMouseEvent *) override { qApp->quit(); }
    void keyPressEvent(QKeyEvent *e) override { if (e->key() == Qt::Key_Escape) qApp->quit(); }
};

int main(int argc, char **argv) {
    QApplication a(argc, argv);
    Backdrop b;
    b.resize(900, 600);
    b.showMaximized();
    Glass g;
    g.resize(460, 320);
    QTimer::singleShot(800, [&]() {
        g.show();
        KWindowEffects::enableBlurBehind(g.windowHandle(), true);
    });
    QTimer::singleShot(600000, &a, &QApplication::quit);
    return a.exec();
}
