#include <weditor/SyntaxHighlighter/CMakeHighlight.h>
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> s_cmakeCommands = {
    "add_executable", "add_library", "add_subdirectory", "add_dependencies", "add_custom_command",
    "add_custom_target", "add_compile_definitions", "add_compile_options", "add_definitions",
    "add_test", "aux_source_directory", "cmake_minimum_required", "configure_file", "create_test_sourcelist",
    "define_property", "enable_language", "enable_testing", "execute_process", "file", "find_package",
    "find_library", "find_program", "find_path", "find_file", "install", "include", "include_directories",
    "include_guard", "link_directories", "link_libraries", "list", "message", "option", "project",
    "set", "set_directory_properties", "set_property", "set_source_files_properties",
    "set_target_properties", "source_group", "string", "target_compile_definitions",
    "target_compile_features", "target_compile_options", "target_include_directories",
    "target_link_libraries", "target_sources", "try_compile", "try_run", "write_file"
};

static const std::unordered_set<std::string> s_cmakeFlow = {
    "if", "else", "elseif", "endif", "foreach", "endforeach", "while", "endwhile",
    "function", "endfunction", "macro", "endmacro", "break", "continue", "return"
};

static const std::unordered_set<std::string> s_cmakePolicies = {
    "cmake_policy", "cmake_minimum_required", "policy", "cmake_host_system_information",
    "cmake_parse_arguments"
};

static const std::unordered_set<std::string> s_cmakeProperties = {
    "get_property", "set_property", "get_target_property", "get_directory_property",
    "get_source_file_property", "set_directory_properties", "set_target_properties",
    "set_source_files_properties", "define_property"
};

//helper functions for tokenization
static bool IsIdentChar(char c)  { return std::isalnum((unsigned char)c) || c == '_' || c == '$'; }
static bool IsIdentStart(char c) { return std::isalpha((unsigned char)c) || c == '_' || c == '$'; }

void CMakeHighlight::ApplyHighlight(wxStyledTextCtrl* textCtrl)
{
    textCtrl->ClearDocumentStyle();
    textCtrl->SetLexer(wxSTC_LEX_NULL);

    const wxString wxText = textCtrl->GetValue();
    if (wxText.empty()) return;

    //work with std::string for easier processing
    const std::string text = wxText.ToStdString();
    const int len = static_cast<int>(text.size());

    std::string styles(len, STYLE_DEFAULT);

    auto setStyle = [&](int from, int to, char style) {
        for (int j = from; j < to && j < len; ++j)
            styles[j] = style;
    };

    int i = 0;
    while (i < len)
    {
        const char c = text[i];
        const char c1 = (i + 1 < len) ? text[i + 1] : '\0';

        // line comment
        if (c == '#')
        {
            const int start = i;
            while (i < len && text[i] != '\n') ++i;
            setStyle(start, i, STYLE_COMMENT);
            continue;
        }

        // string literal
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

        // generator expressions and variables: ${...}, $<...>, $ENV{...}
        if (c == '$')
        {
            const int start = i;
            ++i;
            if (i < len && text[i] == '<')
            {
                ++i;
                while (i < len && text[i] != '>') ++i;
                if (i < len && text[i] == '>') ++i;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }
            if (i < len && (text[i] == '{' || text[i] == '('))
            {
                const char close = (text[i] == '{') ? '}' : ')';
                ++i;
                while (i < len && text[i] != close) ++i;
                if (i < len && text[i] == close) ++i;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }
            if (i < len && std::isalpha((unsigned char)text[i]))
            {
                while (i < len && IsIdentChar(text[i])) ++i;
                setStyle(start, i, STYLE_PREPROCESSOR);
                continue;
            }
            setStyle(start, i, STYLE_PREPROCESSOR);
            continue;
        }

        // numbers
        if (std::isdigit((unsigned char)c) || (c == '.' && std::isdigit((unsigned char)c1)))
        {
            const int start = i;
            if (c == '0' && (c1 == 'x' || c1 == 'X'))
            {
                i += 2;
                while (i < len && std::isxdigit((unsigned char)text[i])) ++i;
            }
            else
            {
                while (i < len && (std::isdigit((unsigned char)text[i]) || text[i] == '.')) ++i;
            }
            setStyle(start, i, STYLE_NUMBER);
            continue;
        }

        if (IsIdentStart(c))
        {
            const int start = i;
            while (i < len && IsIdentChar(text[i])) ++i;
            const std::string word = text.substr(start, i - start);
            std::string lower = word;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return std::tolower(ch); });

            int j = i;
            while (j < len && (text[j] == ' ' || text[j] == '\t')) ++j;
            if (j < len && text[j] == '(')
            {
                if (s_cmakeFlow.count(lower))
                    setStyle(start, i, STYLE_KEYWORD);
                else if (s_cmakeCommands.count(lower) || s_cmakePolicies.count(lower) || s_cmakeProperties.count(lower))
                    setStyle(start, i, STYLE_FUNCTION);
                else
                    setStyle(start, i, STYLE_FUNCTION);
                continue;
            }

            if (s_cmakeFlow.count(lower))
            {
                setStyle(start, i, STYLE_KEYWORD);
                continue;
            }

            if (s_cmakeCommands.count(lower) || s_cmakePolicies.count(lower) || s_cmakeProperties.count(lower))
            {
                setStyle(start, i, STYLE_FUNCTION);
                continue;
            }

            continue;
        }

        ++i;
    }

    //apply styles at once
    textCtrl->StartStyling(0);
    textCtrl->SetStyleBytes(len, styles.data());
}

