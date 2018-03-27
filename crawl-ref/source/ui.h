/**
 * @file
 * @brief Hierarchical layout system.
**/

#pragma once

#include <functional>

#include "format.h"
#include "tilefont.h"
#include "tiledef-gui.h"
#include "windowmanager.h"
#ifdef USE_TILE_LOCAL
# include "tilesdl.h"
# include "tilebuf.h"
#endif

struct i4 {
    int xyzw[4];
    template <typename... Ts> i4 (Ts... l) : xyzw{l...} {}
    const int& operator[](int index) const { return xyzw[index]; }
    int& operator[](int index) { return xyzw[index]; }
    inline bool operator==(const i4& rhs) { return equal(begin(xyzw), end(xyzw), begin(rhs.xyzw)); }
    inline bool operator!=(const i4& rhs) { return !(*this == rhs); }
};

struct i2 {
    int xy[2];
    template <typename... Ts> i2 (Ts... l) : xy{l...} {}
    const int& operator[](int index) const { return xy[index]; }
    int& operator[](int index) { return xy[index]; }
    inline bool operator==(const i2& rhs) { return equal(begin(xy), end(xy), begin(rhs.xy)); }
    inline bool operator!=(const i2& rhs) { return !(*this == rhs); }
};

struct UISizeReq
{
    int min, nat;
};

typedef enum {
    UI_ALIGN_UNSET = 0,
    UI_ALIGN_START,
    UI_ALIGN_END,
    UI_ALIGN_CENTER,
    UI_ALIGN_STRETCH,
} UIAlign_type;

typedef enum {
    UI_JUSTIFY_START = 0,
    UI_JUSTIFY_CENTER,
    UI_JUSTIFY_END,
} UIJustify_type;

template<typename, typename> class Slot;

template<class Target, class... Args>
class Slot<Target, bool (Args...)>
{
public:
    typedef function<bool (Args...)> HandlerSig;
    typedef multimap<Target*, HandlerSig> HandlerMap;
    bool emit(Target *target, Args&... args)
    {
        auto i = handlers.equal_range(target);
        for (auto it = i.first; it != i.second; ++it)
        {
            HandlerSig func = it->second;
            if (func(forward<Args>(args)...))
                return true;
        }
        return false;
    }
    void on(Target *target, HandlerSig handler)
    {
        auto new_pair = pair<Target*, HandlerSig>(target, handler);
        handlers.insert(new_pair);
    }
    void remove_by_target(Target *target)
    {
        handlers.erase(target);
    }
protected:
    HandlerMap handlers;
};

class UI
{
public:
    UI() : margin({0,0,0,0}), flex_grow(1), align_self(UI_ALIGN_UNSET), expand_h(false), expand_v(false), cached_sr_valid{false, false} {};
    ~UI() {
        UI::slots.event.remove_by_target(this);
    }

    i4 margin;
    int flex_grow;
    UIAlign_type align_self;
    bool expand_h, expand_v;
    const i4 get_region() const { return m_region; }

    virtual void _render() = 0;
    virtual UISizeReq _get_preferred_size(int dim, int prosp_width);
    virtual void _allocate_region();

    // Wrapper functions which handle common behavior
    // - margins
    // - debug drawing
    // - caching
    void render();
    UISizeReq get_preferred_size(int dim, int prosp_width);
    void allocate_region(i4 region);

    void set_margin_for_crt(i4 _margin)
    {
#ifndef USE_TILE_LOCAL
        margin = _margin;
#endif
    };
    void set_margin_for_sdl(i4 _margin)
    {
#ifdef USE_TILE_LOCAL
        margin = _margin;
#endif
    };

    virtual bool on_event(wm_event event);

    template<class T, class... Args, typename F>
    void on(Slot<T, bool (Args...)>& slot, F&& cb)
    {
        slot.on(this, cb);
    }
    static struct slots {
        Slot<UI, bool (wm_event)> event;
    } slots;

protected:
    i4 m_region;

private:
    bool cached_sr_valid[2];
    UISizeReq cached_sr[2];
    int cached_sr_pw;
};

