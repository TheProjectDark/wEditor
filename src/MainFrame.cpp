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
#include <wx/display.h>
#ifdef __WXOSX_COCOA__
#include <objc/message.h>
#include <objc/runtime.h>
#endif

namespace
{
//macOS draws the title bar, scrollbars, menus and native controls according to the system appearance
//(light/dark). The app has its own theme setting, so make macOS use the same one. Without this a light
//app theme on a dark macOS mixes light editor colours with dark native parts and looks washed out.
//Same as [NSApp setAppearance:[NSAppearance appearanceNamed:name]], written with the Objective-C runtime
//so this file can stay a plain .cpp. (With wxWidgets 3.3 or newer wxTheApp->SetAppearance() does the same.)
//Does nothing on other platforms.
void ApplyNativeAppearance(const wxString& theme)
{
#ifdef __WXOSX_COCOA__
    Class applicationClass = objc_getClass("NSApplication");
    Class appearanceClass = objc_getClass("NSAppearance");
    Class stringClass = objc_getClass("NSString");
    if (applicationClass == nullptr || appearanceClass == nullptr || stringClass == nullptr)
    {
        return;
    }

    id application = ((id (*)(id, SEL))objc_msgSend)((id)applicationClass, sel_registerName("sharedApplication"));
    if (application == nullptr)
    {
        return;
    }

    //NSApplication.appearance exists since macOS 10.14
    const bool supported = ((BOOL (*)(id, SEL, SEL))objc_msgSend)(
        application, sel_registerName("respondsToSelector:"), sel_registerName("setAppearance:"));
    if (!supported)
    {
        return;
    }

    const char* name = (theme == "Light") ? "NSAppearanceNameAqua" : "NSAppearanceNameDarkAqua";
    id nameString = ((id (*)(id, SEL, const char*))objc_msgSend)(
        (id)stringClass, sel_registerName("stringWithUTF8String:"), name);
    id appearance = ((id (*)(id, SEL, id))objc_msgSend)(
        (id)appearanceClass, sel_registerName("appearanceNamed:"), nameString);
    ((void (*)(id, SEL, id))objc_msgSend)(application, sel_registerName("setAppearance:"), appearance);
#else
    wxUnusedVar(theme);
#endif
}

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
        bool OnInit() override;
        int OnExit() override;
};

wxIMPLEMENT_APP(App);

//main part of the code
MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title)
{
    // Load theme from config
    wxString themeValue = wxConfig::Get()->Read("Preferences/Theme", "Dark");
    ThemeSettings::SetTheme(themeValue);

    panel = new wxPanel(this);
    // set theme colors
    wxColour background = ThemeSettings::GetBackgroundColour();
    wxColour text = ThemeSettings::GetTextColour();
    
    panel->SetBackgroundColour(background);
    panel->SetForegroundColour(text);
    
    SetBackgroundColour(background);
    SetForegroundColour(text);
    // create menu
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

    //no native border: on Windows the default one is a light 3D frame around the whole editor
    textCtrl = new wxStyledTextCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    textCtrl->SetWrapMode(wxSTC_WRAP_NONE);
    //we will display horizontal scroll bar only when needed
    textCtrl->SetScrollWidth(1);
    textCtrl->SetScrollWidthTracking(true);

    ThemeSettings::ApplyTheme(textCtrl);
    //setting icon for Microsoft Windows
    #ifdef __WXMSW__
    SetIcon(wxIcon("app.ico", wxBITMAP_TYPE_ICO));
    #endif
    //the caret line highlight is set up by ThemeSettings::ApplyTheme (so it survives theme changes)
//LOOKBOTH also draws the guides on empty lines (using the indentation of the lines around them),
    //with plain "true" the guides break on every empty line
    textCtrl->SetIndentationGuides(wxSTC_IV_LOOKBOTH);
    highlightTimer.SetOwner(this);
        Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
            HighlightSyntax();
        }, highlightTimer.GetId());
    
    newFile = new ThemedButton(panel, wxID_ANY, "New file");
    saveAs = new ThemedButton(panel, wxID_ANY, "Save as");
    save = new ThemedButton(panel, wxID_ANY, "Save");
    open = new ThemedButton(panel, wxID_ANY, "Open");
    //undo and redo buttons (ctrl+z and ctrl+y)
    undo = new ThemedButton(panel, wxID_ANY, "");
    redo = new ThemedButton(panel, wxID_ANY, "");

    wxBitmap undoBmp = LoadToolbarIcon(EmbeddedIcons::edit_undo_png, EmbeddedIcons::edit_undo_png_len, wxART_UNDO);
    wxBitmap redoBmp = LoadToolbarIcon(EmbeddedIcons::edit_redo_png, EmbeddedIcons::edit_redo_png_len, wxART_REDO);

    undo->SetBitmap(undoBmp);
    redo->SetBitmap(redoBmp);

    //enable drag and drop
    DragNDrop* dropTarget = new DragNDrop(this);
    textCtrl->SetDropTarget(dropTarget);

    //language choice dropdown
    std::vector<wxString> languages = HighlighterFactory::GetAvailableLanguages();
    languageChoice = new ThemedChoice(panel, wxID_ANY);
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
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

}

