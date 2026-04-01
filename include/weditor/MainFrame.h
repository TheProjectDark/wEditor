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
#include <wx/stc/stc.h>
#include <wx/filedlg.h>
#include <wx/file.h>
#include <wx/config.h>
#include <wx/artprov.h>
#include <unordered_set>
#include "ThemeSettings.h"
#include "SyntaxHighlighter/SyntaxHighlighter.h"
#include "DragNDrop.h"
#include "Preferences.h"

//frame class and functions
class MainFrame : public wxFrame
{
    public:
        MainFrame(const wxString& title);
        ~MainFrame();
        void OnDropFiles(const wxArrayString& filenames);
        void OpenFile(const wxString& path);
        void LoadFile(const wxString& path);
        void RestoreLastFile();

    private:
        wxStyledTextCtrl* textCtrl;
        wxChoice* languageChoice;
        SyntaxHighlighter* currentHighlighter;
        wxString currentLanguage;
        wxString currentFilePath;
        //debounce for highlight
        wxTimer highlightTimer;

        //wildcard
        static const wxString wildcard;

        void OnExit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        void OnNewFile(wxCommandEvent& event);
        void OnSaveAs(wxCommandEvent& event);
        void OnSave(wxCommandEvent& event);
        void OnOpen(wxCommandEvent& event);
        void OnUndo(wxCommandEvent& event);
        void OnRedo(wxCommandEvent& event);
        void OnPreferences(wxCommandEvent& event);
        void OnText(wxCommandEvent& event);
        void OnCharAdded(wxStyledTextEvent& event);
        void OnLanguageChange(wxCommandEvent& event);
        void UpdateFrameTitle();
        void HighlightSyntax();
        wxString GetLanguageForExtension(const wxString& filename) const;
        void UpdateLineNumberMargin();
};