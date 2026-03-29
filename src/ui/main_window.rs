use std::rc::Rc;

use gtk::prelude::*;
use gtk4 as gtk;

use crate::app::AppState;
use crate::ui::disasm_view::DisasmView;
use crate::ui::menubar::MenuBarView;
use crate::ui::right_panel::RightPanelView;
use crate::ui::statusbar::StatusbarView;
use crate::ui::technique_list::TechniqueListView;
use crate::ui::toolbar::ToolbarView;

pub fn build_main_window(
    application: &gtk::Application,
    state: Rc<AppState>,
) -> gtk::ApplicationWindow {
    let window = gtk::ApplicationWindow::builder()
        .application(application)
        .title("HeapLens")
        .default_width(1720)
        .default_height(1040)
        .build();
    let root = gtk::Box::new(gtk::Orientation::Vertical, 0);
    let menubar = MenuBarView::new();
    let toolbar = ToolbarView::new();
    let statusbar = StatusbarView::new();
    let exploit_page = build_exploit_page(&state, &toolbar, &statusbar);

    exploit_page.set_hexpand(true);
    exploit_page.set_vexpand(true);

    root.append(&menubar.root);
    root.append(&toolbar.root);
    root.append(&exploit_page);
    root.append(&statusbar.root);
    window.set_child(Some(&root));
    window.maximize();
    window
}

fn build_exploit_page(
    state: &Rc<AppState>,
    toolbar: &ToolbarView,
    statusbar: &StatusbarView,
) -> gtk::Widget {
    let root = gtk::Paned::new(gtk::Orientation::Horizontal);
    let center_split = gtk::Paned::new(gtk::Orientation::Horizontal);
    let techniques = TechniqueListView::new(state.techniques());
    let center = DisasmView::new();
    let right = RightPanelView::new();

    root.add_css_class("main-split");
    center_split.add_css_class("main-split");
    root.set_wide_handle(true);
    center_split.set_wide_handle(true);
    root.set_position(138);
    center_split.set_position(900);
    root.set_resize_start_child(false);
    root.set_resize_end_child(true);
    root.set_shrink_start_child(false);
    root.set_shrink_end_child(true);
    center_split.set_resize_start_child(true);
    center_split.set_resize_end_child(false);
    center_split.set_shrink_start_child(false);
    center_split.set_shrink_end_child(true);

    if let Some(index) = state
        .techniques()
        .iter()
        .find(|item| item.id == "tcache_poisoning")
        .map(|item| item.index)
    {
        state.set_selected_technique(index);
    }

    techniques.connect_selected({
        let toolbar = toolbar.clone();
        let state = state.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        move |label, actual_index| {
            toolbar.set_technique_label(&label);
            if let Some(index) = actual_index {
                state.set_selected_technique(index);
            }
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        }
    });

    bind_controls(state.clone(), toolbar, &center, &right, statusbar);
    toolbar.connect_glibc_clicked({
        let state = state.clone();
        let toolbar = toolbar.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        move || {
            let versions = state.glibc_versions();
            let next = (state.selected_glibc_index() + 1) % versions.len();
            state.set_selected_glibc(next);
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        }
    });
    refresh_exploit_ui(state, toolbar, &center, &right, statusbar);

    center.root.set_hexpand(true);
    center_split.set_start_child(Some(&center.root));
    center_split.set_end_child(Some(&right.root));
    root.set_start_child(Some(&techniques.root));
    root.set_end_child(Some(&center_split));
    root.upcast()
}

