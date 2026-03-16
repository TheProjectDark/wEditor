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
    wxConfigBase* config = wxConfig::Get();
    wxString autosaveValue = config->Read("Preferences/Autosave", "On");
    autosaveToggle->SetStringSelection(autosaveValue);
    autosaveToggle->Bind(wxEVT_CHOICE, [config](wxCommandEvent& event) {
        wxString selection = event.GetString();
        config->Write("Preferences/Autosave", selection);
        config->Flush();
    });
    //open last file on startup choice
    wxStaticText* openLastFileLabel = new wxStaticText(panel, wxID_ANY, "Open last file on startup:");
    wxChoice* openLastFileToggle = new wxChoice(panel, wxID_ANY);
    openLastFileToggle->Append("On");
    openLastFileToggle->Append("Off");
    wxString openLastFileValue = config->Read("Preferences/OpenLastFile", "On");
    openLastFileToggle->SetStringSelection(openLastFileValue);
    openLastFileToggle->Bind(wxEVT_CHOICE, [config](wxCommandEvent& event) {
        wxString selection = event.GetString();
        config->Write("Preferences/OpenLastFile", selection);
        config->Flush();
    });

    //set dark theme
    wxColour darkBackground = ThemeSettings::GetBackgroundColour();
    wxColour darkText = ThemeSettings::GetTextColour();
    panel->SetBackgroundColour(darkBackground);
    panel->SetForegroundColour(darkText);
    SetBackgroundColour(darkBackground);
    SetForegroundColour(darkText);
    autosaveLabel->SetBackgroundColour(darkBackground);
    autosaveLabel->SetForegroundColour(darkText);
    autosaveToggle->SetBackgroundColour(ThemeSettings::GetButtonBackgroundColour());
    autosaveToggle->SetForegroundColour(ThemeSettings::GetButtonForegroundColour());
    openLastFileLabel->SetBackgroundColour(darkBackground);
    openLastFileLabel->SetForegroundColour(darkText);
    openLastFileToggle->SetBackgroundColour(ThemeSettings::GetButtonBackgroundColour());
    openLastFileToggle->SetForegroundColour(ThemeSettings::GetButtonForegroundColour());

    //setup sizers
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* autosaveSizer = new wxBoxSizer(wxHORIZONTAL);
    autosaveSizer->Add(autosaveLabel, 0, wxRIGHT, 5);
    autosaveSizer->Add(autosaveToggle, 0);
    mainSizer->Add(autosaveSizer, 0, wxALL, 10);
    panel->SetSizer(mainSizer);
    //set small size for autosave toggle
    autosaveToggle->SetMinSize(wxSize(100, -1));
    autosaveLabel->SetMinSize(wxSize(70, -1));
    wxBoxSizer* openLastFileSizer = new wxBoxSizer(wxHORIZONTAL);
    openLastFileSizer->Add(openLastFileLabel, 0, wxRIGHT, 5);
    openLastFileSizer->Add(openLastFileToggle, 0);
    mainSizer->Add(openLastFileSizer, 0, wxALL, 10);
    openLastFileToggle->SetMinSize(wxSize(100, -1));
    openLastFileLabel->SetMinSize(wxSize(70, -1));
}