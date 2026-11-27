use gpui_kit::{
    App, Context, Entity, IntoElement, SharedString, Window,
    component::{
        Disableable, IndexPath,
        alert::Alert,
        button::{Button, ButtonVariants},
        description_list::DescriptionList,
        group_box::{GroupBox, GroupBoxVariants},
        h_flex,
        label::Label,
        select::{Select, SelectItem, SelectState},
    },
    prelude::*,
};

use crate::{
    app::{Configurator, State},
    connection::Port,
    device::ConfigSource,
};

impl SelectItem for Port {
    type Value = Port;

    fn title(&self) -> SharedString {
        match &self.serial_number {
            Some(number) => format!("{} · {number}", self.name).into(),
            None => self.name.clone().into(),
        }
    }

    fn value(&self) -> &Self::Value {
        self
    }
}

pub(crate) struct DevicePicker {
    select: Entity<SelectState<Vec<Port>>>,
}

impl DevicePicker {
    pub(crate) fn new(window: &mut Window, cx: &mut Context<Configurator>) -> Self {
        let select = cx.new(|cx| SelectState::new(Vec::new(), None, window, cx));
        Self { select }
    }

    pub(crate) fn selected<'a>(&self, cx: &'a App) -> Option<&'a Port> {
        self.select.read(cx).selected_value()
    }

    pub(crate) fn set_ports(&mut self, ports: Vec<Port>, window: &mut Window, cx: &mut App) {
        let has_ports = !ports.is_empty();
        self.select.update(cx, |state, cx| {
            state.set_items(ports, window, cx);
            if has_ports && state.selected_value().is_none() {
                state.set_selected_index(Some(IndexPath::default()), window, cx);
            }
        });
    }
}

pub(super) fn render(app: &Configurator, cx: &mut Context<Configurator>) -> impl IntoElement {
    let disconnected = matches!(app.state, State::Disconnected);
    let has_port = app.picker.selected(cx).is_some();
    let action = match app.state {
        State::Disconnected => "Conectar",
        State::Connecting => "Cancelar",
        State::Connected(_) => "Desconectar",
    };

    GroupBox::new()
        .id("device-card")
        .outline()
        .title(Label::new("Dispositivo"))
        .child(
            h_flex()
                .gap_2()
                .items_center()
                .child(
                    Select::new(&app.picker.select)
                        .accessibility_label("Porta de configuração")
                        .placeholder("Selecione uma porta de configuração")
                        .disabled(!disconnected || !has_port)
                        .flex_1(),
                )
                .child(
                    Button::new("refresh-ports")
                        .label(if app.scanning {
                            "Buscando portas…"
                        } else {
                            "Buscar portas"
                        })
                        .disabled(!disconnected || app.scanning)
                        .on_click(cx.listener(|app, _, window, cx| app.refresh_ports(window, cx))),
                )
                .child(
                    Button::new("connect")
                        .primary()
                        .label(action)
                        .disabled(disconnected && !has_port)
                        .on_click(
                            cx.listener(|app, _, window, cx| app.toggle_connection(window, cx)),
                        ),
                ),
        )
        .when(disconnected && !app.scanning && !has_port, |panel| {
            panel.child(Label::new(
                "Nenhuma porta de configuração do DAZ Shifter encontrada.",
            ))
        })
        .when(matches!(app.state, State::Connecting), |panel| {
            panel.child(Label::new("Conectando ao dispositivo…"))
        })
        .when_some(app.state.connected(), |panel, device| {
            let [major, minor, patch] = device.info.version;
            let firmware = format!("{major}.{minor}.{patch}");
            let source = match device.snapshot.source {
                ConfigSource::Defaults => "Padrões do firmware".into(),
                ConfigSource::Flash => format!("Flash · geração {}", device.snapshot.generation),
            };

            panel
                .child(
                    DescriptionList::vertical()
                        .columns(3)
                        .item("Projeto", device.info.project.label(), 1)
                        .item("Firmware", firmware, 1)
                        .item("Configuração", source, 1),
                )
                .when(!device.status.unlocked, |panel| {
                    let instruction = format!(
                        "Configuração bloqueada. {}",
                        device.info.project.unlock_instruction()
                    );
                    panel.child(Alert::warning("locked", instruction))
                })
        })
}
