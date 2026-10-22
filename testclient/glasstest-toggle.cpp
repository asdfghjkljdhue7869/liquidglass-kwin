#include <QApplication>
#include <QTimer>
#include <QWidget>

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    QWidget w;
    w.resize(800, 500);
    w.show();
    int n = 0;
    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&]() {
        n++;
        switch (n % 4) {
        case 0:
            w.showNormal();
            break;
        case 1:
            w.showMaximized();
            break;
        case 2:
            w.showNormal();
            break;
        default:
            w.showFullScreen();
            break;
        }
    });
    t.start(300);
    QTimer::singleShot(16000, &a, &QApplication::quit);
    return a.exec();
}
