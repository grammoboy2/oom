//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: mlabel.h,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MLABEL_H__
#define __MLABEL_H__

#include <QLabel>

//---------------------------------------------------------
//   MLabel
//    label widged which sends signal mousePressed
//    on mousePressEvent
//---------------------------------------------------------

class MLabel : public QLabel
{
    Q_OBJECT

protected:
    virtual void mousePressEvent(QMouseEvent*);

signals:
    void mousePressed();

public:

    MLabel(const QString& txt, QWidget* parent, const char* name = 0)
    : QLabel(txt, parent)
    {
        setObjectName(name);
    };

    MLabel(QWidget* parent, const char* name = 0)
    : QLabel(parent)
    {
        setObjectName(name);
    };
};
#endif

