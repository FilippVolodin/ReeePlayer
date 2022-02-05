#include "pch.h"
#include "webvtt.h"
#include "exception.h"
#include "parser_utils.h"
#include "html.h"

namespace qsubs::webvtt
{
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;

    ISubtitlesPtr parse(const QString& filename)
    {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
            return nullptr;

        QTextStream stream(&file);
        Parser parser(stream);
        return parser.parse();
    }

    constexpr QChar NAME_VALUE_SEPARATOR = ':';
    constexpr QStringView ARROW = u"-->";

    void parse_cue_settings(QStringView str, qsizetype& pos, Cue*)
    {
        for (QStringView setting : split_on_spaces(remainder(str, pos)))
        {
            if (!setting.contains(NAME_VALUE_SEPARATOR) ||
                setting.first() == NAME_VALUE_SEPARATOR ||
                setting.last() == NAME_VALUE_SEPARATOR)
                continue;

            qsizetype sep_pos = setting.indexOf(NAME_VALUE_SEPARATOR);
            QStringView name = setting.left(sep_pos);
            QStringView value = setting.mid(sep_pos + 1);
            if (name == u"region")
            {
                // TODO
            }
            else if (name == u"vertical")
            {
                // TODO
            }
            else if (name == u"line")
            {
                // TODO
            }
            else if (name == u"position")
            {
                // TODO
            }
            else if (name == u"size")
            {
                // TODO
            }
            else if (name == u"align")
            {
                // TODO
            }
        }
    }

    bool collect_cue(QStringView str, Cue* cue)
    {
        qsizetype pos = 0;
        skip_whispaces(str, pos);

        try
        {
            cue->set_start_time(collect_timestamp(str, pos));
        }
        catch (Exception& e)
        {
            return false;
        }
        skip_whispaces(str, pos);

        for (QChar c : ARROW)
        {
            if (current(str, pos) != c)
                throw Exception();
            ++pos;
        }

        skip_whispaces(str, pos);
        try
        {
            cue->set_end_time(collect_timestamp(str, pos));
        }
        catch (Exception& e)
        {
            return false;
        }
        parse_cue_settings(str, pos, cue);
        
        // TODO
        return true;
    }

    int Cue::get_index() const
    {
        return m_index;
    }

    Timestamp Cue::get_start_time() const
    {
        return m_start_time;
    }

    Timestamp Cue::get_end_time() const
    {
        return m_end_time;
    }

    QString Cue::get_text() const
    {
        return m_text;
    }

    void Cue::set_index(int index)
    {
        m_index = index;
    }

    void Cue::set_start_time(Timestamp start_time)
    {
        m_start_time = start_time;
    }

    void Cue::set_end_time(Timestamp end_time)
    {
        m_end_time = end_time;
    }

    void Cue::set_text(QString text)
    {
        m_text = text;
    }

    Parser::Parser(QTextStream& stream)
        : m_input(stream)
    {
    }

    std::shared_ptr<Subtitles> Parser::parse()
    {
        m_seen_cue = false;

        if (m_input.atEnd())
            return nullptr;

        // Little work-around from the standard
        // first line must be non-empty
        //m_line = next_non_empty_line(m_input);
        m_line = m_input.readLine();

        if (m_line.startsWith(u"WEBVTT"))
        {
            if (m_line.length() > 6 && !(m_line[6] == QChar::Space || m_line[6] == QChar::Tabulation))
                return nullptr;

            m_line = m_input.readLine();

            if (!m_line.isEmpty())
                collect_block(true);
            else
                m_line = m_input.readLine();

            if (m_line.isEmpty())
                m_line = next_non_empty_line(m_input);
        }
        else if (!is_number(m_line)) 
        {
            // not SubRip
            return nullptr;
        }

        m_subtitles = std::make_shared<Subtitles>();
        while (!m_input.atEnd())
        {
            BlockOpt block = collect_block(false);
            if (block)
            {
                std::visit(overloaded{
                    [this](Cue* arg) { m_subtitles->m_cues.push_back(arg); },
                    [this](Region* arg) { m_subtitles->m_regions.push_back(arg); },
                    [this](StyleSheet* arg) { m_subtitles->m_stylesheets.push_back(arg); },
                    }, block.value());
            }
            m_line = next_non_empty_line(m_input);
        }
        
        std::sort(m_subtitles->m_cues.begin(), m_subtitles->m_cues.end(), [](const ICue* l, const ICue* r)
            {
                return l->get_start_time() < r->get_start_time();
            });

        QRegularExpression br_re("<br\\s*\\/>");
        QRegularExpression tag_re("<[^>]*>");

        for (int i = 0; i < m_subtitles->m_cues.size(); ++i)
        {
            m_subtitles->m_cues[i]->set_index(i);

            // HACK
            QString text = m_subtitles->m_cues[i]->get_text();
            text.replace(br_re, "\r\n");
            text.remove(tag_re);
            m_subtitles->m_cues[i]->set_text(text);

            //std::shared_ptr<NodeObjects> node_objects = parse_cue_text(m_subtitles->m_cues[i]->get_text());
        }

        return m_subtitles;
    }

