use gpui_kit::{
    Context, IntoElement,
    component::{
        Disableable,
        button::Button,
        description_list::DescriptionList,
        group_box::{GroupBox, GroupBoxVariants},
        label::Label,
    },
    prelude::*,
};

use crate::{
    app::{Configurator, Connected},
    connection::Operation,
};

pub(super) fn render(device: &Connected, cx: &mut Context<Configurator>) -> impl IntoElement {
    let status = device.status;
    let mode = if status.sequential { "Sequencial" } else { "H" };
    let handle = if status.handle_connected {
        "Conectada"
    } else {
        "Desconectada"
    };
    GroupBox::new()
        .id("diagnostics")
        .outline()
        .title(Label::new("Diagnóstico"))
        .child(
            DescriptionList::vertical()
                .columns(3)
                .item("Modo", mode, 1)
                .item("Manopla", handle, 1)
                .item("ADC", status.adc.to_string(), 1)
                .item("Entradas", format!("0x{:04X}", status.inputs), 1)
                .item("HID", format!("0x{:04X}", status.hid), 1),
        )
        .child(
            Button::new("redetect")
                .label("Reverificar manopla")
                .disabled(!device.can_send())
                .on_click(cx.listener(|app, _, _, cx| app.perform(Operation::Redetect, cx))),
        )
}