void MainFrame::ApplyTheme()
{
    ApplyNativeAppearance(ThemeSettings::GetCurrentTheme());

    wxColour background = ThemeSettings::GetBackgroundColour();
    wxColour text = ThemeSettings::GetTextColour();
    wxColour buttonBackground = ThemeSettings::GetButtonBackgroundColour();
    wxColour buttonForeground = ThemeSettings::GetButtonForegroundColour();

    if (panel != nullptr) {
        panel->SetBackgroundColour(background);
        panel->SetForegroundColour(text);
    }
    SetBackgroundColour(background);
    SetForegroundColour(text);

    if (textCtrl != nullptr) {
        ThemeSettings::ApplyTheme(textCtrl);
        //ThemeSettings::ApplyTheme resets the line number margin to a fixed width
        UpdateLineNumberMargin();
    }

    if (newFile != nullptr) {
        newFile->SetBackgroundColour(buttonBackground);
        newFile->SetForegroundColour(buttonForeground);
    }
    if (saveAs != nullptr) {
        saveAs->SetBackgroundColour(buttonBackground);
        saveAs->SetForegroundColour(buttonForeground);
    }
    if (save != nullptr) {
        save->SetBackgroundColour(buttonBackground);
        save->SetForegroundColour(buttonForeground);
    }
    if (open != nullptr) {
        open->SetBackgroundColour(buttonBackground);
        open->SetForegroundColour(buttonForeground);
    }
    if (undo != nullptr) {
        undo->SetBackgroundColour(buttonBackground);
        undo->SetForegroundColour(buttonForeground);
    }
    if (redo != nullptr) {
        redo->SetBackgroundColour(buttonBackground);
        redo->SetForegroundColour(buttonForeground);
    }
    if (languageChoice != nullptr) {
        languageChoice->SetBackgroundColour(buttonBackground);
        languageChoice->SetForegroundColour(buttonForeground);
    }

    Refresh();
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
    "Bash files (*.sh)|*.sh|"
    "Batch files (*.bat;*.cmd)|*.bat;*.cmd|"
    "Assembly files (*.asm;*.s)|*.asm;*.s|"
    "SQL files (*.sql)|*.sql";

//show main frame
bool App::OnInit() {
    SetExitOnFrameDelete(true);
    wxInitAllImageHandlers();
    wxConfig::Set(new wxConfig("wEditor"));
    ApplyNativeAppearance(wxConfig::Get()->Read("Preferences/Theme", "Dark"));

    MainFrame* mainFrame = new MainFrame("wEditor");
    mainFrame->SetClientSize(mainFrame->FromDIP(wxSize(800, 600)));
    mainFrame->RestoreWindowState();
    mainFrame->Show();

    if (argc > 1) {
        //a file passed on the command line wins over the last session's file
        mainFrame->OpenFile(argv[1]);
    } else {
        mainFrame->RestoreLastFile(); //restore last opened file on startup
    }
    return true;
}

int App::OnExit()
{
    //we created the global config with new, so we have to delete it
    delete wxConfig::Set(nullptr);
    return wxApp::OnExit();
}

void MainFrame::UpdateFrameTitle()
{
    if (currentFilePath.IsEmpty())
    {
        SetTitle("wEditor");
        return;
    }

    SetTitle("wEditor - " + currentFilePath);
}

bool MainFrame::SaveToPath(const wxString& path, bool showSuccessMessage)
{
    wxFile file;
    if (!file.Create(path, true))
    {
        wxMessageBox(wxString::Format("Failed to save file: %s", path), "wEditor", wxOK | wxICON_ERROR);
        return false;
    }

    if (!file.Write(textCtrl->GetValue()))
    {
        file.Close();
        wxMessageBox(wxString::Format("Failed to save file: %s", path), "wEditor", wxOK | wxICON_ERROR);
        return false;
    }

    file.Close();
    const bool pathChanged = (path != currentFilePath);
    currentFilePath = path;
    UpdateFrameTitle();
    textCtrl->SetSavePoint();

    //a newly named file (new file / save as) gets the highlighting for its extension.
    //an unknown extension keeps whatever language is selected right now
    if (pathChanged)
    {
        const wxString detectedLanguage = GetLanguageForExtension(path);
        if (detectedLanguage != "Text")
        {
            SetLanguage(detectedLanguage);
        }
    }

    wxConfigBase* config = wxConfigBase::Get();
    if (config != nullptr)
    {
        config->Write("Session/LastFile", currentFilePath);
        config->Flush();
    }

    if (showSuccessMessage)
    {
        wxMessageBox("File saved successfully", "wEditor", wxOK | wxICON_INFORMATION);
    }

    return true;
}

bool MainFrame::SaveCurrentDocument(bool showSuccessMessage)
{
    if (!currentFilePath.IsEmpty())
    {
        return SaveToPath(currentFilePath, showSuccessMessage);
    }

    wxFileDialog saveFileDialog(
        this,
        "Save file",
        "",
        "",
        wildcard,
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );

    if (saveFileDialog.ShowModal() == wxID_CANCEL)
    {
        return false;
    }

    return SaveToPath(saveFileDialog.GetPath(), showSuccessMessage);
}

bool MainFrame::PromptToSaveChanges()
{
    if (textCtrl == nullptr || !textCtrl->GetModify())
    {
        return true;
    }

    const wxString documentName = currentFilePath.IsEmpty() ? "Untitled" : currentFilePath;
    const int result = wxMessageBox(
        wxString::Format("Save changes to \"%s\" before continuing?", documentName),
        "Unsaved changes",
        wxYES_NO | wxCANCEL | wxCANCEL_DEFAULT | wxICON_WARNING,
        this
    );

    if (result == wxCANCEL)
    {
        return false;
    }

    if (result == wxYES)
    {
        return SaveCurrentDocument(false);
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

    const wxFileOffset fileLength = file.Length();
    wxString text;
    const bool readOk = file.ReadAll(&text);
    file.Close();

    //ReadAll returns false on a read error and leaves the string empty. Stop here, otherwise the
    //editor shows an empty document for a non-empty file and the next save would overwrite it with nothing
    if (!readOk || (text.IsEmpty() && fileLength > 0))
    {
        wxMessageBox(wxString::Format("Failed to read file as text: %s", path), "wEditor", wxOK | wxICON_ERROR);
        return;
    }

    textCtrl->SetValue(text);
    textCtrl->Refresh();
    currentFilePath = path;
    UpdateFrameTitle();
    //applying syntax highlighting according to file type
    SetLanguage(GetLanguageForExtension(path));
    textCtrl->EmptyUndoBuffer();
    textCtrl->SetSavePoint();
    UpdateLineNumberMargin();
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

bool MainFrame::ShouldSaveWindowState() const
{
    wxConfigBase* config = wxConfigBase::Get();
    if (config == nullptr)
    {
        return false;
    }

    return config->Read("Preferences/SaveWindowState", "On") == "On";
}

void MainFrame::RestoreWindowState()
{
    if (!ShouldSaveWindowState())
    {
        return;
    }

    wxConfigBase* config = wxConfigBase::Get();
    if (config == nullptr)
    {
        return;
    }

    long width = 0;
    long height = 0;

    if (!config->Read("WindowState/Width", &width) || !config->Read("WindowState/Height", &height))
    {
        return;
    }

    if (width <= 0 || height <= 0)
    {
        return;
    }

    long x = wxDefaultCoord;
    long y = wxDefaultCoord;
    config->Read("WindowState/X", &x, static_cast<long>(wxDefaultCoord));
    config->Read("WindowState/Y", &y, static_cast<long>(wxDefaultCoord));

    const wxSize restoredSize(static_cast<int>(width), static_cast<int>(height));

    //ignore a saved position that is no longer on any screen (e.g. an unplugged monitor).
    //we test a point in the title bar, not the corner, because Windows reports a window that is
    //snapped to a screen edge with a few pixels of invisible border outside the screen
    const bool positionIsOnScreen = x != wxDefaultCoord && y != wxDefaultCoord &&
        wxDisplay::GetFromPoint(wxPoint(static_cast<int>(x) + restoredSize.GetWidth() / 2,
                                        static_cast<int>(y) + 10)) != wxNOT_FOUND;
    if (positionIsOnScreen)
    {
        SetSize(static_cast<int>(x), static_cast<int>(y), restoredSize.GetWidth(), restoredSize.GetHeight());
    }
    else
    {
        SetSize(restoredSize);
    }

    long wasFullScreen = 0;
    long wasMaximized = 0;
    config->Read("WindowState/IsFullScreen", &wasFullScreen, 0);
    config->Read("WindowState/IsMaximized", &wasMaximized, 0);

    if (wasFullScreen != 0)
    {
        ShowFullScreen(true);
    }
    else if (wasMaximized != 0)
    {
        Maximize(true);
    }
}

void MainFrame::SaveWindowState() const
{
    if (!ShouldSaveWindowState())
    {
        return;
    }

    wxConfigBase* config = wxConfigBase::Get();
    if (config == nullptr)
    {
        return;
    }

    const bool isFullScreen = IsFullScreen();
    const bool isMaximized = !isFullScreen && IsMaximized();

    if (!isFullScreen && !isMaximized)
    {
        const wxRect frameRect = GetRect();
        config->Write("WindowState/X", static_cast<long>(frameRect.GetX()));
        config->Write("WindowState/Y", static_cast<long>(frameRect.GetY()));
        config->Write("WindowState/Width", static_cast<long>(frameRect.GetWidth()));
        config->Write("WindowState/Height", static_cast<long>(frameRect.GetHeight()));
    }

    config->Write("WindowState/IsFullScreen", isFullScreen ? 1L : 0L);
    config->Write("WindowState/IsMaximized", isMaximized ? 1L : 0L);
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
    textCtrl->SetScrollWidth(1);
    event.Skip();
}
//select a language in the dropdown and switch the highlighter to it
void MainFrame::SetLanguage(const wxString& language) {
    //if the language is not in the dropdown, fall back to its first entry (Text)
    if (!languageChoice->SetStringSelection(language)) {
        languageChoice->SetSelection(0);
    }

    delete currentHighlighter;
    currentHighlighter = nullptr;
    currentLanguage = languageChoice->GetStringSelection();
    currentHighlighter = HighlighterFactory::CreateHighlighter(currentLanguage);
    HighlightSyntax();
}
void MainFrame::OnLanguageChange(wxCommandEvent&) {
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
    wxFileName fileName(filename);
    wxString baseName = fileName.GetFullName();
    wxString ext = filename.AfterLast('.').Lower();
    if (ext == "cpp" || ext == "h" || ext == "hpp") {
        return "C++";
    } else if (ext == "cs") {
        return "C#";
    } else if (ext == "c") {
        return "C";
    } else if (ext == "java" || ext == "jav") {
        return "Java";
    } else if (ext == "py") {
        return "Python";
    } else if (ext == "js" || ext == "jsx" || ext == "ts" || ext == "tsx") {
        return "JavaScript";
    } else if (ext == "sh") {
        return "Bash";
    } else if (ext == "bat" || ext == "cmd") {
        return "Batch";
    } else if (ext == "asm" || ext == "s") {
        return "Assembly";
    } else if (ext == "sql") {
        return "SQL Script";
    } else if (baseName == "CMakeLists.txt" || ext == "cmake") {
        return "CMake";
    } else if (baseName == "Makefile" || ext == "mk") {
        return "Makefile";
    } else {
        return "Text";
    }
}

//new file function
void MainFrame::OnNewFile(wxCommandEvent&) 
{
    if (!PromptToSaveChanges())
    {
        return;
    }

    const wxString restoreText = textCtrl->GetValue();
    const wxString restoreFilePath = currentFilePath;
    const wxString restoreLanguage = languageChoice->GetStringSelection();
    const bool restoreModified = textCtrl->GetModify();

    auto restoreDocument = [this, &restoreText, &restoreFilePath, &restoreLanguage, restoreModified]()
    {
        textCtrl->SetValue(restoreText);
        currentFilePath = restoreFilePath;
        UpdateFrameTitle();

        languageChoice->SetStringSelection(restoreLanguage);

        delete currentHighlighter;
        currentHighlighter = nullptr;
        currentLanguage = languageChoice->GetStringSelection();
        currentHighlighter = HighlighterFactory::CreateHighlighter(currentLanguage);

        HighlightSyntax();
        UpdateLineNumberMargin();
        textCtrl->EmptyUndoBuffer();

        if (!restoreModified)
        {
            textCtrl->SetSavePoint();
        }
    };

    textCtrl->SetValue("");
    textCtrl->EmptyUndoBuffer();
    textCtrl->SetSavePoint();
    currentFilePath.Clear();
    UpdateFrameTitle();
    languageChoice->SetSelection(0);

    delete currentHighlighter;
    currentHighlighter = nullptr;
    currentHighlighter = HighlighterFactory::CreateHighlighter("Text");

    HighlightSyntax();
    UpdateLineNumberMargin();

    if (!SaveCurrentDocument())
    {
        restoreDocument();
    }
}

//save as function
void MainFrame::OnSaveAs(wxCommandEvent&)
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

    SaveToPath(saveFileDialog.GetPath());
}

//save file function
void MainFrame::OnSave(wxCommandEvent&)
{
    SaveCurrentDocument();
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
        "deb", "rpm", "pkg", "app", "macho",
        //compiled bytecode
        "class",
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

    if (!PromptToSaveChanges())
    {
        return;
    }

    LoadFile(fullPath);
}

//open file dialog
void MainFrame::OnOpen(wxCommandEvent&)
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

    OpenFile(openFileDialog.GetPath());
}

//undo and redo functions
void MainFrame::OnUndo(wxCommandEvent&) {
    if (textCtrl->CanUndo()) {
        textCtrl->Undo();
    }
}
void MainFrame::OnRedo(wxCommandEvent&) {
    if (textCtrl->CanRedo()) {
        textCtrl->Redo();
    }
}

//handle drag and drop
void MainFrame::OnDropFiles(const wxArrayString& filenames)
{
    if (filenames.GetCount() > 0)
    {
        //OpenFile can show modal dialogs (unsaved changes, unsupported format). Do that after the
        //drop handler has returned, otherwise the window the file was dragged from stays blocked
        const wxString filename = filenames[0];
        CallAfter([this, filename]() { OpenFile(filename); });
    }
}

//show preferences window
void MainFrame::OnPreferences(wxCommandEvent&)
{
    //only one preferences window at a time, bring the existing one forward instead
    if (preferencesFrame)
    {
        if (preferencesFrame->IsIconized())
        {
            preferencesFrame->Iconize(false);
        }
        preferencesFrame->Raise();
        return;
    }

    preferencesFrame = new PreferencesFrame(this, "Preferences");
    preferencesFrame->SetClientSize(preferencesFrame->FromDIP(wxSize(400, 300)));
    preferencesFrame->Show();
}

//show about window
void MainFrame::OnAbout(wxCommandEvent&)
{
    wxMessageBox("wEditor is a simple cross-platform and open-source text editor written on C++ using wxWidgets library.",
                 "wEditor beta v4.0", wxOK | wxICON_INFORMATION);
}

void MainFrame::OnClose(wxCloseEvent& event)
{
    wxConfigBase* config = wxConfig::Get();
    wxString autosaveValue = "On";
    if (config != nullptr)
    {
        autosaveValue = config->Read("Preferences/Autosave", "On");
    }

    if (textCtrl != nullptr && textCtrl->GetModify())
    {
        //Veto() is only allowed when the close can be vetoed (it can't for a forced close)
        if (autosaveValue == "On" && !currentFilePath.IsEmpty())
        {
            if (!SaveToPath(currentFilePath, false) && event.CanVeto())
            {
                event.Veto();
                return;
            }
        }
        else if (!PromptToSaveChanges() && event.CanVeto())
        {
            event.Veto();
            return;
        }
    }

    if (config != nullptr)
    {
        config->Write("Session/LastFile", currentFilePath);
        SaveWindowState();
        config->Flush();
    }

    event.Skip();
}

//close app
void MainFrame::OnExit(wxCommandEvent&)
{
    //not Close(true): a forced close can't be vetoed, but OnClose relies on Veto() to keep the window open
    Close();
}