    BlockOpt Parser::collect_block(bool in_header)
    {
        int line_count = 0;
        QString buffer;
        bool seen_arrow = false;

        Cue* cue = nullptr;
        Region* region = nullptr;
        StyleSheet* stylesheet = nullptr;

        bool break_loop = false;
        while (!m_input.atEnd() && !break_loop)
        {
            ++line_count;

            if (m_line.isEmpty())
            {
                break_loop = true;
                // CHECK m_line = m_input.readLine();
            }
            else if (m_line.contains(ARROW))
            {
                if (!in_header && (line_count == 1 || line_count == 2 && !seen_arrow))
                {
                    seen_arrow = true;
                    cue = new Cue();
                    if (collect_cue(m_line, cue))
                    {
                        buffer.clear();
                        m_seen_cue = true;
                    }
                    else
                    {
                        delete cue;
                        cue = nullptr;
                    }
                    m_line = m_input.readLine();
                }
                else
                {
                    break_loop = true;
                }
            }
            else
            {
                if (!in_header && line_count == 2)
                {
                    if (!m_seen_cue && trim_right(buffer) == u"STYLE")
                    {
                        stylesheet = new StyleSheet();
                        buffer.clear();
                    }
                    else if (!m_seen_cue && trim_right(buffer) == u"REGION")
                    {
                        region = new Region();
                        buffer.clear();
                    }
                }

                if (!buffer.isEmpty())
                    buffer.append(QChar::LineFeed);

                // VTT spec doesn't contain this condition. Added for srt compatibility (skip index)
                if(seen_arrow)
                    buffer.append(m_line);
                m_line = m_input.readLine();
            }
        }

        if (cue != nullptr)
        {
            cue->set_text(buffer);
            return cue;
        }
        else if (stylesheet != nullptr)
            return stylesheet;
        else if (region != nullptr)
            return region;
        else
            return std::nullopt;
    }

    QString collapse_ws_sequences(QStringView str)
    {
        QString res;
        QStringView trimmed = trim(str);
        bool prev_is_non_ws = true;
        for (QChar c : trimmed)
        {
            bool is_non_ws = !is_whitespace(c);
            if (is_non_ws)
                res.push_back(c);
            else if (prev_is_non_ws)
                res.push_back(QChar::Space);
            prev_is_non_ws = is_non_ws;
        }
        return res;
    }

    struct Token
    {
        enum class Type { String, StartTag, EndTag, TimestampTag };

        Type type = Type::String;
        QString value;
        std::vector<QString> classes;
        QString annotation;
    };

    class Tokenizer
    {
    public:
        Tokenizer(QStringView str, qsizetype& pos);
        Token next_token();
    private:
        void append_to_value(Token&);
        void flush_value(Token&);

        void read_if_data_state(Token&);
        void read_token_if_html_char_ref_in_data_state(Token&);
        void read_if_tag_state(Token&);
        void read_if_start_tag_state(Token&);
        void read_if_start_tag_class_state(Token&);
        void read_if_start_tag_annotation_state(Token&);
        void read_if_html_char_ref_in_annotation_state(Token&);
        void read_if_end_tag_state(Token&);
        void read_if_timestamp_tag_state(Token&);

        enum class TokenizerState {
            Data, HTMLCharRefInData, Tag, StartTag, StartTagClass,
            StartTagAnnotation, HTMLCharRefInAnnotation, EndTag, TimestampTag,
        };

