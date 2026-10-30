#pragma once

#include <QPoint>
#include <QString>
#include <QWidget>

#include <lazytv/client.hpp>

class QLabel;
class QPushButton;

/**
 * Строка «сейчас в эфире» над кнопками пульта.
 *
 * Показывает номер и название канала, текущую программу и громкость.
 * Кнопка «…» открывает меню дополнительных экранов (каналы, кадр ТВ).
 */
class NowPlayingBar : public QWidget {
    Q_OBJECT
public:
    explicit NowPlayingBar(QWidget* parent = nullptr);

    void setChannel(const lazytv::ChannelInfo& channel);
    void setVolume(const lazytv::VolumeInfo& volume);
    void setUnavailable(const QString& text = QString());

signals:
    /** Запрос меню; globalPos — точка для QMenu::exec(). */
    void menuRequested(const QPoint& globalPos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyElidedText();
    void applyVolumeStyle();
    void updateStyle();

    QLabel* m_channelLabel = nullptr;
    QLabel* m_programLabel = nullptr;
    QLabel* m_volumeLabel = nullptr;
    QPushButton* m_moreButton = nullptr;

    QString m_channelText;
    QString m_programText;
    QString m_volumeText;
    bool m_volumeMuted = false;
};
