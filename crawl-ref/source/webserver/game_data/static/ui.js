define(["jquery", "comm", "client", "ui", "./enums", "./cell_renderer",
    "./util", "./scroller", "./tileinfo-main"],
function ($, comm, client, ui, enums, cr, util, scroller, main) {
    "use strict";

    function fmt_body_txt(txt)
    {
        return txt
            .split("\n\n")
            .map(function (s) { return "<p>"+s.trim()+"</p>"; })
            .filter(function (s) { return s !== "<p></p>"; })
            .join("")
            .split("\n").join("<br>");
    }

    function paneset_cycle($el, next)
    {
        var $panes = $el.children(".pane");
        var $current = $panes.filter(".current").removeClass("current");
        if (next === undefined)
            next = $panes.index($current) + 1;
        $panes.eq(next % $panes.length).addClass("current");
    }

    function describe_generic(desc)
    {
        var $popup = $(".templates > .describe-generic").clone();
        var $body = $popup.children(".body");
        $popup.find(".header > span").html(desc.title);
        $body.html(fmt_body_txt(desc.body + desc.footer));
        scroller($popup.children(".body")[0])
            .contentElement.className += " describe-generic-body";

        var canvas = $popup.find(".header > canvas");
        if (desc.tile)
        {
            var renderer = new cr.DungeonCellRenderer();
            util.init_canvas(canvas[0], renderer.cell_width, renderer.cell_height);
            renderer.init(canvas[0]);
            renderer.draw_from_texture(desc.tile.t, 0, 0, desc.tile.tex, 0, 0, desc.tile.ymax, false);
        }
        else
            canvas.remove();

        return $popup;
    }

    function describe_feature_wide(desc)
    {
        var $popup = $(".templates > .describe-feature").clone();
        var $feat_tmpl = $(".templates > .describe-generic");
        desc.feats.forEach(function (feat) {
            var $feat = $feat_tmpl.clone().removeClass("hidden").addClass("describe-feature-feat");
            $feat.find(".header > span").html(feat.title);
            if (feat.body != feat.title)
                $feat.find(".body").html(feat.body);
            else
                $feat.find(".body").remove();

            var canvas = $feat.find(".header > canvas");
            var renderer = new cr.DungeonCellRenderer();
            util.init_canvas(canvas[0], renderer.cell_width, renderer.cell_height);
            renderer.init(canvas[0]);
            renderer.draw_from_texture(feat.tile.t, 0, 0, feat.tile.tex, 0, 0, feat.tile.ymax, false);
            $popup.append($feat);
        });
        scroller($popup[0]);
        return $popup;
    }

    function describe_item(desc)
    {
        var $popup = $(".templates > .describe-item").clone();
        $popup.find(".header > span").html(desc.title);
        $popup.find(".body").html(fmt_body_txt(desc.body));
        scroller($popup.find(".body")[0]);
        if (desc.actions !== "")
            $popup.find(".actions").html(desc.actions);
        else
            $popup.find(".actions").remove();

        var canvas = $popup.find(".header > canvas");
        var renderer = new cr.DungeonCellRenderer();
        util.init_canvas(canvas[0], renderer.cell_width, renderer.cell_height);
        renderer.init(canvas[0]);

        desc.tiles.forEach(function (tile) {
            renderer.draw_from_texture(tile.t, 0, 0, tile.tex, 0, 0, tile.ymax, false);
        });

        return $popup;
    }

    function describe_spell(desc)
    {
        var $popup = $(".templates > .describe-spell").clone();
        $popup.find(".header > span").html(desc.title);
        $popup.find(".body").html(fmt_body_txt(desc.desc));
        scroller($popup.find(".body")[0]);
        if (desc.can_mem)
            $popup.find(".actions").removeClass("hidden");

        var canvas = $popup.find(".header > canvas");
        var renderer = new cr.DungeonCellRenderer();
        util.init_canvas(canvas[0], renderer.cell_width, renderer.cell_height);
        renderer.init(canvas[0]);
        renderer.draw_from_texture(desc.tile.t, 0, 0, desc.tile.tex, 0, 0, desc.tile.ymax, false);

        return $popup;
    }

    function describe_cards(desc)
    {
        var $popup = $(".templates > .describe-cards").clone();
        var $card_tmpl = $(".templates > .describe-generic");
        var t = main.MISC_CARD, tex = enums.texture.DEFAULT;
        desc.cards.forEach(function (card) {
            var $card = $card_tmpl.clone().removeClass("hidden").addClass("describe-card");
            $card.find(".header > span").html(card.name);
            $card.find(".body").html(fmt_body_txt(card.desc + card.decks));

            var canvas = $card.find(".header > canvas");
            var renderer = new cr.DungeonCellRenderer();
            util.init_canvas(canvas[0], renderer.cell_width, renderer.cell_height);
            renderer.init(canvas[0]);
            renderer.draw_from_texture(t, 0, 0, tex, 0, 0, 0, false);
            $popup.append($card);
        });
        scroller($popup[0]);
        return $popup;
    }

    var ui_handlers = {
        "describe-generic" : describe_generic,
        "describe-feature-wide" : describe_feature_wide,
        "describe-item" : describe_item,
        "describe-spell" : describe_spell,
        "describe-cards" : describe_cards,
    };

    function register_ui_handlers(dict)
    {
        $.extend(ui_handlers, dict);
    }

    function recv_ui_push(msg)
    {
        var handler = ui_handlers[msg.type];
        var popup = handler ? handler(msg) : $("<div>Unhandled UI type "+msg.type+"</div>");
        ui.show_popup(popup);
    }

    function recv_ui_pop(msg)
    {
        ui.hide_popup();
    }

    function recv_ui_stack(msg)
    {
        if (!client.is_watching())
            return;
        for (var i = 0; i < msg.items.length; i++)
            comm.handle_message(msg.items[i]);
    }

    function recv_ui_state(msg)
    {
        var ui_handlers = {
        };
        var handler = ui_handlers[msg.type];
        if (handler)
            handler(msg);
    }

    comm.register_handlers({
        "ui-push": recv_ui_push,
        "ui-pop": recv_ui_pop,
        "ui-stack": recv_ui_stack,
        "ui-state": recv_ui_state,
    });

    function top_popup()
    {
        var $popup = $("#ui-stack").children().last();
        if ($popup.length === 0)
            return;
        return $popup.find(".ui-popup-inner").children().eq(0);
    }

    return {
        register_handlers: register_ui_handlers,
    };
});
