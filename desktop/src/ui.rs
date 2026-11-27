mod actions;
mod device_card;
mod diagnostics;
mod settings;

use gpui_kit::{
    Context, IntoElement,
    component::{
        ActiveTheme, StyledExt, alert::Alert, label::Label, scroll::ScrollableElement, v_flex,
    },
    div,
    prelude::*,
    px,
};

pub(crate) use self::{device_card::DevicePicker, settings::SettingsControls};
use crate::app::Configurator;

pub(crate) fn render(app: &Configurator, cx: &mut Context<Configurator>) -> impl IntoElement {
    div()
        .id("configurator")
        .size_full()
        .bg(cx.theme().background)
        .text_color(cx.theme().foreground)
        .overflow_y_scrollbar()
        .child(
            v_flex()
                .w_full()
                .max_w(px(860.))
                .mx_auto()
                .p_5()
                .gap_4()
                .child(Label::new("DAZ Shifter").text_2xl().font_bold())
                .child(device_card::render(app, cx))
                .when_some(
                    app.state.connected().zip(app.settings.as_ref()),
                    |view, (device, controls)| {
                        view.child(settings::render(controls, device, cx))
                            .child(diagnostics::render(device, cx))
                            .child(actions::render(device, cx))
                    },
                )
                .when_some(app.notice.as_ref(), |view, notice| {
                    let message = notice.text.clone();
                    let alert = if notice.error {
                        Alert::error("notice", message)
                    } else {
                        Alert::info("notice", message)
                    };
                    view.child(alert)
                }),
        )
}
