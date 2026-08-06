#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

namespace arete {
namespace app { class AppContext; }
namespace services {

// Music facade over the vendored Phonio engine. Lets the command palette,
// top bar and quick actions drive playback without depending on Phonio.
class MusicService : public QObject
{
    Q_OBJECT

public:
    explicit MusicService(app::AppContext* context, QObject* parent = nullptr);

    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] QString currentTitle() const;
    [[nodiscard]] QString currentArtist() const;
    [[nodiscard]] int volumePercent() const;

    // Finds the first library track whose title contains `term` and plays it.
    bool play(const QString& term);

public slots:
    void playPause();
    void pause();
    void next();
    void previous();
    void setVolume(int percent);
    void toggleShuffle();

signals:
    void playbackChanged();
    void volumeChanged(int percent);

private:
    app::AppContext* m_context;
};

} // namespace services
} // namespace arete
