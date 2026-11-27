use gpui_kit::{
    Context, Entity, Hsla, IntoElement, Rgba, Subscription, Window,
    component::{
        ActiveTheme,
        color_picker::{ColorPicker, ColorPickerEvent, ColorPickerState},
        group_box::{GroupBox, GroupBoxVariants},
        h_flex,
        label::Label,
        slider::{Slider, SliderEvent, SliderState, SliderValue},
        switch::Switch,
        v_flex,
    },
    prelude::*,
    rgb,
};

use crate::{
    app::{Configurator, Connected},
    device::{
        Capabilities, DEBOUNCE_MS, DeviceConfig, DeviceInfo, HANDLE_THRESHOLD, MIN_GEARS,
        SEQUENTIAL_HOLD_MS,
    },
};

pub(crate) struct SettingsControls {
    gears: Entity<SliderState>,
    debounce: Entity<SliderState>,
    hold: Entity<SliderState>,
    threshold: Entity<SliderState>,
    h_color: Entity<ColorPickerState>,
    sequential_color: Entity<ColorPickerState>,
    _subscriptions: [Subscription; 6],
}

impl SettingsControls {
    pub(crate) fn new(
        info: DeviceInfo,
        config: DeviceConfig,
        window: &mut Window,
        cx: &mut Context<Configurator>,
    ) -> Self {
        let (gears, gears_subscription) = slider(
            u16::from(MIN_GEARS)..=u16::from(info.max_gears),
            1.,
            u16::from(config.forward_gears),
            |draft, value| draft.forward_gears = value as u8,
            cx,
        );
        let (debounce, debounce_subscription) = slider(
            DEBOUNCE_MS,
            1.,
            config.debounce_ms,
            |draft, value| draft.debounce_ms = value,
            cx,
        );
        let (hold, hold_subscription) = slider(
            SEQUENTIAL_HOLD_MS,
            50.,
            config.sequential_hold_ms,
            |draft, value| draft.sequential_hold_ms = value,
            cx,
        );
        let (threshold, threshold_subscription) = slider(
            HANDLE_THRESHOLD,
            10.,
            config.handle_threshold,
            |draft, value| draft.handle_threshold = value,
            cx,
        );
        let (h_color, h_subscription) = color_picker(
            config.h_color,
            |draft, value| draft.h_color = value,
            window,
            cx,
        );
        let (sequential_color, seq_subscription) = color_picker(
            config.sequential_color,
            |draft, value| draft.sequential_color = value,
            window,
            cx,
        );
        Self {
            gears,
            debounce,
            hold,
            threshold,
            h_color,
            sequential_color,
            _subscriptions: [
                gears_subscription,
                debounce_subscription,
                hold_subscription,
                threshold_subscription,
                h_subscription,
                seq_subscription,
            ],
        }
    }
}

fn slider(
    range: std::ops::RangeInclusive<u16>,
    step: f32,
    value: u16,
    set: fn(&mut DeviceConfig, u16),
    cx: &mut Context<Configurator>,
) -> (Entity<SliderState>, Subscription) {
    let state = cx.new(|_| {
        SliderState::new()
            .min(f32::from(*range.start()))
            .max(f32::from(*range.end()))
            .step(step)
            .default_value(f32::from(value))
    });
    let subscription = cx.subscribe(&state, move |app, _, event, cx| {
        if let SliderEvent::Change(SliderValue::Single(value)) = *event {
            app.edit(|draft| set(draft, value.round() as u16), cx);
        }
    });
    (state, subscription)
}

fn color_picker(
    value: u32,
    set: fn(&mut DeviceConfig, u32),
    window: &mut Window,
    cx: &mut Context<Configurator>,
) -> (Entity<ColorPickerState>, Subscription) {
    let state = cx.new(|cx| ColorPickerState::new(window, cx).default_value(rgb(value)));
    let subscription = cx.subscribe(&state, move |app, _, event, cx| {
        if let ColorPickerEvent::Change(Some(color)) = *event {
            app.edit(|draft| set(draft, rgb24(color)), cx);
        }
    });
    (state, subscription)
}

