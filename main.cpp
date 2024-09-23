#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
QApplication a(argc, argv);

QPalette palette;
    a.setStyle(QStyleFactory::create("fusion"));
    palette.setColor(QPalette::Window, QColor(53,53,53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(15,15,15));
    palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53,53,53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);

    //palette.setColor(QPalette::Highlight, QColor(142,45,197).lighter());

    palette.setColor(QPalette::Highlight, QColor(67,237,241)/*.lighter()*/);
    palette.setColor(QPalette::HighlightedText, Qt::black);

    palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);

    a.setPalette(palette);

MainWindow w;
    w.show();
    auto exit_code = a.exec();
    spdlog::shutdown();
    return exit_code;
}
