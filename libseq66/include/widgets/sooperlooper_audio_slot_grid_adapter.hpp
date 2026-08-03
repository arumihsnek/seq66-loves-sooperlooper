#if ! defined SEQ66_SOOPERLOOPER_AUDIO_SLOT_GRID_ADAPTER_HPP
#define SEQ66_SOOPERLOOPER_AUDIO_SLOT_GRID_ADAPTER_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_grid_adapter.hpp
 *
 *  Audio slot grid adapter — bridges the audio slot view into the Seq66 grid.
 *
 *  This adapter wraps the audio_slot_view to make it compatible with the
 *  qslotbutton/qslivegrid abstraction.  It provides:
 *  - A widget that can be placed in a grid cell
 *  - Creation, selection, update, and deletion lifecycle
 *  - Integration with the "New audio loop" menu action
 *
 *  Design principles:
 *  - The adapter is a thin wrapper, not a deep modification of Seq66 core
 *  - It preserves MIDI-only slots by being additive (not replacing existing slots)
 *  - It uses the audio_slot_model for state management
 */

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <string>
#include <cstdint>

#include "audio/sooperlooper_audio_slot_widget.hpp"

namespace seq66
{

/**
 *  Audio slot grid adapter.
 *
 *  Wraps the audio_slot_view to make it compatible with the Seq66 grid.
 *  Each adapter instance represents one audio slot in the grid.
 */
class audio_slot_grid_adapter : public QWidget
{
    Q_OBJECT

public:
    explicit audio_slot_grid_adapter (QWidget * parent = nullptr);
    ~audio_slot_grid_adapter () override;

    /**
     *  Initialize the adapter for a specific grid position.
     *
     *  \param row      Grid row
     *  \param column   Grid column
     *  \param slot_id  Unique slot identifier
     */
    void init (int row, int column, int slot_id);

    /** Update the adapter from the model. */
    void update_from_model (const audio_slot_model & m);

    /** Get the current model. */
    const audio_slot_model & model () const { return m_model; }

    /** Get the grid position. */
    int row () const { return m_row; }
    int column () const { return m_column; }
    int slot_id () const { return m_slot_id; }

    /** Check if this adapter is initialized. */
    bool is_initialized () const { return m_initialized; }

    /** Mark the adapter as selected/highlighted. */
    void set_selected (bool selected);

    /** Check if selected. */
    bool is_selected () const { return m_selected; }

signals:
    /** Emitted when the user requests a transport action. */
    void transport_requested (transport_action a);

    /** Emitted when the user requests a tempo mode change. */
    void tempo_mode_requested (tempo_mode_selection m);

    /** Emitted when the adapter is selected. */
    void selected (int slot_id);

    /** Emitted when the adapter is deleted. */
    void deleted (int slot_id);

public slots:
    void on_transport_clicked ();
    void on_tempo_changed (int index);

private:
    audio_slot_model m_model;
    int m_row{-1};
    int m_column{-1};
    int m_slot_id{-1};
    bool m_initialized{false};
    bool m_selected{false};

    QLabel      * m_state_label;
    QLabel      * m_command_label;
    QPushButton * m_start_button;
    QPushButton * m_stop_button;
    QComboBox   * m_tempo_combo;

    void setup_layout ();
    void refresh_labels ();
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_AUDIO_SLOT_GRID_ADAPTER_HPP

/*
 * sooperlooper_audio_slot_grid_adapter.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
