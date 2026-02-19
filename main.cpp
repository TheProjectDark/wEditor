/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/file.h>
#include "Functions/MainFrame.h"
#if defined(__WXOSX__)
#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#endif

//app class to launch this fuckass editor
class App : public wxApp
{
    public:
        bool OnInit();
};

//the main part of the code
MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title)
{
    wxPanel* panel = new wxPanel(this);
    //turning frame dark
    wxColour darkBackground(30, 30, 30);
    wxColour darkText(230, 230, 230);
    
    panel->SetBackgroundColour(darkBackground);
    panel->SetForegroundColour(darkText);
    
    SetBackgroundColour(darkBackground);
    SetForegroundColour(darkText);
    //just basically creating some shi
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(wxID_OPEN);
    menuFile->Append(wxID_SAVE);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
    SetMenuBar(menuBar);

    textCtrl = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    // On macOS, disable automatic smart quotes/dashes for the native NSTextView
#if defined(__WXOSX__)
    {
        typedef id (*ObjcMsgSendID)(id, SEL);
        typedef void (*ObjcMsgSendVoidBool)(id, SEL, BOOL);
        ObjcMsgSendID msgSendID = (ObjcMsgSendID)objc_msgSend;
        ObjcMsgSendVoidBool msgSendVoidBool = (ObjcMsgSendVoidBool)objc_msgSend;

        void* handle = textCtrl->GetHandle();
        if (handle) {
            id nsView = (id)handle;
            SEL selDocumentView = sel_getUid("documentView");
            id docView = NULL;
            if (selDocumentView) docView = msgSendID(nsView, selDocumentView);
            id textView = docView ? docView : nsView;

            SEL selSetQuote = sel_getUid("setAutomaticQuoteSubstitutionEnabled:");
            SEL selSetDash = sel_getUid("setAutomaticDashSubstitutionEnabled:");
            SEL selSetTextReplacement = sel_getUid("setAutomaticTextReplacementEnabled:");
            if (textView) {
                if (selSetQuote) msgSendVoidBool(textView, selSetQuote, NO);
                if (selSetDash) msgSendVoidBool(textView, selSetDash, NO);
                if (selSetTextReplacement) msgSendVoidBool(textView, selSetTextReplacement, NO);
            }
        }
    }
#endif
    wxButton* save = new wxButton(panel, wxID_ANY, "Save");
    wxButton* open = new wxButton(panel, wxID_ANY, "Open");

    //enabling drag and drop
    DragNDrop* dropTarget = new DragNDrop(this);
    textCtrl->SetDropTarget(dropTarget);

    //language choice dropdown
    std::vector<wxString> languages = HighlighterFactory::GetAvailableLanguages();
    languageChoice = new wxChoice(panel, wxID_ANY);
    for (const auto& lang : languages) {
        languageChoice->Append(lang);
    }
    languageChoice->SetSelection(0);
    currentHighlighter = HighlighterFactory::CreateHighlighter("Text");
    
    //ts is for style or smth like that
    textCtrl->SetBackgroundColour(wxColour(40, 40, 40));
    textCtrl->SetForegroundColour(darkText);
    
    wxColour buttonBackground(110, 110, 110);
    wxColour buttonForeground(255, 255, 255);
    
    save->SetBackgroundColour(buttonBackground);
    save->SetForegroundColour(buttonForeground);
    open->SetBackgroundColour(buttonBackground);
    open->SetForegroundColour(buttonForeground);
    languageChoice->SetBackgroundColour(buttonBackground);
    languageChoice->SetForegroundColour(buttonForeground);

    //below are all sizers
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* topSizer  = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    buttonSizer->Add(save, 0, wxRIGHT, 5);
    buttonSizer->Add(open, 0);
    topSizer->Add(buttonSizer, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    topSizer->AddStretchSpacer();
    topSizer->Add(languageChoice, 0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(topSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(textCtrl, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(mainSizer);
    mainSizer->SetSizeHints(this);
    languageChoice->SetMinSize(wxSize(140, -1));

    //below are some bindings
    save->Bind(wxEVT_BUTTON, &MainFrame::OnSave, this);
    open->Bind(wxEVT_BUTTON, &MainFrame::OnOpen, this);
    languageChoice->Bind(wxEVT_CHOICE, &MainFrame::OnLanguageChange, this);
    
    textCtrl->Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& evt) {
        if (evt.GetKeyCode() == WXK_TAB) {
            long from, to;
            textCtrl->GetSelection(&from, &to);
            textCtrl->Replace(from, to, "    ");
            textCtrl->SetInsertionPoint(from + 4);
            return;
        }
        evt.Skip();
    });
    Bind(wxEVT_MENU, &MainFrame::OnOpen, this, wxID_OPEN);
    Bind(wxEVT_MENU, &MainFrame::OnSave, this, wxID_SAVE);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_TEXT, &MainFrame::OnText, this);
}

MainFrame::~MainFrame()
{
    delete currentHighlighter;
}

wxIMPLEMENT_APP(App);

//showing the frame
bool App::OnInit() {
    SetExitOnFrameDelete(true);

    MainFrame* mainFrame = new MainFrame("wEditor");
    mainFrame->SetClientSize(mainFrame->FromDIP(wxSize(800, 600)));
    mainFrame->Show();
    wxTheApp->Yield();  // Process pending events to ensure UI is ready

    if (argc > 1) {
        mainFrame->OpenFile(argv[1]);
    }
    return true;
}



//syntax highlight functions
void MainFrame::OnText(wxCommandEvent& event) {
    HighlightSyntax();
}
void MainFrame::OnLanguageChange(wxCommandEvent& event) {
    currentLanguage = languageChoice->GetStringSelection();
    delete currentHighlighter;
    currentHighlighter = HighlighterFactory::CreateHighlighter(currentLanguage);
    HighlightSyntax();
}
void MainFrame::HighlightSyntax() {
    if (currentHighlighter) {
        currentHighlighter->ApplyHighlight(textCtrl);
    }
}

//function to get language for syntax highlighting based on file extension
wxString MainFrame::GetLanguageForExtension(const wxString& filename) const {
    wxString ext = filename.AfterLast('.').Lower();
    if (ext == "cpp" || ext == "h" || ext == "hpp") {
        return "C++";
    } else if (ext == "c") {
        return "C";
    } else if (ext == "java" || ext == "jav" || ext == "class") {
        return "Java";
    } else if (ext == "py") {
        return "Python";
    } else if (ext == "asm" || ext == "s") {
        return "Assembly";
    } else if (ext == "sql") {
        return "SQL Script";
    } else {
        return "Text";
    }
}

//save file function
void MainFrame::OnSave(wxCommandEvent& event)
{
    wxString path;
    if (!currentFilePath.IsEmpty()) {
        path = currentFilePath;
    } else {
        wxFileDialog saveFileDialog(
            this,
            "Save file",
            "",
            "",
            "Text & Code files (*.txt;*.cpp;*.h;*.hpp;*.c;*.json;*.md;*.ini)|*.txt;*.cpp;*.h;*.hpp;*.c;*.json;*.md;*.ini|All files (*.*)|*.*",
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT
        );

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        path = saveFileDialog.GetPath();
        currentFilePath = path;  // set current file path
    }

    wxString content = textCtrl->GetValue();
    wxFile file;
    if (file.Open(path, wxFile::write))
    {
        file.Write(content);
        file.Close();
        wxMessageBox("File saved succesfully");
    }
    else
    {
        wxMessageBox("File saving error");
    }
}

//protecting against unsupported file formats
bool IsFileSupported(const wxString& filename) {
    wxString ext = filename.AfterLast('.').Lower();
    std::vector<wxString> unsupportedFileFormats = {"docx", "xlsx", "pptx", "pdf", "exe", "dll", "bin", "iso", "img",
        "zip", "rar", "7z", "tar", "gz", "mp3", "wav", "flac", "ogg", "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm",
        "jpg", "jpeg", "png", "bmp", "gif", "svg", "psd", "ai", "eps", "ttf", "otf"};
    for (const auto& unsupported : unsupportedFileFormats) {
        if (ext == unsupported) return false;
    }
    return true;
}

//open associated file function, it also applies syntax highlighting according to file type
void MainFrame::OpenFile(const wxString& path)
{
    wxString fullPath = path;
    if (!wxIsAbsolutePath(fullPath)) {
        fullPath = wxGetCwd() + wxFILE_SEP_PATH + fullPath;
    }

    if (!IsFileSupported(fullPath)) { //check if file is supported
        wxMessageBox("wEditor does not support this file format. Please select a text or code file.", "Unsupported Format", wxOK | wxICON_WARNING);
        return;
    }
    wxFile file;
    if (!file.Open(fullPath))
    {
        wxMessageBox(wxString::Format("Failed to open file: %s", fullPath), "Error");
        return;
    }

    wxString text;
    file.ReadAll(&text);
    file.Close();

    textCtrl->SetValue(text);
    textCtrl->Refresh();
    currentFilePath = fullPath;
    //applying syntax highlighting according to file type
    languageChoice->SetStringSelection(GetLanguageForExtension(fullPath));

    delete currentHighlighter;
    currentLanguage = languageChoice->GetStringSelection();
    currentHighlighter = HighlighterFactory::CreateHighlighter(currentLanguage);
    HighlightSyntax();
}

//open file function
void MainFrame::OnOpen(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(
        this,
        "Open file",
        "",
        "",
        "Text & Code files (*.txt;*.cpp;*.h;*.hpp;*.c;*.json;*.md;*.ini)|*.txt;*.cpp;*.h;*.hpp;*.c;*.json;*.md;*.ini|All files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST
    );
    
    

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString path = openFileDialog.GetPath();
    if (!IsFileSupported(path)) { //check if file is supported
        wxMessageBox("wEditor does not support this file format. Please select a text or code file.", "Unsupported Format", wxOK | wxICON_WARNING);
        return;
    }

    wxFile file;
    if (!file.Open(path))
    {
        wxMessageBox("Failed to open file");
        return;
    }

    wxString text;
    file.ReadAll(&text);
    file.Close();

    textCtrl->SetValue(text);
    textCtrl->Refresh();
    currentFilePath = path;
    //applying syntax highlighting according to file type
    languageChoice->SetStringSelection(GetLanguageForExtension(path));

    delete currentHighlighter;
    currentLanguage = languageChoice->GetStringSelection();
    currentHighlighter = HighlighterFactory::CreateHighlighter(currentLanguage);
    HighlightSyntax();
}

//drag and drop function
void MainFrame::OnDropFiles(const wxArrayString& filenames)
{
    if (filenames.GetCount() > 0)
    {
        wxString path = filenames[0];
        if (!IsFileSupported(path)) { //check if file is supported
            wxMessageBox("wEditor does not support this file format. Please drop a text or code file.", "Unsupported Format", wxOK | wxICON_WARNING);
            return;
        }
        wxFile file;
        if (!file.Open(path))
        {
            wxMessageBox("Failed to open file");
            return;
        }

        wxString text;
        file.ReadAll(&text);
        file.Close();

        textCtrl->SetValue(text);
        textCtrl->Refresh();
        //applying syntax highlighting according to file type
        languageChoice->SetStringSelection(GetLanguageForExtension(path));

        delete currentHighlighter;
        currentLanguage = languageChoice->GetStringSelection();
        currentHighlighter = HighlighterFactory::CreateHighlighter(currentLanguage);
        HighlightSyntax();
    }
}

//ts shows the about window
void MainFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("wEditor is simple cross-platform and open-souce text editor written on C++ using wxWidgets framework.",
                 "wEditor beta v1", wxOK | wxICON_INFORMATION);
}

//ts closes the app
void MainFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}