        QStringView m_str;
        qsizetype& m_pos;
        QString m_buffer;
        QChar m_c;
        TokenizerState m_state = TokenizerState::Data;
        bool m_at_eol = false;
        bool m_token_is_read = false;
        qsizetype m_value_start = -1;
        qsizetype m_value_end = -1;
    };

    Tokenizer::Tokenizer(QStringView str, qsizetype& pos)
        : m_str(str), m_pos(pos)
    {
    }

    void Tokenizer::append_to_value(Token& token)
    {
        if (m_value_start == -1)
        {
            m_value_start = m_pos;
            m_value_end = m_pos;
        }
        else
        {
            if (m_pos == m_value_end + 1)
            {
                m_value_end = m_pos;
            }
            else if (m_pos > m_value_end + 1)
            {
                token.value.append(m_str.mid(m_value_start, m_value_end - m_value_start + 1));
                m_value_start = m_pos;
                m_value_end = m_pos;
            }
        }
    }

    void Tokenizer::flush_value(Token& token)
    {
        if (m_value_start != -1)
        {
            token.value.append(m_str.mid(m_value_start, m_value_end - m_value_start + 1));
            m_value_start = -1;
        }
    }

    void Tokenizer::read_if_data_state(Token& token)
    {
        if (m_at_eol)
        {
            token.type = Token::Type::String;
            m_token_is_read = true;
        }
        else if (m_c == '&')
        {
            m_state = TokenizerState::HTMLCharRefInData;
        }
        else if (m_c == '<')
        {
            if (m_value_start == -1)
            {
                m_state = TokenizerState::Tag;
            }
            else
            {
                token.type = Token::Type::String;
                m_token_is_read = true;
            }
        }
        else
        {
            append_to_value(token);
        }
    }

    void Tokenizer::read_token_if_html_char_ref_in_data_state(Token& token)
    {
        QString char_ref = html::consume_char_ref(m_str, m_pos);
        if (char_ref.isEmpty())
            token.value.push_back('&');
        else
            token.value.append(char_ref);
        m_state = TokenizerState::Data;
    }

    void Tokenizer::read_if_tag_state(Token& token)
    {
        if (m_at_eol)
        {
            token.type = Token::Type::StartTag;
            m_token_is_read = true;
            return;
        }

        switch (m_c.unicode())
        {
        case '\t': case '\r': case '\f': case ' ':
            m_state = TokenizerState::StartTagAnnotation;
            break;
        case '.':
            m_state = TokenizerState::StartTag;
            break;
        case '/':
            m_state = TokenizerState::EndTag;
            break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            if (m_value_start == -1)
                m_value_start = m_pos;
            m_state = TokenizerState::TimestampTag;
            break;
        case '>':
            ++m_pos;
            token.type = Token::Type::StartTag;
            m_token_is_read = true;
            break;
        default:
            append_to_value(token);
            m_state = TokenizerState::StartTag;
        }
    }

    void Tokenizer::read_if_start_tag_state(Token& token)
    {
        if (m_at_eol)
        {
            token.type = Token::Type::StartTag;
            m_token_is_read = true;
            return;
        }

        switch (m_c.unicode())
        {
        case '\t': case '\f': case ' ':
            m_state = TokenizerState::StartTagAnnotation;
            break;
        case '\r':
            m_buffer = m_c;
            m_state = TokenizerState::StartTagAnnotation;
            break;
        case '.':
            m_state = TokenizerState::StartTagClass;
            break;
        case '>':
            ++m_pos;
            token.type = Token::Type::StartTag;
            m_token_is_read = true;
            break;
        default:
            append_to_value(token);
        }
    }

    void Tokenizer::read_if_start_tag_class_state(Token& token)
    {
        if (m_at_eol)
        {
            token.type = Token::Type::StartTag;
            token.classes.push_back(m_buffer);
            m_token_is_read = true;
            return;
        }

        switch (m_c.unicode())
        {
        case '\t': case '\f': case ' ':
        {
            token.classes.push_back(m_buffer);
            m_buffer.clear();
            m_state = TokenizerState::StartTagAnnotation;
            break;
        }
        case '\r':
        {
            token.classes.push_back(m_buffer);
            m_buffer = m_c;
            m_state = TokenizerState::StartTagAnnotation;
            break;
        }
        case '.':
        {
            token.classes.push_back(m_buffer);
            m_buffer.clear();
            break;
        }
        case '>':
        {
            ++m_pos;
            token.type = Token::Type::StartTag;
            token.classes.push_back(m_buffer);
            m_token_is_read = true;
            break;
        }
        default:
            m_buffer.append(m_c);
        }
    }

