#ifndef WIDGET_H
#define WIDGET_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDir>
#include <QFileInfoList>
#include <QLinkedList>
#include <QBuffer>
#include <QByteArray>
#include <QLatin1Char>
#include <QInputDialog>
#include <QLabel>
#include <QHeaderView>

namespace MusM {

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
};

class ElementList : public QTableWidget
{
    Q_OBJECT
public:
    explicit ElementList(QWidget *parent = 0);
    ~ElementList();
private slots:
    void openDir();
    void executeProcedure(char func, QString string);
signals:
    void procedureProceeded();
};

class PathLine: public QLineEdit
{
    Q_OBJECT
public:
    explicit PathLine(QWidget *parent = 0);
    ~PathLine();
private slots:
    void handleLine();
signals:
    void pathChanged();
};

class FuncWnd: public QInputDialog
{
    Q_OBJECT
public:
    explicit FuncWnd(QWidget *parent = 0);
    ~FuncWnd();
private slots:
    void openMenu();
    void prepareData();
signals:
    void transferData(char func, QString string);
};

}
#endif // WIDGET_H
