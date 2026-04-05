/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/Preferences.h>

//Preferences frame constructor
PreferencesFrame::PreferencesFrame(const wxString& title) {
    wxFrame::Create(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(400, 300));
    wxPanel* panel = new wxPanel(this);
    //autosave choice
    wxStaticText* autosaveLabel = new wxStaticText(panel, wxID_ANY, "Autosave:");
    autosaveToggle = new wxChoice(panel, wxID_ANY);
    autosaveToggle->Append("On");
    autosaveToggle->Append("Off");
    wxString autosaveValue = wxConfig::Get()->Read("Preferences/Autosave", "On");
    autosaveToggle->SetStringSelection(autosaveValue);

    //open last file on startup choice
    wxStaticText* openLastFileLabel = new wxStaticText(panel, wxID_ANY, "Open last file on startup:");
    openLastFileToggle = new wxChoice(panel, wxID_ANY);
    openLastFileToggle->Append("On");
    openLastFileToggle->Append("Off");
    wxString openLastFileValue = wxConfig::Get()->Read("Preferences/OpenLastFile", "On");
    openLastFileToggle->SetStringSelection(openLastFileValue);

    //save MainFrame size and position toggle
    wxStaticText* saveWindowStateLabel = new wxStaticText(panel, wxID_ANY, "Save window size and position:");
    saveWindowStateToggle = new wxChoice(panel, wxID_ANY);
    saveWindowStateToggle->Append("On");
    saveWindowStateToggle->Append("Off");
    wxString saveWindowStateValue = wxConfig::Get()->Read("Preferences/SaveWindowState", "On");
    saveWindowStateToggle->SetStringSelection(saveWindowStateValue);


    //restere default button
    wxButton* restoreDefault = new wxButton(panel, wxID_ANY, "Restore by default");
    restoreDefault->Bind(wxEVT_BUTTON, &PreferencesFrame::OnRestoreDefault, this);

    //apply button, ok button and cancel button
    wxButton* applyButton = new wxButton(panel, wxID_APPLY, "Apply");
    wxButton* okButton = new wxButton(panel, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(panel, wxID_CANCEL, "Cancel");
    applyButton->Bind(wxEVT_BUTTON, &PreferencesFrame::OnApply, this);
    okButton->Bind(wxEVT_BUTTON, &PreferencesFrame::OnOk, this);
    cancelButton->Bind(wxEVT_BUTTON, &PreferencesFrame::OnCancel, this);

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
    saveWindowStateLabel->SetBackgroundColour(darkBackground);
    saveWindowStateLabel->SetForegroundColour(darkText);
    saveWindowStateToggle->SetBackgroundColour(buttonBg);
    saveWindowStateToggle->SetForegroundColour(buttonFg);
    restoreDefault->SetBackgroundColour(buttonBg);
    restoreDefault->SetForegroundColour(buttonFg);
    applyButton->SetBackgroundColour(buttonBg);
    applyButton->SetForegroundColour(buttonFg);
    okButton->SetBackgroundColour(buttonBg);
    okButton->SetForegroundColour(buttonFg);
    cancelButton->SetBackgroundColour(buttonBg);
    cancelButton->SetForegroundColour(buttonFg);
    //setting min sizes up
    autosaveLabel->SetMinSize(wxSize(70, -1));
    autosaveToggle->SetMinSize(wxSize(100, -1));
    openLastFileLabel->SetMinSize(wxSize(150, -1));
    openLastFileToggle->SetMinSize(wxSize(100, -1));
    saveWindowStateLabel->SetMinSize(wxSize(200, -1));
    saveWindowStateToggle->SetMinSize(wxSize(100, -1));

    //setup sizers
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 2, 10, 10); // rows, cols, vgap, hgap
    gridSizer->AddGrowableCol(0);

    gridSizer->Add(autosaveLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(autosaveToggle, 0, wxALIGN_CENTER_VERTICAL);

    gridSizer->Add(openLastFileLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(openLastFileToggle, 0, wxALIGN_CENTER_VERTICAL);

    gridSizer->Add(saveWindowStateLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(saveWindowStateToggle, 0, wxALIGN_CENTER_VERTICAL);

    gridSizer->Add(restoreDefault, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(applyButton, 0, wxRIGHT, 5);
    buttonSizer->Add(okButton, 0, wxRIGHT, 5);
    buttonSizer->Add(cancelButton, 0);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(gridSizer, 0, wxALL, 10);
    mainSizer->AddStretchSpacer();
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    panel->SetSizer(mainSizer);
    panel->Layout();
}

bool PreferencesFrame::SavePreferences() {
    wxConfigBase* config = wxConfigBase::Get();
    if (config == nullptr) {
        wxMessageBox(
            "Failed to access the application configuration.",
            "Preferences",
            wxOK | wxICON_ERROR,
            this
        );
        return false;
    }

    config->Write("Preferences/Autosave", autosaveToggle->GetStringSelection());
    config->Write("Preferences/OpenLastFile", openLastFileToggle->GetStringSelection());
    config->Write("Preferences/SaveWindowState", saveWindowStateToggle->GetStringSelection());
    config->Flush();
    return true;
}

void PreferencesFrame::OnApply(wxCommandEvent& event) {
    if (SavePreferences()) {
        Close();
    }
}

void PreferencesFrame::OnOk(wxCommandEvent& event) {
    if (SavePreferences()) {
        Close();
    }
}

void PreferencesFrame::OnCancel(wxCommandEvent& event) {
    Close();
}

void PreferencesFrame::OnRestoreDefault(wxCommandEvent& event) {
    const int result = wxMessageBox(
        "This action will reset all saved application settings to their default values. Continue?",
        "Reset settings",
        wxYES_NO | wxNO_DEFAULT | wxICON_WARNING,
        this
    );

    if (result != wxYES) {
        return;
    }

    if (autosaveToggle != nullptr) {
        autosaveToggle->SetStringSelection("Off");
    }

    if (openLastFileToggle != nullptr) {
        openLastFileToggle->SetStringSelection("Off");
    }

    if (saveWindowStateToggle != nullptr) {
        saveWindowStateToggle->SetStringSelection("Off");
    }

    wxMessageBox(
        "Default values are set in the window. Click Apply or OK to save them.",
        "Reset settings",
        wxOK | wxICON_INFORMATION,
        this
    );
}