    void Tokenizer::read_if_start_tag_annotation_state(Token& token)
    {
        if (m_at_eol)
        {
            m_buffer = collapse_ws_sequences(m_buffer);
            token.type = Token::Type::StartTag;
            token.annotation = m_buffer;
            m_token_is_read = true;
            return;
        }

        switch (m_c.unicode())
        {
        case '&':
        {
            m_state = TokenizerState::HTMLCharRefInAnnotation;
            break;
        }
        case '>':
        {
            ++m_pos;
            m_buffer = collapse_ws_sequences(m_buffer);
            token.type = Token::Type::StartTag;
            token.annotation = m_buffer;
            m_token_is_read = true;
            break;
        }
        default:
            m_buffer.append(m_c);
        }
    }

    void Tokenizer::read_if_html_char_ref_in_annotation_state(Token&)
    {
        QString char_ref = html::consume_char_ref(m_str, m_pos, { '>' });
        if (char_ref.isEmpty())
            m_buffer.append('&');
        else
            m_buffer.append(char_ref);
        m_state = TokenizerState::StartTagAnnotation;
    }

    void Tokenizer::read_if_end_tag_state(Token& token)
    {
        if (m_at_eol)
        {
            token.type = Token::Type::EndTag;
            m_token_is_read = true;
        }
        else if (m_c == '>')
        {
            ++m_pos;
            token.type = Token::Type::EndTag;
            m_token_is_read = true;
        }
        else
        {
            append_to_value(token);
        }
    }

    void Tokenizer::read_if_timestamp_tag_state(Token& token)
    {
        if (m_at_eol)
        {
            token.type = Token::Type::TimestampTag;
            m_token_is_read = true;
        }
        else if (m_c == '>')
        {
            ++m_pos;
            token.type = Token::Type::TimestampTag;
            m_token_is_read = true;
        }
        else
        {
            append_to_value(token);
        }
    }

    Token Tokenizer::next_token()
    {
        Token token;
        m_token_is_read = false;
        m_buffer.clear();
        m_state = TokenizerState::Data;
        m_at_eol = false;
        m_value_start = -1;
        do
        {
            m_at_eol = end_of_line(m_str, m_pos);
            if (!m_at_eol)
                m_c = current(m_str, m_pos);

            switch (m_state)
            {
            case TokenizerState::Data: read_if_data_state(token); break;
            case TokenizerState::HTMLCharRefInData: read_token_if_html_char_ref_in_data_state(token); break;
            case TokenizerState::Tag: read_if_tag_state(token); break;
            case TokenizerState::StartTag: read_if_start_tag_state(token); break;
            case TokenizerState::StartTagClass: read_if_start_tag_class_state(token); break;
            case TokenizerState::StartTagAnnotation: read_if_start_tag_annotation_state(token); break;
            case TokenizerState::HTMLCharRefInAnnotation: read_if_html_char_ref_in_annotation_state(token); break;
            case TokenizerState::EndTag: read_if_end_tag_state(token); break;
            case TokenizerState::TimestampTag: read_if_timestamp_tag_state(token); break;
            }
            if (!m_token_is_read)
                ++m_pos;
        } while (!m_at_eol && !m_token_is_read);

        flush_value(token);

        return token;
    }

    Token next_token(QStringView str, qsizetype& pos)
    {
        return Tokenizer(str, pos).next_token();
    }

    class CueTextParser
    {
    public:
        CueTextParser(QStringView str) : m_str(str) {}
        std::shared_ptr<NodeObjects> parse();
    private:
        void attach(const Token& token, InternalNodeObject* obj);

        QStringView m_str;
        qsizetype m_pos = 0;
        std::vector<QString> m_lang_stack;
        InternalNodeObject* m_current = nullptr;
    };

