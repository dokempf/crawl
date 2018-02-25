define(["jquery", "comm", "client", "ui"],
function ($, comm, client, ui) {
    "use strict";

    var ui_handlers = {};

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

    comm.register_handlers({
        "ui-push": recv_ui_push,
        "ui-pop": recv_ui_pop,
        "ui-stack": recv_ui_stack,
    });

    return {
        register_handlers: register_ui_handlers,
    };
});
