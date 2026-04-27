#ifndef MOTORWORKER_H
#define MOTORWORKER_H

#include <QObject>
#include <QTimer>
#include <atomic>
#include "dji_motor.hpp"
#include "pid.hpp"
#include "io.hpp"

class MotorWorker : public QObject
{
    Q_OBJECT
public:
    explicit MotorWorker(int motorId, QObject *parent = nullptr);
    ~MotorWorker();

public slots:
    void start();           // 启动控制循环（在子线程中调用）
    void stop();            // 停止控制循环
    void setTargetRpm(float rpm);
    void setPidParams(float kp, float ki, float kd);

private slots:
    void onControlTick();

signals:
    void dataUpdated(float rpm,
                     float current,
                     float target_rpm,
                     float target_current,
                     float output_angular_velocity,
                     float output_linear_velocity);
    void statusMessage(const QString &msg);
    void finished();

private:
    void cleanup();         // 清理电机/PID资源（但不删除对象本身）

    std::atomic<bool> m_running{false};
    Hardware::DJIMotor* m_motor = nullptr;
    Pid::PidPosition* m_pid = nullptr;
    QTimer* m_timer = nullptr;
    float m_target_rpm = 3.0f;
    float m_feedback_rpm = 0.0f;
    int m_motorId;
    bool m_initialized = false;   // 标记是否已初始化过（用于避免重复初始化CAN等）
    int m_debugTickCount = 0;
};

#endif