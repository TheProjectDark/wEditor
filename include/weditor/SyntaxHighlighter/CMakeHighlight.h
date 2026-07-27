#pragma once
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include "SyntaxHighlighter.h"

class CMakeHighlight : public SyntaxHighlighter {
    void ApplyHighlight(wxStyledTextCtrl* textCtrl) override;
    wxString GetLanguageName() const override { return "CMake"; }
};