/***************************************************************************
                          trackscanvas.h  -  description
                             -------------------
    begin                : Mon May 7 2001
    copyright            : (C) 2001 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TRACKSCANVAS_H
#define TRACKSCANVAS_H

#include <qwidget.h>
#include <qcanvas.h>

class TrackPart;

class TrackPartItem : public QCanvasRectangle
{
public:
    TrackPartItem(QCanvas* canvas);
    TrackPartItem(const QRect &, QCanvas* canvas);
    TrackPartItem(int x, int y, int width, int height, QCanvas* canvas);

    void setPart(TrackPart *p) { m_part = p; }
    TrackPart* part()          { return m_part; }

protected:

    TrackPart* m_part;

    // add track data here
};



/**A class to visualize and edit track parts

  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class TracksCanvas : public QCanvasView  {
   Q_OBJECT
public:
    TracksCanvas(int gridH, int gridV,
                 QCanvas&,
                 QWidget* parent=0, const char* name=0, WFlags f=0);
    ~TracksCanvas();

    void clear();
    unsigned int gridHStep() const { return m_grid.hstep(); }

    class SnapGrid
    {
    public:
        SnapGrid(unsigned int hstep, unsigned int vstep)
            : m_hstep(hstep), m_vstep(vstep)
        {}

        int snapX(int x) const { return x / m_hstep * m_hstep; }
        int snapY(int y) const { return y / m_vstep * m_vstep; }

        unsigned int hstep() const { return m_hstep; }
        unsigned int vstep() const { return m_vstep; }

    protected:
        unsigned int m_hstep;
        unsigned int m_vstep;
    };

    const SnapGrid& grid() const { return m_grid; }

    TrackPartItem* addPartItem(int x, int y, unsigned int nbBars);

public slots:
    virtual void update();

protected:
    void contentsMousePressEvent(QMouseEvent*);
    void contentsMouseReleaseEvent(QMouseEvent*);
    void contentsMouseMoveEvent(QMouseEvent*);
    virtual void wheelEvent(QWheelEvent*);

    TrackPartItem* findPartClickedOn(QPoint);

signals:
    void addTrackPart(TrackPart*);
    void deleteTrackPart(TrackPart*);
    void editTrackPart(TrackPart*);

private:
    bool m_newRect;

    SnapGrid m_grid;

    TrackPartItem* m_currentItem;

    QCanvasItem* m_moving;

    QBrush *m_brush;
    QPen *m_pen;

    QPopupMenu *m_editMenu;
    
};


class TrackPart
{
public:
    TrackPart(TrackPartItem *r, unsigned int widthToLengthRatio);
    ~TrackPart();

    int trackNb() const { return m_trackNb; }
    void setTrackNb(int nb) { m_trackNb = nb; }

    unsigned int length() const { return m_length; }

    TrackPartItem* canvasPartItem() { return m_canvasPartItem; }

    void updateLength();

protected:
    /// The track this part belongs to
    int m_trackNb;
    /// Part length
    unsigned int m_length;

    /// the rect. width / track length ratio
    unsigned int m_widthToLengthRatio;

    TrackPartItem *m_canvasPartItem;
};


#endif
