/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_grid_adapter.cpp
 *
 *  Audio slot grid adapter implementation.
 */

#include "widgets/sooperlooper_audio_slot_grid_adapter.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>

namespace seq66
{

audio_slot_grid_adapter::audio_slot_grid_adapter (QWidget * parent)
    : QWidget(parent)
    , m_model{}
    , m_state_label(new QLabel(this))
    , m_command_label(new QLabel(this))
    , m_start_button(new QPushButton("Start", this))
    , m_stop_button(new QPushButton("Stop", this))
    , m_tempo_combo(new QComboBox(this))
{
    setup_layout();
    refresh_labels();
}

audio_slot_grid_adapter::~audio_slot_grid_adapter ()
{
    /* Qt owns children. */
}

void
audio_slot_grid_adapter::init (int row, int column, int slot_id)
{
    m_row = row;
    m_column = column;
    m_slot_id = slot_id;
    m_initialized = true;
}

void
audio_slot_grid_adapter::setup_layout ()
{
    auto * layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    /* State and command labels */
    m_state_label->setText("off");
    m_command_label->setText("idle");
    layout->addWidget(m_state_label);
    layout->addWidget(m_command_label);

    /* Transport buttons */
    auto * transport_layout = new QHBoxLayout();
    m_start_button->setText("Start");
    m_stop_button->setText("Stop");
    m_start_button->setFixedHeight(20);
    m_stop_button->setFixedHeight(20);
    transport_layout->addWidget(m_start_button);
    transport_layout->addWidget(m_stop_button);
    layout->addLayout(transport_layout);

    /* Tempo combo */
    m_tempo_combo->addItem("Free");
    m_tempo_combo->addItem("Tape");
    m_tempo_combo->addItem("Elastic");
    m_tempo_combo->setFixedHeight(20);
    layout->addWidget(m_tempo_combo);

    setLayout(layout);

    /* Connections */
    connect(m_start_button, &QPushButton::clicked,
        this, &audio_slot_grid_adapter::on_transport_clicked);
    connect(m_stop_button, &QPushButton::clicked,
        this, &audio_slot_grid_adapter::on_transport_clicked);
    connect(m_tempo_combo,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &audio_slot_grid_adapter::on_tempo_changed);

    /* Selection on click */
    connect(this, &QWidget::customContextMenuRequested,
        [this]() { emit selected(m_slot_id); });
}

void
audio_slot_grid_adapter::update_from_model (const audio_slot_model & m)
{
    m_model = m;
    refresh_labels();
}

void
audio_slot_grid_adapter::refresh_labels ()
{
    m_state_label->setText(m_model.state_label());
    m_command_label->setText(m_model.command_label());
    m_start_button->setEnabled(!m_model.transport_playing);
    m_stop_button->setEnabled(m_model.transport_playing);
}

void
audio_slot_grid_adapter::set_selected (bool selected)
{
    m_selected = selected;
    if (selected)
        setStyleSheet("border: 2px solid blue;");
    else
        setStyleSheet("");
}

void
audio_slot_grid_adapter::on_transport_clicked ()
{
    auto * btn = qobject_cast<QPushButton *>(sender());
    if (btn == m_start_button)
        emit transport_requested(transport_action::start);
    else if (btn == m_stop_button)
        emit transport_requested(transport_action::stop);
}

void
audio_slot_grid_adapter::on_tempo_changed (int index)
{
    auto m = static_cast<tempo_mode_selection>(index);
    emit tempo_mode_requested(m);
}

} // namespace seq66

/*
 * sooperlooper_audio_slot_grid_adapter.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
