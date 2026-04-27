#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QMap>
#include "MotorPanel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddMotor();
    void onStartAll();
    void onStopAll();
    void onRemoveMotor(int motorId);
    void onSaveConfig();

private:
    void setupUI();
    QTabWidget *m_tabWidget;
    QMap<int, MotorPanel*> m_panels;   // motorId -> panel
};

#endif