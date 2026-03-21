/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <wx/dnd.h>

class MainFrame; //forward declare to avoid include cycle

class DragNDrop : public wxFileDropTarget {
    MainFrame* m_frame;
public:
    explicit DragNDrop(MainFrame* frame) : m_frame(frame) {}
    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;
};