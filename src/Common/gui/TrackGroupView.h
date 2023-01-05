#ifndef TRACKGROUPVIEW_H
#define TRACKGROUPVIEW_H

#include <QFrame>
#include <QLineEdit>
#include <QBoxLayout>
#include <QGridLayout>

namespace Ui {
class TrackGroupView;
}

class BaseTrackView;
class QMenu;

class TrackGroupView : public QFrame
{
    Q_OBJECT

public:
    explicit TrackGroupView(QWidget *parent = 0);
    virtual ~TrackGroupView();

    virtual BaseTrackView *addTrackView(long trackID);

    void removeTrackView(BaseTrackView *trackView);
    void removeTrackView(int index);

    QSize sizeHint() const override;

    virtual void updateGuiElements();

    int getTracksCount() const;

    void setUnlightStatus(bool unlighted);
    bool isUnlighted() const;

    template<class T, class F>
    void visitTracks(F&& visitor) const {
        foreach (BaseTrackView *trackView, trackViews) {
            T* castedTrack = dynamic_cast<T*>(trackView);
            if (castedTrack != nullptr) {
                visitor(castedTrack);
            }
        }
    }

    template<class T>
    T* getTrack(int index) const {
        return dynamic_cast<T*>(trackViews.at(index));
    }

    void setTintColor(const QColor &color);

protected:

    void changeEvent(QEvent *) override;

    virtual void translateUi();

    virtual void populateContextMenu(QMenu &contextMenu);

    QList<BaseTrackView *> trackViews;

    virtual BaseTrackView *createTrackView(long trackID) = 0;

    QWidget *topPanel;
    QBoxLayout *tracksLayout;
    QBoxLayout *topPanelLayout;
    QGridLayout *mainLayout;

    virtual void refreshStyleSheet();

private slots:
    void showContextMenu(const QPoint &pos);
    void showPeakMeterOnly();
    void showRmsOnly();
    void showPeakAndRms();
    void showMaxPeakMarker(bool showMarker);

private:
    void setupUI();
};


inline int TrackGroupView::getTracksCount() const
{
    return trackViews.size();
}

#endif // TRACKGROUPVIEW_H
