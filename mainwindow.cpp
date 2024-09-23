#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QString ss = "Hello";
    setWindowTitle(ss);
}

MainWindow::~MainWindow()
{
    delete ui;
}
