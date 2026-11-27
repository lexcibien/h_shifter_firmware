use gpui_kit::{
    Context, IntoElement, Window,
    component::{
        Disableable, WindowExt,
        button::{Button, ButtonVariant, ButtonVariants},
        dialog::DialogButtonProps,
        h_flex,
        label::Label,
        v_flex,
    },
    prelude::*,
};

use crate::{
    app::{Configurator, Connected},
    connection::Operation,
};

pub(super) fn render(device: &Connected, cx: &mut Context<Configurator>) -> impl IntoElement {
    let enabled = device.can_send();
    let operation_label = match device.pending {
        Some(Operation::Save(_)) => Some("Salvando na flash…"),
        Some(Operation::Restore) => Some("Restaurando padrões…"),
        Some(Operation::Reboot) => Some("Reiniciando…"),
        Some(Operation::Redetect) => Some("Reverificando manopla…"),
        _ => None,
    };
    v_flex()
        .gap_2()
        .child(
            h_flex()
                .gap_2()
                .flex_wrap()
                .justify_end()
                .child(
                    Button::new("restore")
                        .label("Restaurar padrões")
                        .danger()
                        .disabled(!enabled)
                        .on_click(cx.listener(|app, _, window, cx| {
                            confirm(
                                app,
                                Operation::Restore,
                                "Restaurar padrões",
                                "Os padrões de fábrica serão gravados na flash.",
                                window,
                                cx,
                            );
                        })),
                )
                .child(
                    Button::new("reboot")
                        .label("Reiniciar")
                        .disabled(!enabled)
                        .on_click(cx.listener(|app, _, window, cx| {
                            confirm(
                                app,
                                Operation::Reboot,
                                "Reiniciar dispositivo",
                                "A conexão será encerrada durante o reinício.",
                                window,
                                cx,
                            );
                        })),
                )
                .child(
                    Button::new("save")
                        .label("Salvar na flash")
                        .primary()
                        .disabled(!enabled)
                        .on_click(cx.listener(|app, _, _, cx| {
                            if let Some(device) = app.state.connected() {
                                app.perform(Operation::Save(device.draft), cx);
                            }
                        })),
                ),
        )
        .when_some(operation_label, |view, text| view.child(Label::new(text)))
}

fn confirm(
    app: &Configurator,
    operation: Operation,
    title: &'static str,
    message: &'static str,
    window: &mut Window,
    cx: &mut Context<Configurator>,
) {
    let target = cx.weak_entity();
    let session = app.session_number;
    window.open_alert_dialog(cx, move |dialog, _, _| {
        let target = target.clone();
        dialog
            .title(title)
            .description(message)
            .button_props(
                DialogButtonProps::default()
                    .ok_text(if matches!(operation, Operation::Restore) {
                        "Restaurar"
                    } else {
                        "Reiniciar"
                    })
                    .cancel_text("Cancelar")
                    .ok_variant(ButtonVariant::Danger)
                    .show_cancel(true),
            )
            .on_ok(move |_, _, cx| {
                let _ = target.update(cx, |app, cx| {
                    if app.session_number == session {
                        app.perform(operation, cx);
                    }
                });
                true
            })
    });
}
