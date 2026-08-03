#if ! defined SEQ66_SOOPERLOOPER_AUDIO_SLOT_VIEW_HPP
#define SEQ66_SOOPERLOOPER_AUDIO_SLOT_VIEW_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_view.hpp
 *
 *  Audio slot view — Qt presentation layer.
 *
 *  This widget renders the audio_slot_model and emits audio_slot_action
 *  requests.  It deliberately contains NO visual decisions: all text is
 *  plain labels, no colours, no fonts, no sizes.  The design is meant to
 *  be replaced once a visual/UX decision is made.
 *
 *  Design principles:
 *  - Sending is NOT confirmation.  The view emits actions; the model
 *    observes feedback separately.
 *  - The view is a thin rendering shell over the model layer.
 *  - No irreversible visual choices are encoded here.
 *
 *  \todo M4-VISUAL: L3 visual/UX design decision required once this
 *  widget is executable and reviewable.
 */

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "audio/sooperlooper_audio_slot_widget.hpp"

namespace seq66
{

class audio_slot_view : public QWidget
{
    Q_OBJECT

public:
    explicit audio_slot_view (QWidget * parent = nullptr);
    ~audio_slot_view () override;

    /** Update the view from the model. */
    void update_from_model (const audio_slot_model & m);

    /** Get the current model (for testing). */
    const audio_slot_model & model () const { return m_model; }

signals:
    /** Emitted when the user requests a transport action. */
    void transport_requested (transport_action a);

    /** Emitted when the user requests a tempo mode change. */
    void tempo_mode_requested (tempo_mode_selection m);

private slots:
    void on_start_clicked ();
    void on_stop_clicked ();
    void on_tempo_changed (int index);

private:
    audio_slot_model m_model;

    /* Widgets — deliberately neutral, easy to replace. */
    QLabel      * m_state_label;
    QLabel      * m_command_label;
    QLabel      * m_tempo_label;
    QLabel      * m_clip_label;
    QPushButton * m_start_button;
    QPushButton * m_stop_button;
    QComboBox   * m_tempo_combo;

    void setup_layout ();
    void refresh_labels ();
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_AUDIO_SLOT_VIEW_HPP

/*
 * sooperlooper_audio_slot_view.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
