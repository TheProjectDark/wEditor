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

    wxFont font(FONT_SIZE, FONT_FAMILY, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    // default style
    textCtrl->StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    textCtrl->StyleSetForeground(wxSTC_STYLE_DEFAULT, fg);
    textCtrl->StyleSetBackground(wxSTC_STYLE_DEFAULT, bg);
    textCtrl->StyleClearAll();

    // caret
    textCtrl->SetCaretForeground(fg);

    // selection
    textCtrl->SetSelBackground(true, sel);
    textCtrl->SetSelForeground(true, fg);

    // line numbers
    textCtrl->StyleSetForeground(wxSTC_STYLE_LINENUMBER, lineNumberFg);
    textCtrl->StyleSetBackground(wxSTC_STYLE_LINENUMBER, lineNumberBg);

    // margins
    textCtrl->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    textCtrl->SetMarginWidth(0, 40);
    textCtrl->SetMarginType(1, wxSTC_MARGIN_SYMBOL);
    textCtrl->SetMarginWidth(1, 0);
    for (int i = 0; i < 2; i++)
        textCtrl->SetMarginBackground(i, bg);
    textCtrl->SetFoldMarginColour(true, bg);
    textCtrl->SetFoldMarginHiColour(true, bg);

    // syntax highlighting
    textCtrl->StyleSetFont(STYLE_COMMENT, font);
    textCtrl->StyleSetForeground(STYLE_COMMENT,
        wxColour(COMMENT_RED, COMMENT_GREEN, COMMENT_BLUE));
    textCtrl->StyleSetBackground(STYLE_COMMENT, bg);

    textCtrl->StyleSetFont(STYLE_STRING, font);
    textCtrl->StyleSetForeground(STYLE_STRING,
        wxColour(STRING_RED, STRING_GREEN, STRING_BLUE));
    textCtrl->StyleSetBackground(STYLE_STRING, bg);

    textCtrl->StyleSetFont(STYLE_KEYWORD, font);
    textCtrl->StyleSetForeground(STYLE_KEYWORD,
        wxColour(KEYWORD_RED, KEYWORD_GREEN, KEYWORD_BLUE));
    textCtrl->StyleSetBackground(STYLE_KEYWORD, bg);
    textCtrl->StyleSetBold(STYLE_KEYWORD, true);

    textCtrl->StyleSetFont(STYLE_PREPROCESSOR, font);
    textCtrl->StyleSetForeground(STYLE_PREPROCESSOR,
        wxColour(PREPROCESSOR_RED, PREPROCESSOR_GREEN, PREPROCESSOR_BLUE));
    textCtrl->StyleSetBackground(STYLE_PREPROCESSOR, bg);

    textCtrl->StyleSetFont(STYLE_NAMESPACE, font);
    textCtrl->StyleSetForeground(STYLE_NAMESPACE,
        wxColour(NAMESPACE_RED, NAMESPACE_GREEN, NAMESPACE_BLUE));
    textCtrl->StyleSetBackground(STYLE_NAMESPACE, bg);

    textCtrl->StyleSetFont(STYLE_NUMBER, font);
    textCtrl->StyleSetForeground(STYLE_NUMBER,
        wxColour(NUMBER_RED, NUMBER_GREEN, NUMBER_BLUE));
    textCtrl->StyleSetBackground(STYLE_NUMBER, bg);

    textCtrl->StyleSetFont(STYLE_OPERATOR, font);
    textCtrl->StyleSetForeground(STYLE_OPERATOR,
        wxColour(OPERATOR_RED, OPERATOR_GREEN, OPERATOR_BLUE));
    textCtrl->StyleSetBackground(STYLE_OPERATOR, bg);

    textCtrl->StyleSetFont(STYLE_FUNCTION, font);
    textCtrl->StyleSetForeground(STYLE_FUNCTION,
        wxColour(FUNCTION_RED, FUNCTION_GREEN, FUNCTION_BLUE));
    textCtrl->StyleSetBackground(STYLE_FUNCTION, bg);

    textCtrl->SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
    textCtrl->SetViewEOL(false);
    textCtrl->SetCaretLineVisible(false);
}