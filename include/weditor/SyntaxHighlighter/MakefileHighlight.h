#pragma once
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include "SyntaxHighlighter.h"

class MakefileHighlight : public SyntaxHighlighter {
    void ApplyHighlight(wxStyledTextCtrl* textCtrl) override;
    wxString GetLanguageName() const override { return "Makefile"; }
};