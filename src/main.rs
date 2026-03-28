mod app;
mod ffi;
mod learning;
mod ui;

use std::fs;
use std::rc::Rc;

use gtk::prelude::*;
use gtk4 as gtk;

fn load_css() {
    let css_path = std::path::Path::new("assets/styles/app.css");
    let provider = gtk::CssProvider::new();

    match fs::read_to_string(css_path) {
        Ok(css) => provider.load_from_string(css.as_str()),
        Err(error) => eprintln!("failed to load CSS from {}: {}", css_path.display(), error),
    }

    if let Some(display) = gtk::gdk::Display::default() {
        gtk::style_context_add_provider_for_display(
            &display,
            &provider,
            gtk::STYLE_PROVIDER_PRIORITY_APPLICATION,
        );
    }
}

fn build_ui(application: &gtk::Application) {
    let state = Rc::new(app::AppState::new());
    let window = ui::main_window::build_main_window(application, state);
    window.present();
}

fn main() {
    let application = gtk::Application::builder()
        .application_id("com.heaplens.app")
        .build();

    #[cfg(not(all(target_os = "linux", target_arch = "x86_64")))]
    eprintln!("HeapLens targets Linux x86_64 only.");

    application.connect_startup(|_| load_css());
    application.connect_activate(build_ui);
    application.run();
}
