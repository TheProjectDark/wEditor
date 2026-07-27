/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/Preferences.h>
#include <weditor/MainFrame.h>

//Preferences frame constructor
PreferencesFrame::PreferencesFrame(MainFrame* owner, const wxString& title)
    : wxFrame(owner, wxID_ANY, title, wxDefaultPosition, wxSize(400, 300))
    , owner(owner)
{
    panel = new wxPanel(this);
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

    //change theme choice
    wxStaticText* themeLabel = new wxStaticText(panel, wxID_ANY, "Theme:");
    themeChoice = new wxChoice(panel, wxID_ANY);
    themeChoice->Append("Dark");
    themeChoice->Append("Light");
    wxString themeValue = wxConfig::Get()->Read("Preferences/Theme", "Dark");
    themeChoice->SetStringSelection(themeValue);
    ThemeSettings::SetTheme(themeValue);

    //save MainFrame size and position toggle
    wxStaticText* saveWindowStateLabel = new wxStaticText(panel, wxID_ANY, "Save window size and position:");
    saveWindowStateToggle = new wxChoice(panel, wxID_ANY);
    saveWindowStateToggle->Append("On");
    saveWindowStateToggle->Append("Off");
    wxString saveWindowStateValue = wxConfig::Get()->Read("Preferences/SaveWindowState", "On");
    saveWindowStateToggle->SetStringSelection(saveWindowStateValue);


    //restere default button
    wxButton* restoreDefault = new wxButton(panel, wxID_ANY, "Restore defaults");
    restoreDefault->Bind(wxEVT_BUTTON, &PreferencesFrame::OnRestoreDefault, this);

    //apply button, ok button and cancel button
    wxButton* applyButton = new wxButton(panel, wxID_APPLY, "Apply");
    wxButton* okButton = new wxButton(panel, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(panel, wxID_CANCEL, "Cancel");
    applyButton->Bind(wxEVT_BUTTON, &PreferencesFrame::OnApply, this);
    okButton->Bind(wxEVT_BUTTON, &PreferencesFrame::OnOk, this);
    cancelButton->Bind(wxEVT_BUTTON, &PreferencesFrame::OnCancel, this);

    //caching color to avoid repeated ThemeSettings calls
    wxColour background = ThemeSettings::GetBackgroundColour();
    wxColour text = ThemeSettings::GetTextColour();
    wxColour buttonBg = ThemeSettings::GetButtonBackgroundColour();
    wxColour buttonFg = ThemeSettings::GetButtonForegroundColour();

    panel->SetBackgroundColour(background);
    panel->SetForegroundColour(text);
    SetBackgroundColour(background);
    SetForegroundColour(text);
    autosaveLabel->SetBackgroundColour(background);
    autosaveLabel->SetForegroundColour(text);
    autosaveToggle->SetBackgroundColour(buttonBg);
    autosaveToggle->SetForegroundColour(buttonFg);
    openLastFileLabel->SetBackgroundColour(background);
    openLastFileLabel->SetForegroundColour(text);
    openLastFileToggle->SetBackgroundColour(buttonBg);
    openLastFileToggle->SetForegroundColour(buttonFg);
    themeLabel->SetBackgroundColour(background);
    themeLabel->SetForegroundColour(text);
    themeChoice->SetBackgroundColour(buttonBg);
    themeChoice->SetForegroundColour(buttonFg);
    saveWindowStateLabel->SetBackgroundColour(background);
    saveWindowStateLabel->SetForegroundColour(text);
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
    themeLabel->SetMinSize(wxSize(50, -1));
    themeChoice->SetMinSize(wxSize(100, -1));
    saveWindowStateLabel->SetMinSize(wxSize(200, -1));
    saveWindowStateToggle->SetMinSize(wxSize(100, -1));

    //setup sizers
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(5, 2, 10, 10); // rows, cols, vgap, hgap
    gridSizer->AddGrowableCol(0);

    gridSizer->Add(autosaveLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(autosaveToggle, 0, wxALIGN_CENTER_VERTICAL);

    gridSizer->Add(openLastFileLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(openLastFileToggle, 0, wxALIGN_CENTER_VERTICAL);

    gridSizer->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(themeChoice, 0, wxALIGN_CENTER_VERTICAL);

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
    ApplyTheme();
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
    config->Write("Preferences/Theme", themeChoice->GetStringSelection());
    config->Write("Preferences/SaveWindowState", saveWindowStateToggle->GetStringSelection());
    config->Flush();

    // Apply the theme immediately
    ThemeSettings::SetTheme(themeChoice->GetStringSelection());
    ApplyTheme();
    if (owner != nullptr) {
        owner->ApplyTheme();
    }

    return true;
}

void PreferencesFrame::ApplyTheme()
{
    if (panel == nullptr) {
        return;
    }

    wxColour background = ThemeSettings::GetBackgroundColour();
    wxColour text = ThemeSettings::GetTextColour();
    wxColour buttonBg = ThemeSettings::GetButtonBackgroundColour();
    wxColour buttonFg = ThemeSettings::GetButtonForegroundColour();

    panel->SetBackgroundColour(background);
    panel->SetForegroundColour(text);
    SetBackgroundColour(background);
    SetForegroundColour(text);

    for (wxWindow* child : panel->GetChildren()) {
        if (wxButton* button = dynamic_cast<wxButton*>(child)) {
            button->SetBackgroundColour(buttonBg);
            button->SetForegroundColour(buttonFg);
        } else if (wxChoice* choice = dynamic_cast<wxChoice*>(child)) {
            choice->SetBackgroundColour(buttonBg);
            choice->SetForegroundColour(buttonFg);
        } else {
            child->SetBackgroundColour(background);
            child->SetForegroundColour(text);
        }
        child->Refresh();
    }
    Refresh();
}

void PreferencesFrame::OnApply(wxCommandEvent&) {
    SavePreferences();
}

void PreferencesFrame::OnOk(wxCommandEvent&) {
    if (SavePreferences()) Close();
}

void PreferencesFrame::OnCancel(wxCommandEvent&) {
    Close();
}

void PreferencesFrame::OnRestoreDefault(wxCommandEvent&) {
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

    if (themeChoice != nullptr) {
        themeChoice->SetStringSelection("Dark");
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
