/*
 * wEditor
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ThemeSettings.h"

//set dark or light theme according to config


wxColour ThemeSettings::GetBackgroundColour()
{
    return wxColour(BG_RED, BG_GREEN, BG_BLUE);
}

wxColour ThemeSettings::GetTextColour()
{
    return wxColour(TEXT_RED, TEXT_GREEN, TEXT_BLUE);
}

wxColour ThemeSettings::GetButtonBackgroundColour()
{
    return wxColour(BUTTON_BG_RED, BUTTON_BG_GREEN, BUTTON_BG_BLUE);
}

wxColour ThemeSettings::GetButtonForegroundColour()
{
    return wxColour(BUTTON_FG_RED, BUTTON_FG_GREEN, BUTTON_FG_BLUE);
}

void ThemeSettings::ApplyDarkTheme(wxStyledTextCtrl* textCtrl)
{
    wxColour bg(38, 38, 38);
    wxColour fg(220, 220, 220);

    wxFont font(FONT_SIZE, FONT_FAMILY, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    //default style
    textCtrl->StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    textCtrl->StyleSetForeground(wxSTC_STYLE_DEFAULT, fg);
    textCtrl->StyleSetBackground(wxSTC_STYLE_DEFAULT, bg);

    textCtrl->StyleClearAll(); //apply default style to all styles

    //caret
    textCtrl->SetCaretForeground(fg);

    //selection
    textCtrl->SetSelBackground(true, wxColour(60, 60, 90));
    textCtrl->SetSelForeground(true, fg);

    //linenumbers
    textCtrl->StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColour(120,120,120));
    textCtrl->StyleSetBackground(wxSTC_STYLE_LINENUMBER, wxColour(32, 32, 32));

    //margins
    textCtrl->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    textCtrl->SetMarginType(1, wxSTC_MARGIN_SYMBOL);
    textCtrl->SetMarginWidth(0, 40); 

    for (int i = 0; i < 3; i++)
        textCtrl->SetMarginBackground(i, bg);

    textCtrl->SetFoldMarginColour(true, bg);
    textCtrl->SetFoldMarginHiColour(true, bg);

    //--SYNTAX HIGHLIGHTING STYLES--
    //comment
    textCtrl->StyleSetFont(STYLE_COMMENT, font);
    textCtrl->StyleSetForeground(STYLE_COMMENT,
        wxColour(COMMENT_RED, COMMENT_GREEN, COMMENT_BLUE));
    textCtrl->StyleSetBackground(STYLE_COMMENT, bg);

    //string
    textCtrl->StyleSetFont(STYLE_STRING, font);
    textCtrl->StyleSetForeground(STYLE_STRING,
        wxColour(STRING_RED, STRING_GREEN, STRING_BLUE));
    textCtrl->StyleSetBackground(STYLE_STRING, bg);

    //keyword
    textCtrl->StyleSetFont(STYLE_KEYWORD, font);
    textCtrl->StyleSetForeground(STYLE_KEYWORD,
        wxColour(KEYWORD_RED, KEYWORD_GREEN, KEYWORD_BLUE));
    textCtrl->StyleSetBackground(STYLE_KEYWORD, bg);
    textCtrl->StyleSetBold(STYLE_KEYWORD, true);

    //preprocessor
    textCtrl->StyleSetFont(STYLE_PREPROCESSOR, font);
    textCtrl->StyleSetForeground(STYLE_PREPROCESSOR,
        wxColour(PREPROCESSOR_RED, PREPROCESSOR_GREEN, PREPROCESSOR_BLUE));
    textCtrl->StyleSetBackground(STYLE_PREPROCESSOR, bg);

    //namespace
    textCtrl->StyleSetFont(STYLE_NAMESPACE, font);
    textCtrl->StyleSetForeground(STYLE_NAMESPACE,
        wxColour(NAMESPACE_RED, NAMESPACE_GREEN, NAMESPACE_BLUE));
    textCtrl->StyleSetBackground(STYLE_NAMESPACE, bg);

    //number
    textCtrl->StyleSetFont(STYLE_NUMBER, font);
    textCtrl->StyleSetForeground(STYLE_NUMBER,
        wxColour(NUMBER_RED, NUMBER_GREEN, NUMBER_BLUE));
    textCtrl->StyleSetBackground(STYLE_NUMBER, bg);

    //operator
    textCtrl->StyleSetFont(STYLE_OPERATOR, font);
    textCtrl->StyleSetForeground(STYLE_OPERATOR,
        wxColour(OPERATOR_RED, OPERATOR_GREEN, OPERATOR_BLUE));
    textCtrl->StyleSetBackground(STYLE_OPERATOR, bg);

    //function
    textCtrl->StyleSetFont(STYLE_FUNCTION, font);
    textCtrl->StyleSetForeground(STYLE_FUNCTION,
        wxColour(FUNCTION_RED, FUNCTION_GREEN, FUNCTION_BLUE));
    textCtrl->StyleSetBackground(STYLE_FUNCTION, bg);

    //misc
    textCtrl->SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
    textCtrl->SetViewEOL(false);
    textCtrl->SetCaretLineVisible(false);
    textCtrl->SetWrapMode(wxSTC_WRAP_WORD);
}
