mod configurator;

use configurator::Configurator;
use gpui::{
    App, AppContext, Application, Bounds, TitlebarOptions, WindowBounds, WindowOptions, px, size,
};

fn main() {
    Application::new().run(|cx: &mut App| {
        let bounds = Bounds::centered(None, size(px(600.), px(400.)), cx);
        cx.open_window(
            WindowOptions {
                titlebar: Some(TitlebarOptions {
                    title: Some("DAZ Shifter".into()),
                    ..Default::default()
                }),
                window_bounds: Some(WindowBounds::Windowed(bounds)),
                window_min_size: Some(size(px(460.), px(360.))),
                ..Default::default()
            },
            |_, cx| cx.new(|_| Configurator::new()),
        )
        .expect("Falha ao abrir a janela do DAZ Shifter");
        cx.activate(true);
    });
}
