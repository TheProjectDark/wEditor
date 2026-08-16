#include <weditor/SyntaxHighlighter/MakefileHighlight.h>
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> s_makefileDirectives = {
    "all", "clean", "install", "uninstall", "test", "check", "distclean",
    ".PHONY", ".SUFFIXES", ".DEFAULT", ".PRECIOUS", ".INTERMEDIATE",
    ".SECONDARY", ".SECONDEXPANSION", ".DELETE_ON_ERROR", ".IGNORE",
    ".LOW_RESOLUTION_TIME", ".SILENT", ".EXPORT_ALL_VARIABLES"
};

static const std::unordered_set<std::string> s_makefileKeywords = {
    "include", "define", "endef", "ifeq", "ifneq", "ifdef", "ifndef",
    "else", "endif", "override", "export", "unexport", "vpath",
    "error", "warning"
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

    //first pass: comments and quoted strings (so they override other rules)
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
                if (text[i] == '\\') { i += 2; continue; }
                if (text[i] == delim) { ++i; break; }
                ++i;
            }
            setStyle(start, i, STYLE_STRING);
            continue;
        }
        ++i;
    }

    //second pass: line-oriented rules (targets, assignments, directives, variable expansions)
    int lineStart = 0;
    while (lineStart < len)
    {
        int lineEnd = lineStart;
        while (lineEnd < len && text[lineEnd] != '\n') ++lineEnd;

        //skip empty lines
        if (lineStart >= lineEnd) { lineStart = lineEnd + 1; continue; }

        //if line starts with tab -> recipe/command
        if (text[lineStart] == '\t')
        {
            setStyle(lineStart, lineEnd, STYLE_FUNCTION);
            //still highlight variable expansions inside the command
        }

        //find first non-space
        int p = lineStart;
        while (p < lineEnd && (text[p] == ' ' || text[p] == '\t')) ++p;
        if (p >= lineEnd) { lineStart = lineEnd + 1; continue; }

        //skip comments
        if (text[p] == '#') { lineStart = lineEnd + 1; continue; }

        //extract first word (allow '.' at start for dot-directives)
        int w = p;
        if (text[w] == '.') ++w;
        while (w < lineEnd && IsIdentChar(text[w])) ++w;
        const std::string firstWord = text.substr(p, w - p);

        //dot-directives like .PHONY etc.
        if (!firstWord.empty() && firstWord[0] == '.' && s_makefileDirectives.count(firstWord))
        {
            setStyle(p, w, STYLE_KEYWORD);
        }
        else if (s_makefileKeywords.count(firstWord))
        {
            //include/define/ifdef/ifeq etc.
            setStyle(p, w, STYLE_PREPROCESSOR);
        }
        else
        {
            //check for variable assignment: name [+:?]?=  
            int q = w;
            while (q < lineEnd && (text[q] == ' ' || text[q] == '\t')) ++q;
            bool isAssignment = false;
            if (q < lineEnd)
            {
                if (text[q] == '=') isAssignment = true;
                else if ((text[q] == ':' || text[q] == '+' || text[q] == '?') && (q + 1 < lineEnd) && text[q + 1] == '=') isAssignment = true;
            }
            if (isAssignment && IsIdentStart(text[p]))
            {
                setStyle(p, w, STYLE_NAMESPACE); //variable name
            }
            else
            {
                //check for target: find ':' before any '=' on the same line
                int colonPos = -1;
                for (int k = p; k < lineEnd; ++k) { if (text[k] == ':') { colonPos = k; break; } if (text[k] == '=') break; }
                if (colonPos != -1 && IsIdentStart(text[p]))
                {
                    setStyle(p, colonPos, STYLE_FUNCTION);
                }
            }
        }

        //variable expansions anywhere in the line: $VAR, ${VAR}, $(cmd), $<, $@, $?
        int j = lineStart;
        while (j < lineEnd)
        {
            if (text[j] == '$')
            {
                int start = j;
                const char c1 = (j + 1 < len) ? text[j + 1] : '\0';
                if (c1 == '{')
                {
                    j += 2; int depth = 1;
                    while (j < lineEnd && depth > 0)
                    {
                        if (text[j] == '{') ++depth;
                        else if (text[j] == '}') --depth;
                        ++j;
                    }
                    setStyle(start, j, STYLE_PREPROCESSOR);
                    continue;
                }
                if (c1 == '(')
                {
                    j += 2; int depth = 1;
                    while (j < lineEnd && depth > 0)
                    {
                        if (text[j] == '(') ++depth;
                        else if (text[j] == ')') --depth;
                        ++j;
                    }
                    setStyle(start, j, STYLE_PREPROCESSOR);
                    continue;
                }
                if (std::isdigit((unsigned char)c1))
                {
                    ++j; while (j < lineEnd && std::isdigit((unsigned char)text[j])) ++j;
                    setStyle(start, j, STYLE_PREPROCESSOR);
                    continue;
                }
                if (c1 == '@' || c1 == '*' || c1 == '#' || c1 == '?' || c1 == '<' || c1 == '^' || c1 == '+')
                {
                    j += 2; setStyle(start, j, STYLE_PREPROCESSOR); continue;
                }
                if (IsIdentStart(c1))
                {
                    j += 2; while (j < lineEnd && IsIdentChar(text[j])) ++j;
                    setStyle(start, j, STYLE_PREPROCESSOR); continue;
                }
            }
            ++j;
        }

        lineStart = lineEnd + 1;
    }

    //apply styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}