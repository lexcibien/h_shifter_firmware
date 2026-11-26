use gpui::{App, ClickEvent, Context, Div, Stateful, Window, div, prelude::*, rgb};

const MIN_GEARS: u8 = 4;
const MAX_GEARS: u8 = 6; // Limite da V1 usada na simulação.

pub struct Configurator {
    connected: bool,
    forward_gears: u8,
}

impl Configurator {
    pub fn new() -> Self {
        Self {
            connected: false,
            forward_gears: MAX_GEARS,
        }
    }
}

impl Render for Configurator {
    fn render(&mut self, _: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        let (status, status_color, connect_label) = if self.connected {
            ("Conectado · DAZ Shifter V1", 0xa6e3bd, "Desconectar")
        } else {
            ("Desconectado", 0xa9b4c0, "Simular conexão")
        };

        div()
            .size_full()
            .flex()
            .flex_col()
            .gap_5()
            .p_6()
            .bg(rgb(0x14191f))
            .text_color(rgb(0xeef2f6))
            .child(
                div()
                    .flex()
                    .flex_col()
                    .gap_1()
                    .child(div().text_2xl().child("Configurador DAZ Shifter"))
                    .child(
                        div()
                            .text_sm()
                            .text_color(rgb(0xa9b4c0))
                            .child("Simulação local para praticar sem a placa."),
                    ),
            )
            .child(
                panel().child("Dispositivo").child(
                    div()
                        .flex()
                        .items_center()
                        .justify_between()
                        .gap_2()
                        .child(div().text_sm().text_color(rgb(status_color)).child(status))
                        .child(button(
                            "connect",
                            connect_label,
                            true,
                            cx.listener(|view, _, _, cx| {
                                view.connected = !view.connected;
                                cx.notify();
                            }),
                        )),
                ),
            )
            .child(
                panel().child("Configuração local").child(
                    div()
                        .flex()
                        .items_center()
                        .justify_between()
                        .gap_2()
                        .child(
                            div()
                                .text_sm()
                                .text_color(rgb(0xa9b4c0))
                                .child("Marchas à frente"),
                        )
                        .child(
                            div()
                                .flex()
                                .items_center()
                                .gap_3()
                                .child(button(
                                    "fewer-gears",
                                    "−",
                                    self.forward_gears > MIN_GEARS,
                                    cx.listener(|view, _, _, cx| {
                                        view.forward_gears =
                                            (view.forward_gears - 1).max(MIN_GEARS);
                                        cx.notify();
                                    }),
                                ))
                                .child(div().text_lg().child(self.forward_gears.to_string()))
                                .child(button(
                                    "more-gears",
                                    "+",
                                    self.forward_gears < MAX_GEARS,
                                    cx.listener(|view, _, _, cx| {
                                        view.forward_gears =
                                            (view.forward_gears + 1).min(MAX_GEARS);
                                        cx.notify();
                                    }),
                                )),
                        ),
                ),
            )
    }
}

fn panel() -> Div {
    div()
        .flex()
        .flex_col()
        .gap_2()
        .p_4()
        .rounded_lg()
        .border_1()
        .border_color(rgb(0x343f4b))
        .bg(rgb(0x202730))
}

fn button(
    id: &'static str,
    label: &'static str,
    enabled: bool,
    on_click: impl Fn(&ClickEvent, &mut Window, &mut App) + 'static,
) -> Stateful<Div> {
    div()
        .id(id)
        .px_3()
        .py_2()
        .rounded_md()
        .text_sm()
        .child(label)
        .when(enabled, |button| {
            button
                .bg(rgb(0x33473e))
                .text_color(rgb(0xcdf5db))
                .cursor_pointer()
                .hover(|style| style.bg(rgb(0x435d50)))
                .on_click(on_click)
        })
        .when(!enabled, |button| {
            button.bg(rgb(0x303842)).text_color(rgb(0x87919d))
        })
}
