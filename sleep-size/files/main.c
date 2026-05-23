#include "sleepsize.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char g_paths[SS_MAX_FILES][PATH_MAX];
static int  g_path_count = 0;
static GtkWidget *g_flowbox;
static GtkWidget *g_lbl_ram;
static GtkWidget *g_window;
/* ─── Helpers Visuais ────────────────────────────────────────────────────── */
static GtkWidget *create_icon_button(const char *label, const char *icon_name) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    GtkWidget *img = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(btn), img);
    gtk_button_set_always_show_image(GTK_BUTTON(btn), TRUE);
    return btn;
}
static void apply_dark_theme(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #1a1a1a; color: #ffffff; }"
        "label { color: #ffffff; }"
        "flowboxchild { padding: 10px; border-radius: 8px; }"
        "flowboxchild:hover { background-color: #333333; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}
static gboolean update_ram_label(gpointer data) {
    (void)data;
    long ram_mb = sleep_get_ram_usage_mb();
    char buf[64];
    snprintf(buf, sizeof(buf), "Uso de RAM: %ld MB", ram_mb);
    gtk_label_set_text(GTK_LABEL(g_lbl_ram), buf);
    return G_SOURCE_CONTINUE;
}
/* ─── Diálogo de Status (Antigo Formulário) ──────────────────────────────── */
typedef struct {
    GtkWidget *dialog;
    GtkWidget *btn_apply;
    GtkWidget *btn_rollback;
    GtkWidget *btn_commit;
    GtkWidget *btn_crypto;
    GtkWidget *lbl_status;
    SleepDir  *dir_state;
    char       path[PATH_MAX];
} StatusCtx;
static void set_status(StatusCtx *ctx, const char *msg, gboolean ok) {
    const char *color = ok ? "#2ecc71" : "#e74c3c";
    char markup[512];
        snprintf(markup, sizeof(markup), "<span foreground='%s'>%s</span>", color, msg);
    gtk_label_set_markup(GTK_LABEL(ctx->lbl_status), markup);
}
static void on_btn_apply_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    StatusCtx *ctx = (StatusCtx *)user_data;
    SSError rc = sleep_init(ctx->dir_state, ctx->path, NULL);
    if (rc != SS_OK) { set_status(ctx, "Erro ao inicializar.", FALSE); return; }
    rc = sleep_freeze(ctx->dir_state);
    if (rc != SS_OK) {
        set_status(ctx, "Erro ao montar OverlayFS.", FALSE);
        sleep_destroy(ctx->dir_state);
        return;
    }
       set_status(ctx, "✓ OverlayFS montado com sucesso (Freeze ativo).", TRUE);
    gtk_widget_set_sensitive(ctx->btn_apply, FALSE);
    gtk_widget_set_sensitive(ctx->btn_rollback, TRUE);
    gtk_widget_set_sensitive(ctx->btn_commit, TRUE);
}
static void on_btn_rollback_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    StatusCtx *ctx = (StatusCtx *)user_data;
    if (sleep_rollback(ctx->dir_state, NULL) == SS_OK) {
        set_status(ctx, "✓ Rollback concluído.", TRUE);
    } else {
        set_status(ctx, "Erro no Rollback.", FALSE);
    }
}
static void on_btn_commit_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    StatusCtx *ctx = (StatusCtx *)user_data;
    if (sleep_commit(ctx->dir_state, NULL) == SS_OK) {
        set_status(ctx, "✓ Commit concluído.", TRUE);
    } else {
        set_status(ctx, "Erro no Commit.", FALSE);
    }
}
static void on_btn_crypto_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    StatusCtx *ctx = (StatusCtx *)user_data;
    
    /* Monta o comando que chama o IdenVault passando o caminho atual */
    char cmd[PATH_MAX + 256];
    snprintf(cmd, sizeof(cmd), "\"C:\\Users\\Pedro\\OneDrive\\Desktop\\IdenVault (1)\" \"%s\" &", ctx->path);
    
    int rc = system(cmd);
    if (rc == 0) {
        set_status(ctx, "✓ IdenVault acionado para este cofre.", TRUE);
    } else {
        set_status(ctx, "Erro ao acionar IdenVault.", FALSE);
    }
}
static void open_status_dialog(const char *path) {
    StatusCtx *ctx = g_new0(StatusCtx, 1);
    ctx->dir_state = g_new0(SleepDir, 1);
    strncpy(ctx->path, path, PATH_MAX);
        ctx->dialog = gtk_dialog_new_with_buttons("Status do Cofre",
                                              GTK_WINDOW(g_window),
                                              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                              "Fechar", GTK_RESPONSE_CLOSE,
                                              NULL);
    gtk_window_set_default_size(GTK_WINDOW(ctx->dialog), 400, 200);
        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(ctx->dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 15);
        GtkWidget *lbl_path = gtk_label_new(path);
    gtk_box_pack_start(GTK_BOX(content_area), lbl_path, FALSE, FALSE, 5);
    GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(content_area), hbox_btns, FALSE, FALSE, 10);
    ctx->btn_apply = create_icon_button("Aplicar Freeze", "changes-prevent-symbolic");
    g_signal_connect(ctx->btn_apply, "clicked", G_CALLBACK(on_btn_apply_clicked), ctx);
    gtk_box_pack_start(GTK_BOX(hbox_btns), ctx->btn_apply, TRUE, TRUE, 0);
    ctx->btn_rollback = create_icon_button("Rollback", "edit-undo-symbolic");
    g_signal_connect(ctx->btn_rollback, "clicked", G_CALLBACK(on_btn_rollback_clicked), ctx);
    gtk_widget_set_sensitive(ctx->btn_rollback, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox_btns), ctx->btn_rollback, TRUE, TRUE, 0);
    ctx->btn_commit = create_icon_button("Commit", "emblem-ok-symbolic");
    g_signal_connect(ctx->btn_commit, "clicked", G_CALLBACK(on_btn_commit_clicked), ctx);
    gtk_widget_set_sensitive(ctx->btn_commit, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox_btns), ctx->btn_commit, TRUE, TRUE, 0);
    ctx->lbl_status = gtk_label_new("Pronto.");
    gtk_box_pack_start(GTK_BOX(content_area), ctx->lbl_status, FALSE, FALSE, 5);
    gtk_widget_show_all(ctx->dialog);
    g_signal_connect_swapped(ctx->dialog, "response", G_CALLBACK(gtk_widget_destroy), ctx->dialog);
    
    /* Ao fechar o dialogo, limpa a memoria */
    g_signal_connect(ctx->dialog, "destroy", G_CALLBACK(gtk_main_quit), NULL); 
    /* Isso faria o gtk sair todo, precisamos apenas limpar ctx. */
}
static void on_dialog_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    StatusCtx *ctx = (StatusCtx *)data;
    if (ctx->dir_state->frozen) {
        sleep_unfreeze(ctx->dir_state, NULL);
    }
    sleep_destroy(ctx->dir_state);
    g_free(ctx->dir_state);
    g_free(ctx);
}
static void open_status_dialog_wrapper(const char *path) {
    StatusCtx *ctx = g_new0(StatusCtx, 1);
    ctx->dir_state = g_new0(SleepDir, 1);
    strncpy(ctx->path, path, PATH_MAX);
        ctx->dialog = gtk_dialog_new_with_buttons("Gerenciar Cofre",
                                              GTK_WINDOW(g_window),
                                              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                              "Fechar", GTK_RESPONSE_CLOSE,
                                              NULL);
    gtk_window_set_default_size(GTK_WINDOW(ctx->dialog), 400, 200);
        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(ctx->dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 15);
        GtkWidget *lbl_path = gtk_label_new(path);
    gtk_box_pack_start(GTK_BOX(content_area), lbl_path, FALSE, FALSE, 5);
        GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(content_area), hbox_btns, FALSE, FALSE, 10);
        ctx->btn_apply = create_icon_button("Aplicar Freeze", "changes-prevent-symbolic");
    g_signal_connect(ctx->btn_apply, "clicked", G_CALLBACK(on_btn_apply_clicked), ctx);
    gtk_box_pack_start(GTK_BOX(hbox_btns), ctx->btn_apply, TRUE, TRUE, 0);
        ctx->btn_rollback = create_icon_button("Rollback", "edit-undo-symbolic");
    g_signal_connect(ctx->btn_rollback, "clicked", G_CALLBACK(on_btn_rollback_clicked), ctx);
    gtk_widget_set_sensitive(ctx->btn_rollback, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox_btns), ctx->btn_rollback, TRUE, TRUE, 0);
    ctx->btn_commit = create_icon_button("Commit", "emblem-ok-symbolic");
    g_signal_connect(ctx->btn_commit, "clicked", G_CALLBACK(on_btn_commit_clicked), ctx);
    gtk_widget_set_sensitive(ctx->btn_commit, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox_btns), ctx->btn_commit, TRUE, TRUE, 0);
    ctx->btn_crypto = create_icon_button("IdenVault (Cripto)", "network-workgroup-symbolic");
    g_signal_connect(ctx->btn_crypto, "clicked", G_CALLBACK(on_btn_crypto_clicked), ctx);
    gtk_box_pack_start(GTK_BOX(hbox_btns), ctx->btn_crypto, TRUE, TRUE, 0);
     ctx->lbl_status = gtk_label_new("Pronto.");
    gtk_box_pack_start(GTK_BOX(content_area), ctx->lbl_status, FALSE, FALSE, 5);
    gtk_widget_show_all(ctx->dialog);
    g_signal_connect(ctx->dialog, "destroy", G_CALLBACK(on_dialog_destroy), ctx);
    g_signal_connect_swapped(ctx->dialog, "response", G_CALLBACK(gtk_widget_destroy), ctx->dialog);
}
/* ─── Área de Trabalho (FlowBox) ─────────────────────────────────────────── */
static void on_child_activated(GtkFlowBox *box, GtkFlowBoxChild *child, gpointer data) {
    (void)box;
    (void)data;
    /* Extrai o caminho escondido (ou vinculado) ao botão */
    GtkWidget *vbox = gtk_bin_get_child(GTK_BIN(child));
    GList *children = gtk_container_get_children(GTK_CONTAINER(vbox));
    /* O segundo elemento é o label com o path */
    if (children && children->next) {
        GtkWidget *lbl = GTK_WIDGET(children->next->data);
        const char *path = gtk_label_get_text(GTK_LABEL(lbl));
        open_status_dialog_wrapper(path);
    }
    g_list_free(children);
}
static void add_folder_to_ui(const char *path) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *icon = gtk_image_new_from_icon_name("folder-symbolic", GTK_ICON_SIZE_DIALOG);
    
    /* Pega so o nome da pasta para exibir */
    const char *basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path;
    GtkWidget *lbl_name = gtk_label_new(basename);
    GtkWidget *lbl_path = gtk_label_new(path); /* Escondido mas presente */
    gtk_widget_set_no_show_all(lbl_path, TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_path, FALSE, FALSE, 0); /* Ordem importa para on_child_activated */
    gtk_box_pack_start(GTK_BOX(vbox), lbl_name, FALSE, FALSE, 0);
        gtk_flow_box_insert(GTK_FLOW_BOX(g_flowbox), vbox, -1);
    gtk_widget_show_all(g_flowbox);
}
static void on_btn_add_clicked(GtkButton *btn, gpointer data) {
    (void)btn; (void)data;
    if (g_path_count >= SS_MAX_FILES) return;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Selecionar Diretório",
                                                  GTK_WINDOW(g_window),
                                                  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                  "_Cancelar", GTK_RESPONSE_CANCEL,
                                                  "_Abrir", GTK_RESPONSE_ACCEPT,
                                                  NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            /* Verifica se já existe */
            int exists = 0;
            for (int i = 0; i < g_path_count; i++) {
                if (strcmp(g_paths[i], filename) == 0) { exists = 1; break; }
            }
            if (!exists) {
                strncpy(g_paths[g_path_count], filename, PATH_MAX);
                g_path_count++;
                add_folder_to_ui(filename);
                sleep_state_save(g_paths, g_path_count);
            }
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}
static void on_btn_idenvault_clicked(GtkButton *btn, gpointer data) {
    (void)btn; (void)data;
    system("\"C:\\Users\\Pedro\\OneDrive\\Desktop\\IdenVault (1)\" &");
}
/* ─── Main ───────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    sleep_apply_seccomp();
    gtk_init(&argc, &argv);
    apply_dark_theme();
        g_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_window), "SleepSize Workspace");
    gtk_window_set_default_size(GTK_WINDOW(g_window), 800, 600);
    g_signal_connect(g_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(g_window), vbox);
    /* Header Bar */
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(header, 10);
    gtk_widget_set_margin_end(header, 10);
    gtk_widget_set_margin_top(header, 10);
    gtk_widget_set_margin_bottom(header, 10);
    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 0);
    GtkWidget *btn_add = create_icon_button("Adicionar Cofre", "list-add-symbolic");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_btn_add_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(header), btn_add, FALSE, FALSE, 0);
    GtkWidget *btn_idenvault = create_icon_button("IdenVault", "security-high-symbolic");
    g_signal_connect(btn_idenvault, "clicked", G_CALLBACK(on_btn_idenvault_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(header), btn_idenvault, FALSE, FALSE, 0);
    g_lbl_ram = gtk_label_new("Uso de RAM: -- MB");
    gtk_widget_set_halign(g_lbl_ram, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(header), g_lbl_ram, FALSE, FALSE, 0);
    /* Desktop Area */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    g_flowbox = gtk_flow_box_new();
    gtk_flow_box_set_valign(GTK_FLOW_BOX(g_flowbox), GTK_ALIGN_START);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(g_flowbox), 10);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(g_flowbox), GTK_SELECTION_NONE);
    g_signal_connect(g_flowbox, "child-activated", G_CALLBACK(on_child_activated), NULL);
    
    gtk_container_add(GTK_CONTAINER(scrolled), g_flowbox);
    /* Carrega Estado */
    g_path_count = sleep_state_load(g_paths);
    for (int i = 0; i < g_path_count; i++) {
        add_folder_to_ui(g_paths[i]);
    }
    /* Inicia monitor de RAM */
    g_timeout_add_seconds(1, update_ram_label, NULL);
    gtk_widget_show_all(g_window);
    gtk_main();
    return 0;
}
