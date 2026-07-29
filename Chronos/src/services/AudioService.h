#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QSoundEffect;
QT_END_NAMESPACE

namespace chronos {

class AudioService : public QObject {
    Q_OBJECT

public:
    explicit AudioService(QObject* parent = nullptr);
    ~AudioService() override;

    void playNotification();
    void setEnabled(bool enabled);
    bool isEnabled() const;

private:
    void loadSound();

    bool m_enabled = true;
    QSoundEffect* m_effect = nullptr;
};

} // namespace chronos
