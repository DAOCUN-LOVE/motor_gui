#include "MotorWorker.h"
#include "config.hpp"
#include "config_manager.hpp"
#include <QDebug>
#include <mutex>

static std::once_flag can_init_flag;

MotorWorker::MotorWorker(int motorId, QObject *parent)
    : QObject(parent), m_motorId(motorId)
{
}

MotorWorker::~MotorWorker()
{
    stop();
    cleanup();
}

void MotorWorker::cleanup()
{
    if (m_motor) {
        if (m_motor->motor_enabled_) {
            Hardware::DJIMotorManager::unregister_motor(*m_motor);
        }
        delete m_motor;
        m_motor = nullptr;
    }
    if (m_pid) {
        delete m_pid;
        m_pid = nullptr;
    }
}

void MotorWorker::start()
{
    if (m_running) return;

    // 全局初始化CAN和电机管理器（仅一次）
    std::call_once(can_init_flag, []() {
        try {
            IO::io<CAN>.insert("can0");
            qDebug() << "CAN initialized";
        } catch (...) {
            qDebug() << "CAN already initialized or error";
        }
        Hardware::DJIMotorManager::start();
        qDebug() << "DJIMotorManager started";
    });

    // 如果之前已经创建过电机和PID，先清理再重新创建
    if (m_motor || m_pid) {
        cleanup();
    }

    // 创建电机对象
    m_motor = new Hardware::DJIMotor(6020, "can0", m_motorId);
    m_motor->enable();   // 实际上会调用 DJIMotorManager::register_motor
    emit statusMessage(QString("电机 ID %1 已使能").arg(m_motorId));

    // 加载配置文件
    auto& configManager = ConfigManager::instance();
    if (configManager.loadConfig("config.json")) {
        emit statusMessage("配置文件加载成功");
    } else {
        emit statusMessage("使用默认PID参数");
    }

    // 创建PID控制器
    // Keep PID feedback and setpoint in the same unit (RPM).
    m_feedback_rpm = 0.0f;
    auto pidConfig = configManager.getPidConfig("M6020", "speed");
    m_pid = new Pid::PidPosition(pidConfig, m_feedback_rpm);
    m_motor->setCtrl(*m_pid);
    emit statusMessage("PID 控制器已绑定");

    m_running = true;
    m_debugTickCount = 0;
    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &MotorWorker::onControlTick);
    }
    m_timer->start(1);
    emit statusMessage("控制循环已启动");
}

void MotorWorker::stop()
{
    if (!m_running) return;

    m_running = false;
    if (m_timer) {
        m_timer->stop();
    }
    cleanup();
    emit statusMessage("控制循环已停止，资源已释放");
    emit finished();
}

void MotorWorker::setTargetRpm(float rpm)
{
    m_target_rpm = rpm;
    emit statusMessage(QString("目标转速已设置为 %1 RPM").arg(rpm));
}

void MotorWorker::setPidParams(float kp, float ki, float kd)
{
    if (m_pid) {
        m_pid->kp = kp;
        m_pid->ki = ki;
        m_pid->kd = kd;
        m_pid->clean();
        if (m_motor) {
            // setCtrl stores a copy in ControllerList, so rebind after updating gains.
            m_motor->setCtrl(*m_pid);
        }
        emit statusMessage(QString("PID 参数已更新: Kp=%1, Ki=%2, Kd=%3").arg(kp).arg(ki).arg(kd));
    } else {
        emit statusMessage("PID 未初始化，无法更新参数");
    }
}

void MotorWorker::onControlTick()
{
    if (!m_running || !m_motor) {
        return;
    }

    // Refresh feedback first; PID reads this value by reference.
    m_feedback_rpm = static_cast<float>(m_motor->motor_measure_.speed_rpm);
    m_motor->set(m_target_rpm);
    float rpm = m_motor->motor_measure_.speed_rpm;
    float current = m_motor->motor_measure_.given_current;
    float target_current = m_motor->give_current;
    float output_angular_velocity = m_motor->data_.output_angular_velocity;
    float output_linear_velocity = m_motor->data_.output_linear_velocity;

    if (++m_debugTickCount >= 200) {
        m_debugTickCount = 0;
        emit statusMessage(QString("调试: target=%1 rpm, feedback=%2 rpm, command=%3, current=%4 mA")
                               .arg(m_target_rpm)
                               .arg(rpm)
                               .arg(target_current)
                               .arg(current));
    }

    emit dataUpdated(rpm, current, m_target_rpm, target_current, output_angular_velocity, output_linear_velocity);
}