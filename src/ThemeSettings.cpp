/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <weditor/ThemeSettings.h>

wxString ThemeSettings::currentTheme = "Dark";

void ThemeSettings::SetTheme(const wxString& theme)
{
    currentTheme = theme;
}

wxString ThemeSettings::GetCurrentTheme()
{
    return currentTheme;
}

wxColour ThemeSettings::GetBackgroundColour()
{
    if (currentTheme == "Light") {
        return wxColour(250, 250, 247);
    }
    return wxColour(DARK_BG_RED, DARK_BG_GREEN, DARK_BG_BLUE);
}

wxColour ThemeSettings::GetTextColour()
{
    if (currentTheme == "Light") {
        return wxColour(23, 25, 30);
    }
    return wxColour(DARK_TEXT_RED, DARK_TEXT_GREEN, DARK_TEXT_BLUE);
}

wxColour ThemeSettings::GetButtonBackgroundColour()
{
    if (currentTheme == "Light") {
        return wxColour(239, 241, 246);
    }
    return wxColour(DARK_BUTTON_BG_RED, DARK_BUTTON_BG_GREEN, DARK_BUTTON_BG_BLUE);
}

wxColour ThemeSettings::GetButtonForegroundColour()
{
    if (currentTheme == "Light") {
        return wxColour(23, 25, 30);
    }
    return wxColour(DARK_BUTTON_FG_RED, DARK_BUTTON_FG_GREEN, DARK_BUTTON_FG_BLUE);
}

wxColour ThemeSettings::GetEditorBackgroundColour()
{
    if (currentTheme == "Light") {
        return wxColour(255, 255, 255);
    }
    return wxColour(38, 38, 38);
}

wxColour ThemeSettings::GetCaretLineBackgroundColour()
{
    if (currentTheme == "Light") {
        return wxColour(227, 232, 240);
    }
    return wxColour(50, 50, 70);
}

wxColour ThemeSettings::GetSelectionBackgroundColour()
{
    if (currentTheme == "Light") {
        return wxColour(188, 204, 226);
    }
    return wxColour(60, 60, 90);
}

wxColour ThemeSettings::GetLineNumberBackgroundColour()
{
    if (currentTheme == "Light") {
        return wxColour(245, 246, 248);
    }
    return wxColour(32, 32, 32);
}

wxColour ThemeSettings::GetLineNumberForegroundColour()
{
    if (currentTheme == "Light") {
        return wxColour(112, 118, 128);
    }
    return wxColour(120, 120, 120);
}

void ThemeSettings::ApplyTheme(wxStyledTextCtrl* textCtrl)
{
    wxColour bg = GetEditorBackgroundColour();
    wxColour fg = GetTextColour();
    wxColour sel = GetSelectionBackgroundColour();
    wxColour lineNumberBg = GetLineNumberBackgroundColour();
    wxColour lineNumberFg = GetLineNumberForegroundColour();

    //Syntax colours. The values in the header are tuned for the dark theme. On the white editor several
    //of them are hard to read (light blue keywords ~3:1 contrast, orange strings ~2.3:1, light green
    //numbers and light grey operators ~1.7:1), which is what makes the light theme look washed out.
    //So the light theme has its own palette (close to VS Code's Light+) and the dark one keeps the
    //header values, except for the namespace colour (dark green was ~2:1 on the dark editor).
    const bool light = (currentTheme == "Light");
    const wxColour commentColour = light ? wxColour(0, 128, 0)
                                         : wxColour(COMMENT_RED, COMMENT_GREEN, COMMENT_BLUE);
    const wxColour stringColour = light ? wxColour(163, 21, 21)
                                        : wxColour(STRING_RED, STRING_GREEN, STRING_BLUE);
    const wxColour keywordColour = light ? wxColour(0, 0, 255)
                                         : wxColour(KEYWORD_RED, KEYWORD_GREEN, KEYWORD_BLUE);
    const wxColour preprocessorColour = wxColour(PREPROCESSOR_RED, PREPROCESSOR_GREEN, PREPROCESSOR_BLUE);
    const wxColour namespaceColour = light ? wxColour(NAMESPACE_RED, NAMESPACE_GREEN, NAMESPACE_BLUE)
                                           : wxColour(60, 170, 60);
    const wxColour numberColour = light ? wxColour(9, 134, 88)
                                        : wxColour(NUMBER_RED, NUMBER_GREEN, NUMBER_BLUE);
    const wxColour operatorColour = light ? wxColour(64, 64, 64)
                                          : wxColour(OPERATOR_RED, OPERATOR_GREEN, OPERATOR_BLUE);
    const wxColour functionColour = light ? wxColour(200, 0, 170)
                                          : wxColour(FUNCTION_RED, FUNCTION_GREEN, FUNCTION_BLUE);
    const wxColour indentGuideColour = light ? wxColour(210, 210, 210) : wxColour(70, 70, 70);

    wxFont font(FONT_SIZE, FONT_FAMILY, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    //default style
    textCtrl->StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    textCtrl->StyleSetForeground(wxSTC_STYLE_DEFAULT, fg);
    textCtrl->StyleSetBackground(wxSTC_STYLE_DEFAULT, bg);
    textCtrl->StyleClearAll();

    //caret
    textCtrl->SetCaretForeground(fg);

    //selection
    textCtrl->SetSelBackground(true, sel);
    textCtrl->SetSelForeground(true, fg);

    //line numbers
    textCtrl->StyleSetForeground(wxSTC_STYLE_LINENUMBER, lineNumberFg);
    textCtrl->StyleSetBackground(wxSTC_STYLE_LINENUMBER, lineNumberBg);

    //margins
    textCtrl->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    textCtrl->SetMarginWidth(0, 40);
    textCtrl->SetMarginType(1, wxSTC_MARGIN_SYMBOL);
    textCtrl->SetMarginWidth(1, 0);
    for (int i = 0; i < 2; i++)
        textCtrl->SetMarginBackground(i, bg);
    textCtrl->SetFoldMarginColour(true, bg);
    textCtrl->SetFoldMarginHiColour(true, bg);

    //syntax highlighting
    auto setSyntaxStyle = [&](int style, const wxColour& colour) {
        textCtrl->StyleSetFont(style, font);
        textCtrl->StyleSetForeground(style, colour);
        textCtrl->StyleSetBackground(style, bg);
    };
    setSyntaxStyle(STYLE_COMMENT, commentColour);
    setSyntaxStyle(STYLE_STRING, stringColour);
    setSyntaxStyle(STYLE_KEYWORD, keywordColour);
    textCtrl->StyleSetBold(STYLE_KEYWORD, true);
    setSyntaxStyle(STYLE_PREPROCESSOR, preprocessorColour);
    setSyntaxStyle(STYLE_NAMESPACE, namespaceColour);
    setSyntaxStyle(STYLE_NUMBER, numberColour);
    setSyntaxStyle(STYLE_OPERATOR, operatorColour);
    setSyntaxStyle(STYLE_FUNCTION, functionColour);

    //indentation guides. After StyleClearAll they would use the text colour, which is far too loud
    textCtrl->StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, indentGuideColour);
    textCtrl->StyleSetBackground(wxSTC_STYLE_INDENTGUIDE, bg);

    textCtrl->SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
    textCtrl->SetViewEOL(false);
    //caret line highlight. It is configured here (not only in the MainFrame constructor) so that
    //applying a theme later on doesn't leave it switched off
    textCtrl->SetCaretLineBackground(GetCaretLineBackgroundColour());
    textCtrl->SetCaretLineVisible(true);
}