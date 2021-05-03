#pragma once

#include "types.h"
#include "qsubtitles.h"

namespace qsubs::webvtt
{
    class Parser;

    class Cue : public ICue
    {
    public:
        int get_index() const override;
        Timestamp get_start_time() const override;
        Timestamp get_end_time() const override;
        QString get_text() const override;

        void set_index(int);
        void set_start_time(Timestamp);
        void set_end_time(Timestamp);
        void set_text(QString);
    private:
        int m_index = -1;
        Timestamp m_start_time = 0;
        Timestamp m_end_time = 0;
        QString m_text;
    };
    
    class Region
    {

    };

    class StyleSheet
    {

    };

    class Subtitles : public qsubs::ISubtitles
    {
        friend Parser;
    public:
        ~Subtitles();

        int get_num_cues() const override;
        const ICue* get_cue(int index) const override;
        const ICue* pick_cue(Timestamp time, bool check_end_time) const override;
    private:
        std::vector<Cue*> m_cues;
        std::vector<Region*> m_regions;
        std::vector<StyleSheet*> m_stylesheets;
    };

    ISubtitlesPtr parse(const QString& filename);

    using Block = std::variant<Cue*, Region*, StyleSheet*>;
    using BlockOpt = std::optional<Block>;

    class Parser
    {
    public:
        Parser(QTextStream& stream);
        std::shared_ptr<Subtitles> parse();
    private:
        BlockOpt collect_block(bool in_header);

        QTextStream& m_input;
        QString m_line;

        std::shared_ptr<Subtitles> m_subtitles;
        bool m_seen_cue = false;
    };
    
    class InternalNodeObject;

    class NodeObject
    {
    public:
        enum class Type { ObjectList, Class, Italic, Bold, Underline, Ruby, RubyText, Voice, Language, Text, Timestamp };

        NodeObject() = default;
        NodeObject(const NodeObject&) = delete;
        NodeObject& operator=(const NodeObject&) = delete;
        virtual ~NodeObject() = default;

        virtual Type get_type() const = 0;

        //QString m_value;
        InternalNodeObject* parent = nullptr;
    };

    class InternalNodeObject : public NodeObject
    {
    public:
        InternalNodeObject() = default;
        InternalNodeObject(const InternalNodeObject&) = delete;
        InternalNodeObject& operator=(const InternalNodeObject&) = delete;
        ~InternalNodeObject() { for (NodeObject* obj : children) delete obj; }

        std::vector<NodeObject*> children;
        std::vector<QString> classes;
        QString language;
    };

    class NodeObjects : public InternalNodeObject
    {
    public:
        NodeObjects() = default;
        NodeObjects(const NodeObjects&) = delete;
        NodeObjects& operator=(const NodeObjects&) = delete;

        virtual Type get_type() const { return Type::ObjectList; }
    };

    class ClassObject : public InternalNodeObject
    {
    public:
        ClassObject() = default;
        ClassObject(const ClassObject&) = delete;
        ClassObject& operator=(const ClassObject&) = delete;

        virtual Type get_type() const { return Type::Class; }
    };

    class ItalicObject : public InternalNodeObject
    {
    public:
        ItalicObject() = default;
        ItalicObject(const ItalicObject&) = delete;
        ItalicObject& operator=(const ItalicObject&) = delete;

        virtual Type get_type() const { return Type::Italic; }
    };

    class BoldObject : public InternalNodeObject
    {
    public:
        BoldObject() = default;
        BoldObject(const BoldObject&) = delete;
        BoldObject& operator=(const BoldObject&) = delete;

        virtual Type get_type() const { return Type::Bold; }
    };

    class UnderlineObject : public InternalNodeObject
    {
    public:
        UnderlineObject() = default;
        UnderlineObject(const UnderlineObject&) = delete;
        UnderlineObject& operator=(const UnderlineObject&) = delete;

        virtual Type get_type() const { return Type::Underline; }
    };

    class RubyObject : public InternalNodeObject
    {
    public:
        RubyObject() = default;
        RubyObject(const RubyObject&) = delete;
        RubyObject& operator=(const RubyObject&) = delete;

        virtual Type get_type() const { return Type::Ruby; }
    };

    class RubyTextObject : public InternalNodeObject
    {
    public:
        RubyTextObject() = default;
        RubyTextObject(const RubyTextObject&) = delete;
        RubyTextObject& operator=(const RubyTextObject&) = delete;

        virtual Type get_type() const { return Type::RubyText; }
    };

    class VoiceObject : public InternalNodeObject
    {
    public:
        VoiceObject() = default;
        VoiceObject(const VoiceObject&) = delete;
        VoiceObject& operator=(const VoiceObject&) = delete;

        virtual Type get_type() const { return Type::Voice; }
        QString value;
    };

    class LanguageObject : public InternalNodeObject
    {
    public:
        LanguageObject() = default;
        LanguageObject(const LanguageObject&) = delete;
        LanguageObject& operator=(const LanguageObject&) = delete;

        virtual Type get_type() const { return Type::Language; }
    };

    class LeafNodeObject : public NodeObject
    {
    public:
        LeafNodeObject() = default;
        LeafNodeObject(const LeafNodeObject&) = delete;
        LeafNodeObject& operator=(const LeafNodeObject&) = delete;

    };

    class TextObject : public LeafNodeObject
    {
    public:
        TextObject() = default;
        TextObject(const TextObject&) = delete;
        TextObject& operator=(const TextObject&) = delete;

        virtual Type get_type() const { return Type::Text; }

        QString m_value;
    };

    class TimestampObject : public LeafNodeObject
    {
    public:
        TimestampObject() = default;
        TimestampObject(const TimestampObject&) = delete;
        TimestampObject& operator=(const TimestampObject&) = delete;

        virtual Type get_type() const { return Type::Timestamp; }

        Timestamp timestamp = 0;
    };

    std::shared_ptr<NodeObjects> parse_cue_text(QStringView str);
}