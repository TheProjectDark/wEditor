/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "DragNDrop.h"
#include "MainFrame.h"

bool DragNDrop::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    if (!m_frame) return false;
    m_frame->OnDropFiles(filenames);
    return true;
}