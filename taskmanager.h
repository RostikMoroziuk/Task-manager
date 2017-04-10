#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QMainWindow>
#include <QDebug>
#include <QStandardItemModel>
#include <QStandardItem>
#include <windows.h>
#include <tlhelp32.h>//навантаження процесора
#include <psapi.h>
#include <QString>
#include <QProcess>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include <QSysInfo>
#include <QTime>

namespace Ui {
class TaskManager;
}

class TaskManager : public QMainWindow
{
    Q_OBJECT

public:
    explicit TaskManager(QWidget *parent = 0);
    ~TaskManager();

private:
    Ui::TaskManager *ui;
    QTimer* timer;
    QTimer* workerTimer;
    int updateTime;//інтервал оновлень (необхідно для обчислення загрузки процесора)
    double msInHour, msInMinute, msInSecond;
    unsigned long long totalMemorySize;//загальна кількість пам'яті
    double cpu;//завантаження процесора

    QStandardItemModel* apps;//модель запущених програм
    QStandardItemModel* process;//модель запущених процесів
    QStandardItemModel* services;//модель запущених процесів
    int appsVScrollPos, appsHScrollPos;//позиція скрола аплікацій до оновлення
    int procVScrollPos, procHScrollPos;//позиція скрола процесів до оновлення
    int servicesVScrollPos, servicesHScrollPos;//позиція скрола процесів до оновлення
    QString selectedActiveApps;//ім'я виділеної програми в списку запущених програм
    QString selectedActiveProces;//ім'я виділеного процесу в списку запущених процесів
    QString selectedActiveService;//ім'я виділеного процесу в списку запущених процесів

    QString newTaskFileName;
    QProcess* newProcess;//запускає нові процеси

    void activeApps();
    void activeProcess();
    void mainSystemInfo();
    void servicesList();

    QHash<DWORD, double> timeForProces;//зберігається час виділений процесу в попередній раз

signals:
    void showActiveApps();

private slots:
    void indexApp(QModelIndex);
    void indexProc(QModelIndex);
    void on_endTask_clicked();
    void timeout();//час оновлення
    void setUpdateSpeed();
    void on_newTask_clicked();
    void on_actionExit_triggered();
    void on_endTask2_clicked();
    void updateWorkTime();
    void on_actionAuthor_triggered();
};

#endif // TASKMANAGER_H
