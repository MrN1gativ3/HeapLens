use std::rc::Rc;

use gtk::prelude::*;
use gtk4 as gtk;

#[derive(Clone)]
pub struct TabBarView {
    pub root: gtk::Box,
}

impl TabBarView {
    pub fn new(stack: &gtk::Stack) -> Self {
        let root = gtk::Box::new(gtk::Orientation::Horizontal, 0);
        let learning = tab_button("📚", "GLIBC HEAP INTERNALS", false);
        let exploit = tab_button("●", "EXPLOITATION LAB", true);
        let learning_button = learning.0;
        let exploit_button = exploit.0;

        root.add_css_class("tab-bar");
        root.set_size_request(-1, 28);
        root.append(&learning_button);
        root.append(&exploit_button);

        {
            let stack = stack.clone();
            let clicked_button = learning_button.clone();
            let learning_button = learning_button.clone();
            let exploit_button = exploit_button.clone();
            clicked_button.connect_clicked(move |_| {
                stack.set_visible_child_name("learning");
                set_main_tab_active(&learning_button, true);
                set_main_tab_active(&exploit_button, false);
            });
        }

        {
            let stack = stack.clone();
            let clicked_button = exploit_button.clone();
            let learning_button = learning_button.clone();
            let exploit_button = exploit_button.clone();
            clicked_button.connect_clicked(move |_| {
                stack.set_visible_child_name("exploit");
                set_main_tab_active(&learning_button, false);
                set_main_tab_active(&exploit_button, true);
            });
        }

        Self { root }
    }
}

fn tab_button(icon: &str, text: &str, active: bool) -> (gtk::Button, Rc<gtk::Label>) {
    let button = gtk::Button::new();
    let content = gtk::Box::new(gtk::Orientation::Horizontal, 6);
    let dot = Rc::new(gtk::Label::new(Some(icon)));
    let label = gtk::Label::new(Some(text));

    button.add_css_class("main-tab");
    set_main_tab_active(&button, active);
    if icon == "●" {
        dot.add_css_class("main-tab-dot");
    }

    content.append(dot.as_ref());
    content.append(&label);
    button.set_child(Some(&content));
    (button, dot)
}

fn set_main_tab_active(button: &gtk::Button, active: bool) {
    if active {
        button.add_css_class("main-tab-active");
        button.remove_css_class("main-tab-inactive");
    } else {
        button.add_css_class("main-tab-inactive");
        button.remove_css_class("main-tab-active");
    }
}
