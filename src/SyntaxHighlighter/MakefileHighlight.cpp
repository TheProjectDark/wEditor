#include <weditor/SyntaxHighlighter/MakefileHighlight.h>
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> s_makefileDirectives = {
    "all", "clean", "install", "uninstall", "test", "check", "distclean",
    ".PHONY", ".SUFFIXES", ".DEFAULT", ".PRECIOUS", ".INTERMEDIATE",
    ".SECONDARY", ".SECONDEXPANSION", ".DELETE_ON_ERROR", ".IGNORE",
    ".LOW_RESOLUTION_TIME", ".SILENT", ".EXPORT_ALL_VARIABLES"
};

//helper functions for tokenization
static bool IsIdentChar(char c)  { return std::isalnum((unsigned char)c) || c == '_' || c == '$' || c == '.' || c == '-'; };
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_' || c == '$' || c == '.' || c == '-'; };

//basic syntax highlighting for Makefile
void MakefileHighlight::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);

    const wxString wxText = textCtrl->GetValue();
    if (wxText.empty()) return;
    const std::string text = wxText.ToStdString();
    const int len = static_cast<int>(text.size());

    std::string styles(len, STYLE_DEFAULT);

    auto setStyle = [&](int from, int to, char style) {
        for (int i = from; i < to && i < len; ++i)
            styles[i] = style;
    };

    int i = 0;
    while (i < len)
    {
        const char c  = text[i];
        const char c1 = (i + 1 < len) ? text[i + 1] : '\0';
        

        //line comment
        if (c == '#')
        {
            const int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        //string literal
        if (c == '"' || c == '\'')
        {
            const int start = i;
            const char delim = c;
            ++i;
            while (i < len)
            {
                if (text[i] == '\\')
                {
                    i += 2;
                    continue;
                }
                if (text[i] == delim)
                {
                    ++i;
                    break;
                }
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }
        ++i;
    }

    //apply styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}