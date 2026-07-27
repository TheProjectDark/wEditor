/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <wx/wx.h>
#include <wx/config.h>
#include "ThemeSettings.h"

class MainFrame;

//Preferences frame class
class PreferencesFrame : public wxFrame {
    public:
        PreferencesFrame(MainFrame* owner, const wxString& title);
        bool SavePreferences();
        void ApplyTheme();
        void OnApply(wxCommandEvent& event);
        void OnOk(wxCommandEvent& event);
        void OnCancel(wxCommandEvent& event);
        void OnRestoreDefault(wxCommandEvent& event);

    private:
        MainFrame* owner = nullptr;
        wxPanel* panel = nullptr;
        wxChoice* autosaveToggle = nullptr;
        wxChoice* openLastFileToggle = nullptr;
        wxChoice* saveWindowStateToggle = nullptr;
        wxChoice* themeChoice = nullptr;
};