fn bind_controls(
    state: Rc<AppState>,
    toolbar: &ToolbarView,
    center: &DisasmView,
    right: &RightPanelView,
    statusbar: &StatusbarView,
) {
    {
        let state = state.clone();
        let toolbar = toolbar.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        let button = toolbar.restart_button.clone();
        button.connect_clicked(move |_| {
            state.reset_lab(true);
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        });
    }
    {
        let state = state.clone();
        let toolbar = toolbar.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        let button = toolbar.run_button.clone();
        button.connect_clicked(move |_| {
            if let Err(error) = state.run_selected() {
                state.record_message(&error);
            }
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        });
    }
    {
        let state = state.clone();
        let toolbar = toolbar.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        let button = toolbar.pause_button.clone();
        button.connect_clicked(move |_| {
            if let Err(error) = state.pause() {
                state.record_message(&error);
            }
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        });
    }
    {
        let state = state.clone();
        let toolbar = toolbar.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        let button = toolbar.step_into_button.clone();
        button.connect_clicked(move |_| {
            if let Err(error) = state.step_selected() {
                state.record_message(&error);
            }
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        });
    }
    {
        let state = state.clone();
        let toolbar = toolbar.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        let button = toolbar.step_over_button.clone();
        button.connect_clicked(move |_| {
            if let Err(error) = state.step_selected() {
                state.record_message(&error);
            }
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        });
    }
    {
        let state = state.clone();
        let toolbar = toolbar.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        let button = toolbar.step_out_button.clone();
        button.connect_clicked(move |_| {
            if let Err(error) = state.step_selected() {
                state.record_message(&error);
            }
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        });
    }
    {
        let state = state.clone();
        let toolbar = toolbar.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        let button = toolbar.run_to_cursor_button.clone();
        button.connect_clicked(move |_| {
            if let Err(error) = state.step_selected() {
                state.record_message(&error);
            }
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        });
    }
    {
        let state = state.clone();
        let toolbar = toolbar.clone();
        let center = center.clone();
        let right = right.clone();
        let statusbar = statusbar.clone();
        let button = toolbar.stop_button.clone();
        button.connect_clicked(move |_| {
            state.reset_lab(true);
            refresh_exploit_ui(&state, &toolbar, &center, &right, &statusbar);
        });
    }
}

fn refresh_exploit_ui(
    state: &Rc<AppState>,
    toolbar: &ToolbarView,
    center: &DisasmView,
    right: &RightPanelView,
    statusbar: &StatusbarView,
) {
    if let Some(technique) = state.selected_technique() {
        toolbar.set_technique_label(&technique.label);
    }
    toolbar.set_glibc_label(state.selected_glibc());
    toolbar.set_mitigations(&state.mitigation_badges());

    let snapshot = state.snapshot_view();
    if let Some(ref snapshot) = snapshot {
        center.set_rows(&snapshot.disasm_rows);
        right.heap.set_chunks(&snapshot.heap_summary, &snapshot.heap_chunks);
        right.trace.set_lines(
            &state
                .trace_text()
                .lines()
                .map(|line| line.to_string())
                .collect::<Vec<_>>(),
        );
        right.regs.set_snapshot(
            &snapshot.rip_address,
            &snapshot.rip_symbol,
            &snapshot.flags,
            &snapshot.register_rows,
        );
        right.proc_tab.set_text(&snapshot.process_text);
    } else {
        center.clear();
        right.heap.clear();
        right.trace.set_lines(
            &state
                .trace_text()
                .lines()
                .map(|line| line.to_string())
                .collect::<Vec<_>>(),
        );
        right.regs.clear();
        right.proc_tab.set_text("Target: waiting");
    }

    let theory = state.theory_text();
    let source = state.source_code();
    let target_summary = state.target_summary();
    let technique_title = state
        .selected_technique()
        .map(|item| item.label.to_uppercase())
        .unwrap_or_else(|| "NO TECHNIQUE".to_string());
    let category = state
        .selected_technique()
        .map(|item| item.category.clone())
        .unwrap_or_else(|| "Uncategorized".to_string());
    let practice = state.practice_sources();
    let logs = state
        .trace_text()
        .lines()
        .rev()
        .take(4)
        .collect::<Vec<_>>()
        .into_iter()
        .rev()
        .collect::<Vec<_>>()
        .join("\n");

    right.info.set_content(
        technique_title.as_str(),
        format!("GLIBC {}  {}", state.selected_glibc(), category).as_str(),
        truncate_block(theory.as_str(), 520).as_str(),
        truncate_block(target_summary.as_str(), 220).as_str(),
        truncate_block(source.as_str(), 220).as_str(),
        practice
            .first()
            .map(|item| format!("Practice: {} / {}", item.provider, item.title))
            .unwrap_or_else(|| "Practice: none".to_string())
            .as_str(),
        if logs.is_empty() { "No trace lines yet." } else { logs.as_str() },
    );

    let latest_trace = state
        .trace_text()
        .lines()
        .rev()
        .find(|line| !line.trim().is_empty())
        .unwrap_or("ready")
        .to_string();
    let right_text = snapshot
        .as_ref()
        .map(|view| view.heap_summary.clone())
        .unwrap_or_else(|| "heap unavailable".to_string());
    statusbar.set_sections(&state.status_text(), latest_trace.as_str(), right_text.as_str());
}

fn truncate_block(text: &str, max_chars: usize) -> String {
    let compact = text
        .lines()
        .map(str::trim)
        .filter(|line| !line.is_empty())
        .collect::<Vec<_>>()
        .join("\n");
    if compact.chars().count() <= max_chars {
        compact
    } else {
        let mut out: String = compact.chars().take(max_chars).collect();
        out.push_str("...");
        out
    }
}