fn rgb24(color: Hsla) -> u32 {
    let color = Rgba::from(color);
    // The firmware stores RGB bytes, while the picker uses float channels and alpha.
    let to_byte = |value: f32| (value.clamp(0., 1.) * 255.).round() as u32;
    (to_byte(color.r) << 16) | (to_byte(color.g) << 8) | to_byte(color.b)
}

pub(super) fn render(
    controls: &SettingsControls,
    device: &Connected,
    cx: &mut Context<Configurator>,
) -> impl IntoElement {
    let draft = device.draft;
    let capabilities = device.info.capabilities;
    v_flex()
        .gap_4()
        .child(
            GroupBox::new()
                .id("operation-settings")
                .outline()
                .title(
                    h_flex()
                        .gap_3()
                        .items_center()
                        .child(Label::new("Funcionamento"))
                        .when(device.dirty(), |title| {
                            title.child(
                                Label::new("Alterações não salvas").text_color(cx.theme().warning),
                            )
                        }),
                )
                .child(range_row(
                    "Marchas dianteiras",
                    format!("{} marchas", draft.forward_gears),
                    &controls.gears,
                ))
                .when(capabilities.has(Capabilities::INITIAL_MODE), |panel| {
                    panel.child(
                        Switch::new("initial-mode")
                            .label("Iniciar em sequencial")
                            .checked(draft.start_sequential)
                            .on_change(cx.listener(|app, value, _, cx| {
                                app.edit(|draft| draft.start_sequential = *value, cx)
                            })),
                    )
                })
                .when(capabilities.has(Capabilities::REVERSE), |panel| {
                    panel.child(
                        Switch::new("reverse")
                            .label("Marcha ré")
                            .checked(draft.reverse_enabled)
                            .on_change(cx.listener(|app, value, _, cx| {
                                app.edit(|draft| draft.reverse_enabled = *value, cx)
                            })),
                    )
                }),
        )
        .when(
            capabilities
                .has(Capabilities::DEBOUNCE | Capabilities::SEQUENTIAL_HOLD | Capabilities::HANDLE),
            |view| {
                view.child(
                    GroupBox::new()
                        .id("input-settings")
                        .outline()
                        .title(Label::new("Entradas"))
                        .when(capabilities.has(Capabilities::DEBOUNCE), |panel| {
                            panel.child(range_row(
                                "Debounce",
                                format!("{} ms", draft.debounce_ms),
                                &controls.debounce,
                            ))
                        })
                        .when(capabilities.has(Capabilities::SEQUENTIAL_HOLD), |panel| {
                            panel.child(range_row(
                                "Tempo de acionamento sequencial",
                                format!("{} ms", draft.sequential_hold_ms),
                                &controls.hold,
                            ))
                        })
                        .when(capabilities.has(Capabilities::HANDLE), |panel| {
                            panel.child(range_row(
                                "Limiar da manopla",
                                draft.handle_threshold.to_string(),
                                &controls.threshold,
                            ))
                        }),
                )
            },
        )
        .when(capabilities.has(Capabilities::COLORS), |view| {
            view.child(
                GroupBox::new()
                    .id("led-settings")
                    .outline()
                    .title(Label::new("LED"))
                    .child(color_row("Modo H", draft.h_color, &controls.h_color))
                    .child(color_row(
                        "Sequencial",
                        draft.sequential_color,
                        &controls.sequential_color,
                    )),
            )
        })
}

fn range_row(label: &'static str, value: String, state: &Entity<SliderState>) -> impl IntoElement {
    v_flex()
        .gap_1()
        .child(
            h_flex()
                .justify_between()
                .gap_2()
                .child(Label::new(label))
                .child(Label::new(value)),
        )
        .child(Slider::new(state).accessibility_label(label))
}

fn color_row(
    label: &'static str,
    value: u32,
    state: &Entity<ColorPickerState>,
) -> impl IntoElement {
    h_flex()
        .gap_3()
        .justify_between()
        .child(Label::new(label))
        .child(ColorPicker::new(state).label(format!("#{value:06X}")))
}
