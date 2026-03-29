/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/MainFrame.h>
#include <weditor/EmbeddedIcons.h>
#include <wx/mstream.h>

namespace
{
wxBitmap LoadToolbarIcon(const unsigned char* data, std::size_t length, const wxArtID& fallbackArtId)
{
    wxMemoryInputStream input(data, length);
    wxImage image(input, wxBITMAP_TYPE_PNG);
    if (image.IsOk())
    {
        return wxBitmap(image);
    }

    return wxArtProvider::GetBitmap(fallbackArtId, wxART_BUTTON);
}
}

//app class to launch this editor
class App : public wxApp
{
    public:
        bool OnInit();
};

wxIMPLEMENT_APP(App);

//main part of the code
MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title)
{
    wxPanel* panel = new wxPanel(this);
    //set dark theme
    wxColour darkBackground = ThemeSettings::GetBackgroundColour();
    wxColour darkText = ThemeSettings::GetTextColour();
    
    panel->SetBackgroundColour(darkBackground);
    panel->SetForegroundColour(darkText);
    
    SetBackgroundColour(darkBackground);
    SetForegroundColour(darkText);
    //create menu
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(wxID_NEW);
    menuFile->Append(wxID_SAVEAS);
    menuFile->Append(wxID_SAVE);
    menuFile->Append(wxID_OPEN);
    menuFile->Append(wxID_UNDO);
    menuFile->Append(wxID_REDO);
    menuFile->Append(wxID_PREFERENCES);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
    SetMenuBar(menuBar);

    textCtrl = new wxStyledTextCtrl(panel, wxID_ANY);
    ThemeSettings::ApplyDarkTheme(textCtrl);
    //setting icon for Microsoft Windows
    #ifdef __WXMSW__
    SetIcon(wxIcon("app.ico", wxBITMAP_TYPE_ICO));
    #endif
    //set caret line to visible with a subtle background color
    textCtrl->SetCaretLineBackground(wxColour(50, 50, 70));
    constexpr bool CARET_LINE_VISIBLE = true;
    textCtrl->SetCaretLineVisible(CARET_LINE_VISIBLE);
    textCtrl->SetIndentationGuides(true);
    textCtrl->SetWrapMode(wxSTC_WRAP_WORD);
    highlightTimer.SetOwner(this);
        Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
            HighlightSyntax();
        }, highlightTimer.GetId());
    
    wxButton* newFile = new wxButton(panel, wxID_ANY, "New file");
    wxButton* saveAs = new wxButton(panel, wxID_ANY, "Save as");
    wxButton* save = new wxButton(panel, wxID_ANY, "Save");
    wxButton* open = new wxButton(panel, wxID_ANY, "Open");
    //undo and redo buttons (ctrl+z and ctrl+y)
    wxButton* undo = new wxButton(panel, wxID_ANY, "");
    wxButton* redo = new wxButton(panel, wxID_ANY, "");

    wxBitmap undoBmp = LoadToolbarIcon(EmbeddedIcons::edit_undo_png, EmbeddedIcons::edit_undo_png_len, wxART_UNDO);
    wxBitmap redoBmp = LoadToolbarIcon(EmbeddedIcons::edit_redo_png, EmbeddedIcons::edit_redo_png_len, wxART_REDO);

    undo->SetBitmap(undoBmp);
    redo->SetBitmap(redoBmp);

    //enable drag and drop
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
    
    wxColour buttonBackground = ThemeSettings::GetButtonBackgroundColour();
    wxColour buttonForeground = ThemeSettings::GetButtonForegroundColour();
    
    newFile->SetBackgroundColour(buttonBackground);
    newFile->SetForegroundColour(buttonForeground);
    saveAs->SetBackgroundColour(buttonBackground);
    saveAs->SetForegroundColour(buttonForeground);
    save->SetBackgroundColour(buttonBackground);
    save->SetForegroundColour(buttonForeground);
    open->SetBackgroundColour(buttonBackground);
    open->SetForegroundColour(buttonForeground);
    undo->SetBackgroundColour(buttonBackground);
    undo->SetForegroundColour(buttonForeground);
    redo->SetBackgroundColour(buttonBackground);
    redo->SetForegroundColour(buttonForeground);

    languageChoice->SetBackgroundColour(buttonBackground);
    languageChoice->SetForegroundColour(buttonForeground);

    //setup sizers
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* topSizer  = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* rightButtonSizer = new wxBoxSizer(wxHORIZONTAL);

    //main buttons on left of the top bar
    buttonSizer->Add(newFile, 0, wxRIGHT, 5);
    buttonSizer->Add(saveAs, 0, wxRIGHT, 5);
    buttonSizer->Add(save, 0, wxRIGHT, 5);
    buttonSizer->Add(open, 0);
    //add undo and redo buttons right to the top bar but before language choice
    rightButtonSizer->Add(undo, 0, wxRIGHT, 5);
    rightButtonSizer->Add(redo, 0);

    topSizer->Add(buttonSizer, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    topSizer->AddStretchSpacer();
    topSizer->Add(rightButtonSizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    topSizer->Add(languageChoice, 0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(topSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(textCtrl, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(mainSizer);
    mainSizer->SetSizeHints(this);
    //set small size for undo and redo buttons
    undo->SetMinSize(wxSize(30, -1));
    redo->SetMinSize(wxSize(30, -1));

    languageChoice->SetMinSize(wxSize(140, -1));

    //tab
    textCtrl->SetUseTabs(false);
    textCtrl->SetTabWidth(4);
    textCtrl->SetIndent(4);
    textCtrl->SetTabIndents(true);
    textCtrl->SetBackSpaceUnIndents(true);
    textCtrl->Bind(wxEVT_STC_CHARADDED, &MainFrame::OnCharAdded, this);

    //setup bindings
    newFile->Bind(wxEVT_BUTTON, &MainFrame::OnNewFile, this);
    saveAs->Bind(wxEVT_BUTTON, &MainFrame::OnSaveAs, this);
    save->Bind(wxEVT_BUTTON, &MainFrame::OnSave, this);
    open->Bind(wxEVT_BUTTON, &MainFrame::OnOpen, this);
    undo->Bind(wxEVT_BUTTON, &MainFrame::OnUndo, this);
    redo->Bind(wxEVT_BUTTON, &MainFrame::OnRedo, this);
    languageChoice->Bind(wxEVT_CHOICE, &MainFrame::OnLanguageChange, this);

    Bind(wxEVT_MENU, &MainFrame::OnNewFile, this, wxID_NEW);
    Bind(wxEVT_MENU, &MainFrame::OnSaveAs, this, wxID_SAVEAS);
    Bind(wxEVT_MENU, &MainFrame::OnSave, this, wxID_SAVE);
    Bind(wxEVT_MENU, &MainFrame::OnOpen, this, wxID_OPEN);
    Bind(wxEVT_MENU, &MainFrame::OnUndo, this, wxID_UNDO);
    Bind(wxEVT_MENU, &MainFrame::OnRedo, this, wxID_REDO);
    Bind(wxEVT_MENU, &MainFrame::OnPreferences, this, wxID_PREFERENCES);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_STC_CHANGE, &MainFrame::OnText, this);

}

MainFrame::~MainFrame()
{
    delete currentHighlighter;
    currentHighlighter = nullptr;
}

//adding values to wildcard
const::wxString MainFrame::wildcard =
    "All files (*.*)|*.*|"
    "Text files (*.txt)|*.txt|"
    "C++ files (*.cpp;*.hpp;*.h)|*.cpp;*.hpp;*.h|"
    "C files (*.c;*.h)|*.c;*.h|"
    "Java files (*.java)|*.java|"
    "Python files (*.py)|*.py|"
    "Bash files (*.sh)|*sh|"
    "Batch files (*.bat;*.cmd)|*.bat;*.cmd|"
    "Assembly files (*.asm;*.s)|*.asm;*.s|"
    "SQL files (*.sql)|*.sql";

//show main frame
bool App::OnInit() {
    SetExitOnFrameDelete(true);
    wxInitAllImageHandlers();
    wxConfig::Set(new wxConfig("wEditor"));

    MainFrame* mainFrame = new MainFrame("wEditor");
    mainFrame->SetClientSize(mainFrame->FromDIP(wxSize(800, 600)));
    mainFrame->Show();
    mainFrame->RestoreLastFile(); //restore last opened file on startup

    if (argc > 1) {
        mainFrame->OpenFile(argv[1]);
    }
    return true;
}

//load file function to prevent code duplication in open file and restore last file functions
void MainFrame::LoadFile(const wxString& path) {
    wxFile file;
    if (!file.Open(path))
    {
        wxMessageBox(wxString::Format("Failed to open file: %s", path), "wEditor", wxOK | wxICON_ERROR);
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
    currentHighlighter = nullptr;
    currentLanguage = languageChoice->GetStringSelection();
    currentHighlighter = HighlighterFactory::CreateHighlighter(currentLanguage);
    HighlightSyntax();
}

//restore last opened file on startup if enabled in preferences
void MainFrame::RestoreLastFile()
{
    wxConfigBase* config = wxConfig::Get();
    wxString openLastFileValue = config->Read("Preferences/OpenLastFile", "On");
    if (openLastFileValue == "On") {
        wxString lastFilePath = config->Read("Session/LastFile", "");
        if (!lastFilePath.IsEmpty()) {
            OpenFile(lastFilePath);
        }
    }
}

//update line number margin width according to line count
void MainFrame::UpdateLineNumberMargin()
{
    int lineCount = textCtrl->GetLineCount();
    int digits = std::to_string(lineCount).length();
    int width = textCtrl->TextWidth(wxSTC_STYLE_LINENUMBER, std::string(digits, '9'));
    textCtrl->SetMarginWidth(0, width + 10);
}

//syntax highlight functions
void MainFrame::OnText(wxCommandEvent& event) {
    UpdateLineNumberMargin();
    highlightTimer.StartOnce(150);
}
void MainFrame::OnLanguageChange(wxCommandEvent& event) {
    currentLanguage = languageChoice->GetStringSelection();
    delete currentHighlighter;
    currentHighlighter = nullptr;
    currentHighlighter = HighlighterFactory::CreateHighlighter(currentLanguage);
    HighlightSyntax();
}
void MainFrame::HighlightSyntax() {
    if (currentHighlighter) {
        currentHighlighter->ApplyHighlight(textCtrl);
    }
}

//auto indent on enter
void MainFrame::OnCharAdded(wxStyledTextEvent& event)
{
    if (event.GetKey() == '\n')
    {
        int currentLine = textCtrl->GetCurrentLine();

        if (currentLine > 0)
        {
            wxString prevLine = textCtrl->GetLine(currentLine - 1);

            wxString indent;
            for (wxChar c : prevLine)
            {
                if (c == ' ' || c == '\t')
                    indent += c;
                else
                    break;
            }

            textCtrl->AddText(indent);
        }
    }
}

//get language for syntax highlight by extension
wxString MainFrame::GetLanguageForExtension(const wxString& filename) const {
    wxString ext = filename.AfterLast('.').Lower();
    if (ext == "cpp" || ext == "h" || ext == "hpp") {
        return "C++";
    } else if (ext == "cs") {
        return "C#";
    } else if (ext == "c") {
        return "C";
    } else if (ext == "java" || ext == "jav" || ext == "class") {
        return "Java";
    } else if (ext == "py") {
        return "Python";
    } else if (ext == "sh") {
        return "Bash";
    } else if (ext == "bat" || ext == "cmd") {
        return "Batch";
    } else if (ext == "asm" || ext == "s") {
        return "Assembly";
    } else if (ext == "sql") {
        return "SQL Script";
    } else {
        return "Text";
    }
}

//new file function
void MainFrame::OnNewFile(wxCommandEvent& event) 
{
    //autosave before cleaning textCtrl
    if (!currentFilePath.IsEmpty())
    {
        wxString content = textCtrl->GetValue();
        wxFile file;
        if (file.Open(currentFilePath, wxFile::write))
        {
            file.Write(content);
            file.Close();
        }
    }

    wxString prevFilePath = currentFilePath;

    textCtrl->Clear();
    textCtrl->SetValue("");
    currentFilePath.Clear();
    languageChoice->SetSelection(0);

    delete currentHighlighter;
    currentHighlighter = nullptr;
    currentHighlighter = HighlighterFactory::CreateHighlighter("Text");

    HighlightSyntax();

    wxConfigBase::Get()->DeleteEntry("Session/LastFile");
    wxConfigBase::Get()->Flush();

    OnSave(event);
    if (currentFilePath.IsEmpty() && !prevFilePath.IsEmpty())
    {
        OpenFile(prevFilePath);
    }
}

//save as function
void MainFrame::OnSaveAs(wxCommandEvent& event)
{    wxFileDialog saveFileDialog(
        this,
        "Save file",
        "",
        "",
        wildcard,
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );

    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;

    currentFilePath = saveFileDialog.GetPath();
    OnSave(event);
    wxConfigBase::Get()->Write("Session/LastFile", currentFilePath);
    wxConfigBase::Get()->Flush();
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
            wildcard,
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT
        );

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        path = saveFileDialog.GetPath();
        currentFilePath = path;  // set current file path
        wxConfigBase::Get()->Write("Session/LastFile", currentFilePath);
        wxConfigBase::Get()->Flush();
    }

    wxString content = textCtrl->GetValue();
    wxFile file;
    if (file.Open(path, wxFile::write))
    {
        file.Write(content);
        file.Close();
        wxMessageBox("File saved successfully", "wEditor", wxOK | wxICON_INFORMATION);
    }
    else
    {
        wxMessageBox("File saving error", "wEditor", wxOK | wxICON_ERROR);
    }
}

//check unsupported file formats
bool IsFileSupported(const wxString& filename) {
    wxString ext = filename.AfterLast('.').Lower();
    static const std::unordered_set<wxString> unsupportedExts = {
        //documents
        "doc", "docx", "docm", "xls", "xlsx", "xlsm",
        "ppt", "pptx", "pptm", "pdf",
        "odt", "ods", "odp", "odg", "odf", "odb", "odc", "odi", "odm",
        //archives
        "zip", "rar", "7z", "tar", "gz", "bz2", "xz",
        //images
        "jpg", "jpeg", "png", "bmp", "gif", "svg",
        "psd", "ai", "eps", "ico", "cur", "ani",
        //audio
        "mp3", "wav", "flac", "ogg",
        //video
        "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm",
        //fonts
        "ttf", "otf", "woff", "woff2", "eot",
        //executables and binaries
        "exe", "dll", "sys", "drv", "bin", "iso", "img", "raw",
        "msi", "msix", "appx", "apk", "ipa", "dmg", "so",
        "deb", "rpm", "pkg", "msi", "app", "macho",
        //virtual machines
        "vmdk", "vhd", "vhdx", "qcow2"
    };
    return unsupportedExts.find(ext) == unsupportedExts.end();
}

//open file and apply syntax highlight
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
    LoadFile(fullPath);
}

//open file dialog
void MainFrame::OnOpen(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(
        this,
        "Open file",
        "",
        "",
        wildcard,
        wxFD_OPEN | wxFD_FILE_MUST_EXIST
    );
    
    

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString path = openFileDialog.GetPath();
    if (!IsFileSupported(path)) { //check if file is supported
        wxMessageBox("wEditor does not support this file format. Please select a text or code file.", "Unsupported Format", wxOK | wxICON_WARNING);
        return;
    }

    LoadFile(path);
}

//undo and redo functions
void MainFrame::OnUndo(wxCommandEvent& event) {
    if (textCtrl->CanUndo()) {
        textCtrl->Undo();
    }
}
void MainFrame::OnRedo(wxCommandEvent& event) {
    if (textCtrl->CanRedo()) {
        textCtrl->Redo();
    }
}

//handle drag and drop
void MainFrame::OnDropFiles(const wxArrayString& filenames)
{
    if (filenames.GetCount() > 0)
    {
        wxString path = filenames[0];
        if (!IsFileSupported(path)) { //check if file is supported
            wxMessageBox("wEditor does not support this file format. Please drop a text or code file.", "Unsupported Format", wxOK | wxICON_WARNING);
            return;
        }
        LoadFile(path);
    }
}

//show preferences window
void MainFrame::OnPreferences(wxCommandEvent& event)
{    
    PreferencesFrame* preferencesFrame = new PreferencesFrame("Preferences");
    preferencesFrame->SetClientSize(preferencesFrame->FromDIP(wxSize(400, 300)));
    preferencesFrame->Show();
}

//show about window
void MainFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("wEditor is a simple cross-platform and open-source text editor written on C++ using wxWidgets library.",
                 "wEditor beta v3.1", wxOK | wxICON_INFORMATION);
}

//close app
void MainFrame::OnExit(wxCommandEvent& event)
{
    //save last opened file path to config
    wxConfigBase::Get()->Write("Session/LastFile", currentFilePath);
    wxConfigBase::Get()->Flush();

    //auto save current file on exit if enabled in preferences
    wxConfigBase* config = wxConfig::Get();
    wxString autosaveValue = config->Read("Preferences/Autosave", "On");

    if (autosaveValue == "On")
    {
        if (!currentFilePath.IsEmpty())
        {
            wxString content = textCtrl->GetValue();
            wxFile file;

            if (file.Open(currentFilePath, wxFile::write))
            {
                file.Write(content);
                file.Close();
            }
        }
    }

    Close(true);
}
