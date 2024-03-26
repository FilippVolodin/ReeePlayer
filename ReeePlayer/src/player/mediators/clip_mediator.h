#pragma once

class Clip;
class ClipUserData;

class ClipUnit
{
public:
    virtual ~ClipUnit() = default;

    virtual void load(const Clip& clip) = 0;
    virtual void save(ClipUserData& clip) = 0;
};

class ClipMediator
{
public:
    void add_unit(ClipUnit*);
    void load(const Clip& clip);
    std::unique_ptr<ClipUserData> save();
    void save(ClipUserData& clip);

private:
    std::vector<ClipUnit*> m_units;
};