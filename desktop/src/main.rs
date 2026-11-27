mod app;
mod connection;
mod device;
mod protocol;
mod ui;

use app::Configurator;
use gpui_kit::{
    AppContext, Bounds, TitlebarOptions, WindowBounds, WindowOptions, application, assets,
    component::{Theme, ThemeMode, set_locale},
    px, size,
};

fn main() {
    application().with_assets(assets::Assets).run(|cx| {
        gpui_kit::init(cx);
        set_locale("pt-BR");
        Theme::change(ThemeMode::Dark, None, cx);
        let bounds = Bounds::centered(None, size(px(760.), px(780.)), cx);
        match gpui_kit::open_window(
            WindowOptions {
                titlebar: Some(TitlebarOptions {
                    title: Some("DAZ Shifter".into()),
                    ..Default::default()
                }),
                window_bounds: Some(WindowBounds::Windowed(bounds)),
                window_min_size: Some(size(px(460.), px(360.))),
                ..Default::default()
            },
            cx,
            |window, cx| cx.new(|cx| Configurator::new(window, cx)),
        ) {
            Ok(_) => cx.activate(true),
            Err(error) => {
                eprintln!("Falha ao abrir a janela do DAZ Shifter: {error}");
                cx.quit();
            }
        }
    });
}