class UIContainer : public UI
{
protected:
    class iter_impl
    {
    public:
        virtual ~iter_impl() {};
        virtual void operator++() = 0;
        virtual shared_ptr<UI>& operator*() = 0;
        virtual bool equal (iter_impl &other) const = 0;
    };

public:
    class iterator
    {
    public:
        iterator(iter_impl *_it) : it(_it) {};
        ~iterator() { delete it; };
        void operator++() { ++(*it); };
        bool operator==(iterator &other) const {
            return typeid(it) == typeid(other.it) && it->equal(*other.it);
        };
        bool operator!=(iterator &other) const { return !(*this == other); }
        shared_ptr<UI>& operator*() { return **it; };
    protected:
        iter_impl *it;
    };

    virtual bool on_event(wm_event event) override;

protected:
    virtual iterator begin() = 0;
    virtual iterator end() = 0;
};

class UIBin : public UI
{
public:
    virtual bool on_event(wm_event event) override;
    virtual shared_ptr<UI> get_child() { return m_child; };
protected:
    shared_ptr<UI> m_child;
};

class UIContainerVec : public UIContainer
{
private:
    typedef UIContainer::iterator I;

    class iter_impl_vec : public iter_impl
    {
    private:
        typedef vector<shared_ptr<UI>> C;
    public:
        explicit iter_impl_vec (C& _c, C::iterator _it) : c(_c), it(_it) {};
    protected:
        virtual void operator++() override { ++it; };
        virtual shared_ptr<UI>& operator*() override { return *it; };
        virtual bool equal (iter_impl &_other) const override {
            iter_impl_vec &other = static_cast<iter_impl_vec&>(_other);
            return c == other.c && it == other.it;
        };

        C& c;
        C::iterator it;
    };

protected:
    virtual I begin() override { return I(new iter_impl_vec(m_children, m_children.begin())); }
    virtual I end() override { return I(new iter_impl_vec(m_children, m_children.end())); }
    vector<shared_ptr<UI>> m_children;
};

// Box widget: similar to the CSS flexbox (without wrapping)
//  - Lays its children out in either a row or a column
//  - Extra space is allocated according to each child's flex_grow property
//  - align and justify properties work like flexbox's

class UIBox : public UIContainerVec
{
public:
    UIBox() : horz(false), justify_items(UI_JUSTIFY_START), align_items(UI_ALIGN_UNSET) {
        expand_h = expand_v = true;
    };
    void add_child(shared_ptr<UI> child);
    bool horz;
    UIJustify_type justify_items;
    UIAlign_type align_items;

    virtual void _render() override;
    virtual UISizeReq _get_preferred_size(int dim, int prosp_width) override;
    virtual void _allocate_region() override;

protected:
    vector<int> layout_main_axis(vector<UISizeReq>& ch_psz, int main_sz);
    vector<int> layout_cross_axis(vector<UISizeReq>& ch_psz, int cross_sz);
};

class UIText : public UI
{
public:
    UIText() : wrap_text(false), ellipsize(false), m_wrapped_size{ -1, -1 } {}
    UIText(string text) : UIText()
    {
        set_text(formatted_string::parse_string(text));
    }

    void set_text(const formatted_string &fs);

    virtual void _render() override;
    virtual UISizeReq _get_preferred_size(int dim, int prosp_width) override;
    virtual void _allocate_region() override;

    bool wrap_text, ellipsize;

protected:
    void wrap_text_to_size(int width, int height);

    formatted_string m_text;
#ifdef USE_TILE_LOCAL
    struct brkpt { unsigned int op, line; };
    vector<brkpt> m_brkpts;
    formatted_string m_text_wrapped;
#else
    vector<formatted_string> m_wrapped_lines;
#endif
    i2 m_wrapped_size;
};

