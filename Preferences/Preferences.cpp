/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Preferences.h"

//Preferences frame constructor
PreferencesFrame::PreferencesFrame(const wxString& title) {
    wxFrame::Create(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(400, 300));
    wxPanel* panel = new wxPanel(this);
    //autosave choice
    wxStaticText* autosaveLabel = new wxStaticText(panel, wxID_ANY, "Autosave:");
    wxChoice* autosaveToggle = new wxChoice(panel, wxID_ANY);
    autosaveToggle->Append("On");
    autosaveToggle->Append("Off");
    wxString autosaveValue = wxConfig::Get()->Read("Preferences/Autosave", "On");
    autosaveToggle->SetStringSelection(autosaveValue);
    autosaveToggle->Bind(wxEVT_CHOICE, [](wxCommandEvent& event) {
        wxConfig::Get()->Write("Preferences/Autosave", event.GetString());
        wxConfig::Get()->Flush();
    });

    //open last file on startup choice
    wxStaticText* openLastFileLabel = new wxStaticText(panel, wxID_ANY, "Open last file on startup:");
    wxChoice* openLastFileToggle = new wxChoice(panel, wxID_ANY);
    openLastFileToggle->Append("On");
    openLastFileToggle->Append("Off");
    wxString openLastFileValue = wxConfig::Get()->Read("Preferences/OpenLastFile", "On");
    openLastFileToggle->SetStringSelection(openLastFileValue);
    openLastFileToggle->Bind(wxEVT_CHOICE, [](wxCommandEvent& event) {
        wxConfig::Get()->Write("Preferences/OpenLastFile", event.GetString());
        wxConfig::Get()->Flush();
    });

    //caching color to avoid repeated ThemeSettings calls
    wxColour darkBackground = ThemeSettings::GetBackgroundColour();
    wxColour darkText = ThemeSettings::GetTextColour();
    wxColour buttonBg = ThemeSettings::GetButtonBackgroundColour();
    wxColour buttonFg = ThemeSettings::GetButtonForegroundColour();

    panel->SetBackgroundColour(darkBackground);
    panel->SetForegroundColour(darkText);
    SetBackgroundColour(darkBackground);
    SetForegroundColour(darkText);
    autosaveLabel->SetBackgroundColour(darkBackground);
    autosaveLabel->SetForegroundColour(darkText);
    autosaveToggle->SetBackgroundColour(buttonBg);
    autosaveToggle->SetForegroundColour(buttonFg);
    openLastFileLabel->SetBackgroundColour(darkBackground);
    openLastFileLabel->SetForegroundColour(darkText);
    openLastFileToggle->SetBackgroundColour(buttonBg);
    openLastFileToggle->SetForegroundColour(buttonFg);
    //setting min sizes up
    autosaveLabel->SetMinSize(wxSize(70, -1));
    autosaveToggle->SetMinSize(wxSize(100, -1));
    openLastFileLabel->SetMinSize(wxSize(150, -1));
    openLastFileToggle->SetMinSize(wxSize(100, -1));

    //setup sizers
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 2, 10, 10); // rows, cols, vgap, hgap
    gridSizer->AddGrowableCol(0);

    gridSizer->Add(autosaveLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(autosaveToggle, 0, wxALIGN_CENTER_VERTICAL);

    gridSizer->Add(openLastFileLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(openLastFileToggle, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(gridSizer, 0, wxALL, 10);
    panel->SetSizer(mainSizer);
    panel->Layout();
}