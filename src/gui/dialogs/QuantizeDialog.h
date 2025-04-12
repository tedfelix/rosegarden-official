
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_QUANTIZEDIALOG_H
#define RG_QUANTIZEDIALOG_H

#include <QDialog>
#include <memory>

class QWidget;

namespace Rosegarden
{


class Quantizer;
class QuantizeParameters;


/// The "Quantize" dialog.
class QuantizeDialog : public QDialog
{
    Q_OBJECT

public:
    QuantizeDialog(QWidget *parent, bool inNotation = false);

    /// Returns a Quantizer configured to the user's specifications.
    std::shared_ptr<Quantizer> getQuantizer() const;

public slots:
    // QDialog override.
    void accept() override;

private:
    QuantizeParameters *m_quantizeParameters;
};


}

#endif
