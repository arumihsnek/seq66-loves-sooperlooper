/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_view.cpp
 *
 *  Audio slot view - Qt presentation layer implementation.
 */

#include "widgets/sooperlooper_audio_slot_view.hpp"

namespace seq66
{

audio_slot_view::audio_slot_view (QWidget * parent)
    : QWidget(parent)
    , m_model{}
    , m_state_label(new QLabel(this))
    , m_command_label(new QLabel(this))
    , m_tempo_label(new QLabel(this))
    , m_clip_label(new QLabel(this))
    , m_start_button(new QPushButton("Start", this))
    , m_stop_button(new QPushButton("Stop", this))
    , m_tempo_combo(new QComboBox(this))
{
    setup_layout();
    refresh_labels();
}

audio_slot_view::~audio_slot_view ()
{
    /* Qt owns children. */
}

void
audio_slot_view::setup_layout ()
{
    auto * main_layout = new QVBoxLayout(this);

    /* Top row: clip + state + command */
    auto * info_layout = new QHBoxLayout();
    m_clip_label->setText("Clip: (none)");
    m_state_label->setText("State: off");
    m_command_label->setText("Command: none");
    info_layout->addWidget(m_clip_label);
    info_layout->addWidget(m_state_label);
    info_layout->addWidget(m_command_label);
    main_layout->addLayout(info_layout);

    /* Transport row: start + stop */
    auto * transport_layout = new QHBoxLayout();
    transport_layout->addWidget(m_start_button);
    transport_layout->addWidget(m_stop_button);
    main_layout->addLayout(transport_layout);

    /* Tempo row: combo + label */
    auto * tempo_layout = new QHBoxLayout();
    m_tempo_combo->addItem("Free");
    m_tempo_combo->addItem("Tape");
    m_tempo_combo->addItem("Elastic");
    m_tempo_label->setText("Tempo: free");
    tempo_layout->addWidget(m_tempo_combo);
    tempo_layout->addWidget(m_tempo_label);
    main_layout->addLayout(tempo_layout);

    setLayout(main_layout);

    /* Connections */
    connect(m_start_button, &QPushButton::clicked,
        this, &audio_slot_view::on_start_clicked);
    connect(m_stop_button, &QPushButton::clicked,
        this, &audio_slot_view::on_stop_clicked);
    connect(m_tempo_combo,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &audio_slot_view::on_tempo_changed);
}

void
audio_slot_view::update_from_model (const audio_slot_model & m)
{
    m_model = m;
    refresh_labels();
}

void
audio_slot_view::refresh_labels ()
{
    m_state_label->setText(
        QString("State: %1").arg(m_model.state_label()));
    m_command_label->setText(
        QString("Command: %1").arg(m_model.command_label()));
    m_tempo_label->setText(
        QString("Tempo: %1").arg(m_model.tempo_label()));

    if (m_model.has_clip())
        m_clip_label->setText(
            QString("Clip: %1").arg(
                QString::fromStdString(m_model.clip_uuid)));
    else
        m_clip_label->setText("Clip: (none)");

    /* Disable start when already playing; disable stop when not. */
    m_start_button->setEnabled(!m_model.transport_playing);
    m_stop_button->setEnabled(m_model.transport_playing);
}

void
audio_slot_view::on_start_clicked ()
{
    emit transport_requested(transport_action::start);
}

void
audio_slot_view::on_stop_clicked ()
{
    emit transport_requested(transport_action::stop);
}

void
audio_slot_view::on_tempo_changed (int index)
{
    auto m = static_cast<tempo_mode_selection>(index);
    emit tempo_mode_requested(m);
}

} // namespace seq66

/*
 * sooperlooper_audio_slot_view.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