class UIImage : public UI
{
public:
    UIImage() : shrink_h(false), shrink_v(false), m_tile(TILEG_ERROR, TEX_GUI) {};
    void set_tile(tile_def tile);
    void set_file(string img_path);

    bool shrink_h, shrink_v;

    virtual void _render() override;
    virtual UISizeReq _get_preferred_size(int dim, int prosp_width) override;

protected:
    bool m_using_tile;

    tile_def m_tile;
    int m_tw, m_th;

#ifdef USE_TILE_LOCAL
    GenericTexture m_img;
#endif
};

class UIStack : public UIContainerVec
{
public:
    void add_child(shared_ptr<UI> child);
    void pop_child();
    size_t num_children() const { return m_children.size(); }
    shared_ptr<UI> get_child(size_t idx) const { return m_children[idx]; };

    virtual void _render() override;
    virtual UISizeReq _get_preferred_size(int dim, int prosp_width) override;
    virtual void _allocate_region() override;
    virtual bool on_event(wm_event event) override;
};

class UIGrid : public UIContainer
{
public:
    UIGrid() : m_track_info_dirty(false) {};

    void add_child(shared_ptr<UI> child, int x, int y, int w = 1, int h = 1);
    int& track_flex_grow(int x, int y)
    {
        init_track_info();
        ASSERT(x == -1 || y == -1);
        return (x >= 0 ? m_col_info[x].flex_grow : m_row_info[y].flex_grow);
    }

    virtual void _render() override;
    virtual UISizeReq _get_preferred_size(int dim, int prosp_width) override;
    virtual void _allocate_region() override;

protected:
    i4 get_tracks_region(int x, int y, int w, int h) const
    {
        return {
            m_col_info[x].offset, m_row_info[y].offset,
            m_col_info[x+w-1].size + m_col_info[x+w-1].offset - m_col_info[x].offset,
            m_row_info[y+h-1].size + m_row_info[y+h-1].offset - m_row_info[y].offset,
        };
    }

    struct track_info {
        int size;
        int offset;
        UISizeReq sr;
        int flex_grow;
    };
    vector<track_info> m_col_info;
    vector<track_info> m_row_info;

    struct child_info {
        i2 pos;
        i2 span;
        shared_ptr<UI> widget;
        inline bool operator==(const child_info& rhs) const { return widget == rhs.widget; }
    };
    vector<child_info> m_child_info;

    void layout_track(int dim, UISizeReq sr, int size);
    void set_track_offsets(vector<track_info>& tracks);
    void compute_track_sizereqs(int dim);
    void init_track_info();
    bool m_track_info_dirty;

private:
    typedef UIContainer::iterator I;

    class iter_impl_grid : public iter_impl
    {
    private:
        typedef vector<child_info> C;
    public:
        explicit iter_impl_grid (C _c, C::iterator _it) : c(_c), it(_it) {};
    protected:
        virtual void operator++() override { ++it; };
        virtual shared_ptr<UI>& operator*() override { return it->widget; };
        virtual bool equal (iter_impl &_other) const override {
            iter_impl_grid &other = static_cast<iter_impl_grid&>(_other);
            return c == other.c && it == other.it;
        };

        C c;
        C::iterator it;
    };

public:
    virtual I begin() override { return I(new iter_impl_grid(m_child_info, m_child_info.begin())); }
    virtual I end() override { return I(new iter_impl_grid(m_child_info, m_child_info.end())); }
};

void ui_push_layout(shared_ptr<UI> root);
void ui_pop_layout();
void ui_pump_events();

void ui_push_scissor(i4 scissor);
void ui_pop_scissor();
i4 ui_get_scissor();

// XXX: this is a hack used to ensure that when switching to a
// layout-based UI, the starting window size is correct. This is necessary
// because there's no way to query TilesFramework for the current screen size
void ui_resize(int w, int h);
