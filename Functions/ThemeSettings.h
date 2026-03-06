/*
 * wEditor Theme Settings
 * Copyright (C) 2026 TheProjectDark
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include "../SyntaxHighlighter/SyntaxHighlighter.h"

class ThemeSettings
{
public:
    //dark theme colors
    static constexpr int BG_RED = 30;
    static constexpr int BG_GREEN = 30;
    static constexpr int BG_BLUE = 30;
    
    static constexpr int TEXT_RED = 230;
    static constexpr int TEXT_GREEN = 230;
    static constexpr int TEXT_BLUE = 230;
    
    static constexpr int BUTTON_BG_RED = 110;
    static constexpr int BUTTON_BG_GREEN = 110;
    static constexpr int BUTTON_BG_BLUE = 110;

    static constexpr int BUTTON_FG_RED = 255;
    static constexpr int BUTTON_FG_GREEN = 255;
    static constexpr int BUTTON_FG_BLUE = 255;
    
    //font settings
    static constexpr int FONT_SIZE = 14;
    static constexpr wxFontFamily FONT_FAMILY = wxFONTFAMILY_TELETYPE;
    
    //comment color (green)
    static constexpr int COMMENT_RED = 128;
    static constexpr int COMMENT_GREEN = 255;
    static constexpr int COMMENT_BLUE = 170;
    
    //string color (orange)
    static constexpr int STRING_RED = 255;
    static constexpr int STRING_GREEN = 140;
    static constexpr int STRING_BLUE = 0;
    
    //keyword color (blue)
    static constexpr int KEYWORD_RED = 86;
    static constexpr int KEYWORD_GREEN = 156;
    static constexpr int KEYWORD_BLUE = 214;
    
    //preprocessor color (cyan)
    static constexpr int PREPROCESSOR_RED = 0;
    static constexpr int PREPROCESSOR_GREEN = 128;
    static constexpr int PREPROCESSOR_BLUE = 128;
    
    //namespace color (dark green)
    static constexpr int NAMESPACE_RED = 0;
    static constexpr int NAMESPACE_GREEN = 100;
    static constexpr int NAMESPACE_BLUE = 0;
    
    //number color (light green)
    static constexpr int NUMBER_RED = 181;
    static constexpr int NUMBER_GREEN = 206;
    static constexpr int NUMBER_BLUE = 168;
    
    //operator color (light gray)
    static constexpr int OPERATOR_RED = 200;
    static constexpr int OPERATOR_GREEN = 200;
    static constexpr int OPERATOR_BLUE = 200;
    
    //function color (light yellow)
    static constexpr int FUNCTION_RED = 220;
    static constexpr int FUNCTION_GREEN = 220;
    static constexpr int FUNCTION_BLUE = 170;
    
    static void ApplyDarkTheme(wxStyledTextCtrl* textCtrl);
    static wxColour GetBackgroundColour();
    static wxColour GetTextColour();
    static wxColour GetButtonBackgroundColour();
    static wxColour GetButtonForegroundColour();
};
