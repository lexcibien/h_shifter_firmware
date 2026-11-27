mod state;

use gpui_kit::{Context, IntoElement, Render, Task, Window};

pub(crate) use self::state::{Connected, Notice, State};
use crate::{
    connection::{self, Event, Operation, Session},
    device::DeviceConfig,
    ui::{self, DevicePicker, SettingsControls},
};

pub(crate) struct Configurator {
    pub(crate) state: State,
    pub(crate) notice: Option<Notice>,
    pub(crate) picker: DevicePicker,
    pub(crate) settings: Option<SettingsControls>,
    pub(crate) scanning: bool,
    session: Option<Session>,
    session_events: Option<Task<()>>,
    pub(crate) session_number: u64,
}

impl Configurator {
    pub(crate) fn new(window: &mut Window, cx: &mut Context<Self>) -> Self {
        let mut app = Self {
            state: State::Disconnected,
            notice: None,
            picker: DevicePicker::new(window, cx),
            settings: None,
            scanning: false,
            session: None,
            session_events: None,
            session_number: 0,
        };
        app.refresh_ports(window, cx);
        app
    }

    pub(crate) fn refresh_ports(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        if self.scanning || !matches!(self.state, State::Disconnected) {
            return;
        }
        self.scanning = true;
        let scan = cx
            .background_executor()
            .spawn(async { connection::ports() });
        cx.spawn_in(window, async move |this, cx| {
            let result = scan.await;
            let _ = this.update_in(cx, |app, window, cx| {
                app.scanning = false;
                match result {
                    Ok(ports) => {
                        app.picker.set_ports(ports, window, cx);
                    }
                    Err(error) if matches!(app.state, State::Disconnected) => {
                        app.set_notice(format!("Falha ao listar portas. {error}"), true);
                    }
                    Err(_) => {}
                }
                cx.notify();
            });
        })
        .detach();
        cx.notify();
    }

    pub(crate) fn toggle_connection(&mut self, window: &mut Window, cx: &mut Context<Self>) {
        if !matches!(self.state, State::Disconnected) {
            let saving = self.state.connected().is_some_and(|device| {
                matches!(
                    device.pending,
                    Some(Operation::Save(_) | Operation::Restore)
                )
            });
            self.close_session();
            let message = if saving {
                "Gravação não confirmada. Reconecte e confira a configuração lida."
            } else {
                "Dispositivo desconectado."
            };
            self.set_notice(message, saving);
        } else if let Some(port) = self.picker.selected(cx).cloned() {
            match Session::start(port) {
                Ok((session, events)) => {
                    self.session = Some(session);
                    self.session_number = self.session_number.wrapping_add(1);
                    self.state = State::Connecting;
                    self.notice = None;
                    self.session_events = Some(cx.spawn_in(window, async move |this, cx| {
                        while let Ok(event) = events.recv().await {
                            let result = this.update_in(cx, |app, window, cx| {
                                app.receive(event, window, cx);
                                app.session.is_some()
                            });
                            if !matches!(result, Ok(true)) {
                                return;
                            }
                        }
                        let _ = this.update_in(cx, |app, _, cx| {
                            app.close_session();
                            app.set_notice("A sessão do dispositivo foi encerrada.", true);
                            cx.notify();
                        });
                    }));
                }
                Err(error) => self.set_notice(format!("Falha ao iniciar a conexão. {error}"), true),
            }
        }
        cx.notify();
    }

    pub(crate) fn edit(&mut self, change: impl FnOnce(&mut DeviceConfig), cx: &mut Context<Self>) {
        if let Some(device) = self.state.connected_mut() {
            change(&mut device.draft);
            if device.pending.is_none() {
                self.notice = None;
            }
            cx.notify();
        }
    }

    pub(crate) fn perform(&mut self, operation: Operation, cx: &mut Context<Self>) {
        let (Some(session), Some(device)) = (&self.session, self.state.connected_mut()) else {
            return;
        };
        if !device.can_send() {
            return;
        }
        if let Operation::Save(config) = operation
            && let Err(error) = config.validate(device.info)
        {
            self.set_notice(error.to_string(), true);
            cx.notify();
            return;
        }
        match session.submit(operation) {
            Ok(()) => {
                device.pending = Some(operation);
                self.notice = None;
            }
            Err(error) => {
                self.close_session();
                self.set_notice(error.to_string(), true);
            }
        }
        cx.notify();
    }

    fn receive(&mut self, event: Event, window: &mut Window, cx: &mut Context<Self>) {
        match event {
            Event::Connected {
                info,
                snapshot,
                status,
            } => {
                self.settings = Some(SettingsControls::new(info, snapshot.config, window, cx));
                self.state = State::Connected(Connected {
                    info,
                    snapshot,
                    draft: snapshot.config,
                    status,
                    pending: None,
                });
            }
            Event::Status(status) => {
                if let Some(device) = self.state.connected_mut() {
                    if device.status == status {
                        return;
                    }
                    device.status = status;
                }
            }
            Event::Saved(snapshot) => {
                if let Some(device) = self.state.connected_mut() {
                    device.confirm_save(snapshot);
                }
                self.set_notice(
                    format!(
                        "Configuração salva na flash (geração {}).",
                        snapshot.generation
                    ),
                    false,
                );
            }
            Event::Restored(snapshot) => {
                if let Some(device) = self.state.connected_mut() {
                    device.confirm_save(snapshot);
                    device.draft = snapshot.config;
                    self.settings = Some(SettingsControls::new(
                        device.info,
                        snapshot.config,
                        window,
                        cx,
                    ));
                }
                self.set_notice("Padrões de fábrica restaurados.", false);
            }
            Event::Redetected(status) => {
                if let Some(device) = self.state.connected_mut() {
                    device.pending = None;
                    device.status = status;
                }
                self.set_notice("Manopla reverificada.", false);
            }
            Event::Rebooting => {
                self.close_session();
                self.set_notice(
                    "Dispositivo reiniciando. Reconecte quando ele voltar.",
                    false,
                );
            }
            Event::Failed {
                operation,
                error,
                terminal,
            } => {
                if terminal {
                    self.close_session();
                } else if let Some(device) = self.state.connected_mut() {
                    device.pending = None;
                    if error.is_locked() {
                        device.status.unlocked = false;
                    }
                }
                let mut message = match operation {
                    Some(operation) => format!("{} {error}", operation.failure()),
                    None => error.to_string(),
                };
                if terminal
                    && operation
                        .is_some_and(|op| matches!(op, Operation::Save(_) | Operation::Restore))
                {
                    message.push_str(" Reconecte e confira a configuração lida.");
                }
                self.set_notice(message, true);
            }
        }
        cx.notify();
    }

    fn close_session(&mut self) {
        self.session = None;
        self.session_events = None;
        self.settings = None;
        self.state = State::Disconnected;
    }

    fn set_notice(&mut self, text: impl Into<String>, error: bool) {
        self.notice = Some(Notice {
            text: text.into(),
            error,
        });
    }
}

impl Render for Configurator {
    fn render(&mut self, _: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        ui::render(self, cx)
    }
}