    std::shared_ptr<NodeObjects> CueTextParser::parse()
    {
        NodeObjects* result = new NodeObjects();
        m_current = result;

        while (!end_of_line(m_str, m_pos))
        {
            Token token = next_token(m_str, m_pos);
            switch (token.type)
            {
            case Token::Type::String:
            {
                TextObject* obj = new TextObject();
                obj->m_value = token.value;
                m_current->children.push_back(obj);
                break;
            }
            case Token::Type::StartTag:
            {
                if (token.value == "c")
                {
                    attach(token, new ClassObject());
                }
                else if (token.value == "i")
                {
                    attach(token, new ItalicObject());
                }
                else if (token.value == "b")
                {
                    attach(token, new BoldObject());
                }
                else if (token.value == "u")
                {
                    attach(token, new UnderlineObject());
                }
                else if (token.value == "ruby")
                {
                    attach(token, new RubyObject());
                }
                else if (token.value == "rt")
                {
                    if (dynamic_cast<RubyObject*>(m_current))
                        attach(token, new RubyTextObject());
                }
                else if (token.value == "v")
                {
                    VoiceObject* obj = new VoiceObject();
                    obj->value = token.annotation;
                    attach(token, obj);
                }
                else if (token.value == "lang")
                {
                    m_lang_stack.push_back(token.annotation);
                    attach(token, new LanguageObject());
                }
                break;
            }
            case Token::Type::EndTag:
            {
                bool res = false;
                switch (m_current->get_type())
                {
                case NodeObject::Type::Class:     res = (token.value == "c"); break;
                case NodeObject::Type::Italic:    res = (token.value == "i"); break;
                case NodeObject::Type::Bold:      res = (token.value == "b"); break;
                case NodeObject::Type::Underline: res = (token.value == "u"); break;
                case NodeObject::Type::Ruby:      res = (token.value == "ruby"); break;
                case NodeObject::Type::RubyText:
                    res = (token.value == "rt");
                    if (!res && token.value == "ruby")
                        m_current = m_current->parent->parent;
                    break;
                case NodeObject::Type::Voice:     res = (token.value == "v"); break;
                case NodeObject::Type::Language:
                    res = (token.value == "lang");
                    if (res)
                        m_lang_stack.pop_back();
                    break;
                }

                if (res)
                    m_current = m_current->parent;

                break;
            }
            case Token::Type::TimestampTag:
            {
                try
                {
                    qsizetype pos = 0;
                    Timestamp ts = collect_timestamp(token.value, pos);
                    if (end_of_line(token.value, pos))
                    {
                        TimestampObject* obj = new TimestampObject();
                        obj->timestamp = ts;
                        m_current->children.push_back(obj);
                    }
                }
                catch (std::exception&)
                {
                }
                break;
            }
            default:
                break;
            }
        }
        return std::shared_ptr<NodeObjects>(result);
    }

    void CueTextParser::attach(const Token& token, InternalNodeObject* obj)
    {
        for (const QString& cl : token.classes)
            if (!cl.isEmpty())
                obj->classes.push_back(cl);
        if (!m_lang_stack.empty())
            obj->language = m_lang_stack.back();
        obj->parent = m_current;
        m_current->children.push_back(obj);
        m_current = obj;
    }

    std::shared_ptr<NodeObjects> parse_cue_text(QStringView str)
    {
        return CueTextParser(str).parse();
    }

    Subtitles::~Subtitles()
    {
        qDeleteAll(m_cues);
        qDeleteAll(m_regions);
        qDeleteAll(m_stylesheets);
    }

    int Subtitles::get_num_cues() const
    {
        return m_cues.size();
    }

    const ICue* Subtitles::get_cue(int index) const
    {
        if (index >= 0 && index < m_cues.size())
            return m_cues[index];
        else
            return nullptr;
    }

    const ICue* Subtitles::pick_cue(Timestamp time, bool check_end_time) const
    {
        auto comp = [](const ICue* cue, int time) -> bool {
            return cue->get_start_time() < time;
        };

        auto it = std::lower_bound(m_cues.begin(), m_cues.end(), time, comp);
        if (it == m_cues.begin())
            it = m_cues.end();
        else
            --it;

        if (it != m_cues.end())
        {
            const ICue* cue = *it;
            if (!check_end_time || time < cue->get_end_time())
                return cue;
        }

        return nullptr;
    }

}