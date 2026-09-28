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
#include <algorithm>
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

    editorHost = new wxPanel(panel);
    editorHost->SetBackgroundColour(ThemeSettings::GetEditorBackgroundColour());
    editorSizer = new wxBoxSizer(wxVERTICAL);
    editorHost->SetSizer(editorSizer);

    tabsBar = new wxPanel(panel);
    tabsBar->SetBackgroundColour(background);
    wxBoxSizer* tabsBarSizer = new wxBoxSizer(wxHORIZONTAL);
    tabScroll = new wxScrolledWindow(tabsBar, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxHSCROLL | wxBORDER_NONE);
    tabScroll->SetScrollRate(20, 0);
    tabSizer = new wxBoxSizer(wxHORIZONTAL);
    tabScroll->SetSizer(tabSizer);
    addTabButton = new ThemedButton(tabsBar, wxID_ANY, "+");
    addTabButton->SetMinSize(FromDIP(wxSize(24, 18)));
    addTabButton->SetMaxSize(FromDIP(wxSize(24, 18)));
    addTabButton->Bind(wxEVT_BUTTON, &MainFrame::OnNewTab, this);
    tabsBarSizer->Add(tabScroll, 1, wxEXPAND | wxRIGHT, FromDIP(4));
    tabsBarSizer->Add(addTabButton, 0, wxALIGN_CENTER_VERTICAL);
    tabsBar->SetSizer(tabsBarSizer);
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
    textCtrl = nullptr;
    //setting icon for Microsoft Windows
    #ifdef __WXMSW__
    SetIcon(wxIcon("app.ico", wxBITMAP_TYPE_ICO));
    #endif
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
    mainSizer->Add(tabsBar, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    mainSizer->Add(editorHost, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(mainSizer);
    mainSizer->SetSizeHints(this);
    //set small size for undo and redo buttons
    undo->SetMinSize(wxSize(30, -1));
    redo->SetMinSize(wxSize(30, -1));

    languageChoice->SetMinSize(wxSize(140, -1));

    AddTab();
    ActivateTab(tabs.back().id);

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
    if (editorHost != nullptr) {
        editorHost->SetBackgroundColour(ThemeSettings::GetEditorBackgroundColour());
    }
    SetBackgroundColour(background);
    SetForegroundColour(text);

    for (const EditorTab& tab : tabs) {
        ThemeSettings::ApplyTheme(tab.editor);
    }
    if (textCtrl != nullptr) {
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

    if (tabsBar != nullptr) {
        UpdateTabTheme();
    }

    Refresh();
}

MainFrame::~MainFrame()
{
    delete currentHighlighter;
    currentHighlighter = nullptr;
}

MainFrame::EditorTab* MainFrame::FindTab(int tabId)
{
    for (EditorTab& tab : tabs)
    {
        if (tab.id == tabId)
        {
            return &tab;
        }
    }
    return nullptr;
}

int MainFrame::AddTab(const wxString& filePath, const wxString& content, const wxString& language)
{
    const int tabId = nextTabId++;
    const wxString untitledName = filePath.IsEmpty()
        ? (nextUntitledNumber == 1 ? wxString("Untitled")
                                   : wxString::Format("Untitled %d", nextUntitledNumber))
        : wxString();
    if (filePath.IsEmpty())
    {
        ++nextUntitledNumber;
    }

    wxStyledTextCtrl* editor = new wxStyledTextCtrl(
        editorHost, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    editor->SetWrapMode(wxSTC_WRAP_NONE);
    editor->SetScrollWidth(1);
    editor->SetScrollWidthTracking(true);
    ThemeSettings::ApplyTheme(editor);
    //LOOKBOTH also draws guides on empty lines using the indentation of nearby lines.
    editor->SetIndentationGuides(wxSTC_IV_LOOKBOTH);
    editor->SetUseTabs(false);
    editor->SetTabWidth(4);
    editor->SetIndent(4);
    editor->SetTabIndents(true);
    editor->SetBackSpaceUnIndents(true);
    editor->Bind(wxEVT_STC_CHARADDED, &MainFrame::OnCharAdded, this);
    editor->Bind(wxEVT_STC_CHANGE, &MainFrame::OnText, this);
    editor->Bind(wxEVT_KEY_DOWN, &MainFrame::OnEditorKeyDown, this);
    editor->SetDropTarget(new DragNDrop(this));
    if (!content.IsEmpty())
    {
        editor->SetValue(content);
        editor->EmptyUndoBuffer();
        editor->SetSavePoint();
    }
    editor->Hide();
    editorSizer->Add(editor, 1, wxEXPAND);

    ThemedTabButton* tabButton = new ThemedTabButton(tabScroll, wxID_ANY, untitledName);
    tabButton->SetToolTip(filePath.IsEmpty() ? untitledName : filePath);
    tabButton->Bind(wxEVT_BUTTON, [this, tabId](wxCommandEvent&) { ActivateTab(tabId); });
    tabButton->SetCloseHandler([this, tabId]() { CloseTab(tabId); });
    tabSizer->Add(tabButton, 0, wxEXPAND | wxRIGHT, FromDIP(1));

    EditorTab tab;
    tab.id = tabId;
    tab.editor = editor;
    tab.tabButton = tabButton;
    tab.filePath = filePath;
    tab.untitledName = untitledName;
    tab.language = language;
    tabs.push_back(tab);

    tabScroll->FitInside();
    tabScroll->Layout();
    tabScroll->Scroll(tabScroll->GetScrollRange(wxHORIZONTAL), 0);
    UpdateTabLabels();
    return tabId;
}

void MainFrame::ActivateTab(int tabId)
{
    EditorTab* tab = FindTab(tabId);
    if (tab == nullptr || activeTabId == tabId)
    {
        return;
    }

    if (textCtrl != nullptr)
    {
        textCtrl->Hide();
    }

    activeTabId = tabId;
    textCtrl = tab->editor;
    currentFilePath = tab->filePath;
    textCtrl->Show();
    languageChoice->SetStringSelection(tab->language);
    SetLanguage(tab->language);
    UpdateFrameTitle();
    UpdateLineNumberMargin();
    UpdateTabTheme();
    editorHost->Layout();
    textCtrl->SetFocus();
}

void MainFrame::CloseTab(int tabId)
{
    EditorTab* tab = FindTab(tabId);
    if (tab == nullptr)
    {
        return;
    }

    ActivateTab(tabId);
    if (!PromptToSaveChanges())
    {
        return;
    }

    const auto it = std::find_if(tabs.begin(), tabs.end(), [tabId](const EditorTab& item) {
        return item.id == tabId;
    });
    if (it == tabs.end())
    {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(std::distance(tabs.begin(), it));
    const int nextTabId = tabs.size() > 1
        ? tabs[index == tabs.size() - 1 ? index - 1 : index + 1].id
        : wxID_NONE;

    editorSizer->Detach(it->editor);
    tabSizer->Detach(it->tabButton);
    it->editor->Destroy();
    it->tabButton->Destroy();
    tabs.erase(it);
    textCtrl = nullptr;
    activeTabId = wxID_NONE;

    if (nextTabId != wxID_NONE)
    {
        ActivateTab(nextTabId);
    }
    else
    {
        const int emptyTabId = AddTab();
        ActivateTab(emptyTabId);
    }

    tabScroll->FitInside();
    tabScroll->Layout();
    UpdateTabLabels();
}

void MainFrame::UpdateTabLabels()
{
    bool sizeChanged = false;
    for (EditorTab& tab : tabs)
    {
        wxString label = tab.filePath.IsEmpty() ? tab.untitledName : wxFileName(tab.filePath).GetFullName();
        if (label.IsEmpty())
        {
            label = "Untitled";
        }
        wxString visibleLabel = label;
        if (visibleLabel.length() > 24)
        {
            visibleLabel = visibleLabel.Left(21) + "…";
        }
        if (tab.tabButton->GetLabel() != visibleLabel)
        {
            tab.tabButton->SetLabel(visibleLabel);
            sizeChanged = true;
        }
        tab.tabButton->SetModified(tab.editor->GetModify());
        tab.tabButton->SetToolTip(tab.filePath.IsEmpty() ? tab.untitledName : tab.filePath);
    }

    if (sizeChanged)
    {
        tabScroll->FitInside();
        tabScroll->Layout();
        tabScroll->Scroll(tabScroll->GetScrollRange(wxHORIZONTAL), 0);
    }
}

void MainFrame::UpdateTabTheme()
{
    const wxColour inactiveBackground = ThemeSettings::GetBackgroundColour();
    const wxColour inactiveForeground = ThemedControlsDetail::Mix(
        ThemeSettings::GetTextColour(), inactiveBackground, 25);
    const wxColour plusBackground = ThemeSettings::GetButtonBackgroundColour();
    const wxColour plusForeground = ThemeSettings::GetButtonForegroundColour();
    const wxColour activeBackground = ThemeSettings::GetEditorBackgroundColour();
    const wxColour activeForeground = ThemeSettings::GetTextColour();
    const wxColour behind = ThemeSettings::GetBackgroundColour();

    tabsBar->SetBackgroundColour(behind);
    tabScroll->SetBackgroundColour(behind);
    addTabButton->SetBackgroundColour(plusBackground);
    addTabButton->SetForegroundColour(plusForeground);
    for (const EditorTab& tab : tabs)
    {
        const bool active = tab.id == activeTabId;
        tab.tabButton->SetActive(active);
        tab.tabButton->SetBackgroundColour(active ? activeBackground : inactiveBackground);
        tab.tabButton->SetForegroundColour(active ? activeForeground : inactiveForeground);
        tab.tabButton->Refresh();
    }
}

void MainFrame::OnNewTab(wxCommandEvent&)
{
    const int tabId = AddTab();
    ActivateTab(tabId);
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
    if (EditorTab* tab = FindTab(activeTabId))
    {
        tab->filePath = path;
    }
    UpdateFrameTitle();
    textCtrl->SetSavePoint();
    UpdateTabLabels();

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

    for (const EditorTab& tab : tabs)
    {
        if (!tab.filePath.IsEmpty() && wxFileName(tab.filePath).GetFullPath() == wxFileName(path).GetFullPath())
        {
            ActivateTab(tab.id);
            return;
        }
    }

    const wxString language = GetLanguageForExtension(path);
    EditorTab* activeTab = FindTab(activeTabId);
    if (activeTab != nullptr && activeTab->filePath.IsEmpty() &&
        textCtrl->GetValue().IsEmpty() && !textCtrl->GetModify())
    {
        activeTab->filePath = path;
        activeTab->untitledName.Clear();
        activeTab->language = language;
        textCtrl->SetValue(text);
        textCtrl->EmptyUndoBuffer();
        textCtrl->SetSavePoint();
        SetLanguage(language);
        textCtrl->Refresh();
        UpdateFrameTitle();
        UpdateLineNumberMargin();
        UpdateTabLabels();
        return;
    }

    const int tabId = AddTab(path, text, language);
    ActivateTab(tabId);
    UpdateTabLabels();
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
    if (textCtrl == nullptr)
    {
        return;
    }
    int lineCount = textCtrl->GetLineCount();
    int digits = std::to_string(lineCount).length();
    int width = textCtrl->TextWidth(wxSTC_STYLE_LINENUMBER, std::string(digits, '9'));
    textCtrl->SetMarginWidth(0, width + 10);
}

//syntax highlight functions
void MainFrame::OnText(wxCommandEvent& event) {
    wxStyledTextCtrl* changedEditor = dynamic_cast<wxStyledTextCtrl*>(event.GetEventObject());
    if (changedEditor != nullptr)
    {
        UpdateTabLabels();
    }
    if (changedEditor != textCtrl)
    {
        event.Skip();
        return;
    }
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
    if (EditorTab* tab = FindTab(activeTabId))
    {
        tab->language = currentLanguage;
    }
    HighlightSyntax();
}
void MainFrame::OnLanguageChange(wxCommandEvent&) {
    currentLanguage = languageChoice->GetStringSelection();
    delete currentHighlighter;
    currentHighlighter = nullptr;
    currentHighlighter = HighlighterFactory::CreateHighlighter(currentLanguage);
    if (EditorTab* tab = FindTab(activeTabId))
    {
        tab->language = currentLanguage;
    }
    HighlightSyntax();
}
void MainFrame::HighlightSyntax() {
    if (currentHighlighter && textCtrl != nullptr) {
        currentHighlighter->ApplyHighlight(textCtrl);
    }
}

//auto indent on enter
void MainFrame::OnCharAdded(wxStyledTextEvent& event)
{
    wxStyledTextCtrl* editor = dynamic_cast<wxStyledTextCtrl*>(event.GetEventObject());
    if (editor == nullptr)
    {
        return;
    }
    if (event.GetKey() == '\n')
    {
        int currentLine = editor->GetCurrentLine();

        if (currentLine > 0)
        {
            wxString prevLine = editor->GetLine(currentLine - 1);

            wxString indent;
            for (wxChar c : prevLine)
            {
                if (c == ' ' || c == '\t')
                    indent += c;
                else
                    break;
            }

            editor->AddText(indent);
        }
    }
}

void MainFrame::OnEditorKeyDown(wxKeyEvent& event)
{
    if (event.CmdDown() && event.GetKeyCode() == WXK_TAB)
    {
        if (tabs.size() > 1)
        {
            const auto it = std::find_if(tabs.begin(), tabs.end(), [this](const EditorTab& tab) {
                return tab.id == activeTabId;
            });
            if (it != tabs.end())
            {
                const std::size_t index = static_cast<std::size_t>(std::distance(tabs.begin(), it));
                const std::size_t offset = event.ShiftDown() ? tabs.size() - 1 : 1;
                ActivateTab(tabs[(index + offset) % tabs.size()].id);
            }
        }
        return;
    }

    if (event.CmdDown() && (event.GetKeyCode() == 'w' || event.GetKeyCode() == 'W'))
    {
        CloseTab(activeTabId);
        return;
    }

    event.Skip();
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

//new file function creates an untitled document tab immediately
void MainFrame::OnNewFile(wxCommandEvent&)
{
    const int tabId = AddTab();
    ActivateTab(tabId);
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
                 "wEditor beta v4.1", wxOK | wxICON_INFORMATION);
}

void MainFrame::OnClose(wxCloseEvent& event)
{
    wxConfigBase* config = wxConfig::Get();
    wxString autosaveValue = "On";
    if (config != nullptr)
    {
        autosaveValue = config->Read("Preferences/Autosave", "On");
    }

    const int previouslyActiveTab = activeTabId;
    std::vector<int> tabIds;
    tabIds.reserve(tabs.size());
    for (const EditorTab& tab : tabs)
    {
        tabIds.push_back(tab.id);
    }

    for (int tabId : tabIds)
    {
        EditorTab* tab = FindTab(tabId);
        if (tab == nullptr || !tab->editor->GetModify())
        {
            continue;
        }

        ActivateTab(tabId);
        bool savedOrDiscarded = true;
        //Veto() is only allowed when the close can be vetoed (it can't for a forced close).
        if (autosaveValue == "On" && !currentFilePath.IsEmpty())
        {
            savedOrDiscarded = SaveToPath(currentFilePath, false);
        }
        else
        {
            savedOrDiscarded = PromptToSaveChanges();
        }

        if (!savedOrDiscarded && event.CanVeto())
        {
            ActivateTab(previouslyActiveTab);
            event.Veto();
            return;
        }
    }

    if (FindTab(previouslyActiveTab) != nullptr)
    {
        ActivateTab(previouslyActiveTab);
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