/*
 * main.c
 * Interface GTK3 do SleepSize.
 * Janela com checkboxes para configurar e aplicar freeze.
 */

#include "sleepsize.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ─── Contexto da UI ─────────────────────────────────────────────────────── */
typedef struct {
    GtkWidget *window;
    GtkWidget *entry_path;
    GtkWidget *chk_fanotify;    /* obrigatório, desabilitado                */
    GtkWidget *chk_freeze;      /* obrigatório, desabilitado                */
    GtkWidget *chk_password;    /* opcional                                 */
    GtkWidget *chk_json;        /* opcional                                 */
    GtkWidget *entry_passwd;    /* revelado quando chk_password ativo       */
    GtkWidget *entry_json;      /* revelado quando chk_json ativo           */
    GtkWidget *box_passwd;      /* container da senha                       */
    GtkWidget *box_json;        /* container do path JSON                   */
    GtkWidget *btn_apply;
    GtkWidget *lbl_status;
} UICtx;

/* ─── Callbacks ──────────────────────────────────────────────────────────── */

static void on_chk_password_toggled(GtkToggleButton *btn, gpointer user_data)
{
    UICtx *ctx = (UICtx *)user_data;
    gboolean active = gtk_toggle_button_get_active(btn);
    gtk_widget_set_visible(ctx->box_passwd, active);
}

static void on_chk_json_toggled(GtkToggleButton *btn, gpointer user_data)
{
    UICtx *ctx = (UICtx *)user_data;
    gboolean active = gtk_toggle_button_get_active(btn);
    gtk_widget_set_visible(ctx->box_json, active);
}

static void set_status(UICtx *ctx, const char *msg, gboolean ok)
{
    const char *color = ok ? "#2ecc71" : "#e74c3c";
    char markup[512];
    snprintf(markup, sizeof(markup),
             "<span foreground='%s'>%s</span>", color, msg);
    gtk_label_set_markup(GTK_LABEL(ctx->lbl_status), markup);
}

static void on_btn_apply_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    UICtx *ctx = (UICtx *)user_data;

    const char *path   = gtk_entry_get_text(GTK_ENTRY(ctx->entry_path));
    gboolean    use_pw = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctx->chk_password));
    gboolean    use_js = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctx->chk_json));

    const char *passwd  = use_pw ? gtk_entry_get_text(GTK_ENTRY(ctx->entry_passwd)) : NULL;
    const char *jsonout = use_js ? gtk_entry_get_text(GTK_ENTRY(ctx->entry_json))   : NULL;

    if (!path || path[0] == '\0') {
        set_status(ctx, "Informe o caminho do diretório.", FALSE);
        return;
    }

    if (use_pw && (!passwd || passwd[0] == '\0')) {
        set_status(ctx, "Informe a senha.", FALSE);
        return;
    }

    if (use_js && (!jsonout || jsonout[0] == '\0')) {
        set_status(ctx, "Informe o caminho para o arquivo JSON.", FALSE);
        return;
    }

    SleepDir dir;
    SSError  rc;

    /* Inicializa */
    rc = sleep_init(&dir, path, passwd);
    if (rc == SS_ERR_NOTFOUND) { set_status(ctx, "Diretório não encontrado.", FALSE); return; }
    if (rc == SS_ERR_PATH)     { set_status(ctx, "Caminho inválido.",          FALSE); return; }
    if (rc != SS_OK)           { set_status(ctx, "Erro ao inicializar.",        FALSE); return; }

    /* Exporta JSON antes de congelar (enquanto ainda lê) */
    if (use_js && jsonout && jsonout[0] != '\0') {
        if (sleep_export_json(&dir, jsonout) != SS_OK) {
            set_status(ctx, "Erro ao exportar JSON.", FALSE);
            sleep_destroy(&dir);
            return;
        }
    }

    /* Aplica freeze */
    rc = sleep_freeze(&dir);
    if (rc != SS_OK) {
        set_status(ctx, "Erro ao aplicar freeze.", FALSE);
        sleep_destroy(&dir);
        return;
    }

    sleep_destroy(&dir);
    set_status(ctx, "✓ Freeze aplicado com sucesso.", TRUE);
    gtk_widget_set_sensitive(ctx->btn_apply, FALSE);
}

/* ─── Construção da UI ───────────────────────────────────────────────────── */

static void build_ui(UICtx *ctx, const char *initial_path)
{
    /* Janela principal */
    ctx->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ctx->window), "SleepSize v" SS_VERSION);
    gtk_window_set_default_size(GTK_WINDOW(ctx->window), 480, -1);
    gtk_window_set_resizable(GTK_WINDOW(ctx->window), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(ctx->window), 20);
    g_signal_connect(ctx->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Container vertical principal */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(ctx->window), vbox);

    /* Título */
    GtkWidget *lbl_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_title),
        "<b>SleepSize</b> — freeze de diretório");
    gtk_widget_set_halign(lbl_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_title, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox),
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

    /* Campo de path */
    GtkWidget *lbl_path = gtk_label_new("Caminho do diretório:");
    gtk_widget_set_halign(lbl_path, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_path, FALSE, FALSE, 0);

    ctx->entry_path = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->entry_path),
                                   "/home/user/documentos");
    if (initial_path && initial_path[0] != '\0')
        gtk_entry_set_text(GTK_ENTRY(ctx->entry_path), initial_path);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->entry_path, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox),
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

    /* ── Checkboxes obrigatórios ── */
    GtkWidget *lbl_opts = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_opts), "<b>Opções</b>");
    gtk_widget_set_halign(lbl_opts, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_opts, FALSE, FALSE, 0);

    /* Fanotify — obrigatório */
    ctx->chk_fanotify = gtk_check_button_new_with_label(
        "Ativar Fanotify  (monitoramento em tempo real)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctx->chk_fanotify), TRUE);
    gtk_widget_set_sensitive(ctx->chk_fanotify, FALSE); /* sempre ativo */
    gtk_box_pack_start(GTK_BOX(vbox), ctx->chk_fanotify, FALSE, FALSE, 0);

    /* Freeze — obrigatório */
    ctx->chk_freeze = gtk_check_button_new_with_label(
        "Freeze  (nada entra, só sai)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctx->chk_freeze), TRUE);
    gtk_widget_set_sensitive(ctx->chk_freeze, FALSE); /* sempre ativo */
    gtk_box_pack_start(GTK_BOX(vbox), ctx->chk_freeze, FALSE, FALSE, 0);

    /* ── Subcategorias opcionais ── */
    GtkWidget *lbl_sub = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_sub), "  <i>Opções adicionais:</i>");
    gtk_widget_set_halign(lbl_sub, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_sub, FALSE, FALSE, 0);

    /* Senha */
    ctx->chk_password = gtk_check_button_new_with_label(
        "  Desativar pasta com senha");
    g_signal_connect(ctx->chk_password, "toggled",
                     G_CALLBACK(on_chk_password_toggled), ctx);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->chk_password, FALSE, FALSE, 0);

    ctx->box_passwd = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lbl_pw = gtk_label_new("    Senha:");
    ctx->entry_passwd = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(ctx->entry_passwd), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->entry_passwd), "senha secreta");
    gtk_box_pack_start(GTK_BOX(ctx->box_passwd), lbl_pw, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctx->box_passwd), ctx->entry_passwd, TRUE, TRUE, 0);
    gtk_widget_set_no_show_all(ctx->box_passwd, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->box_passwd, FALSE, FALSE, 0);

    /* Exportar JSON */
    ctx->chk_json = gtk_check_button_new_with_label(
        "  Exportar metadados para JSON");
    g_signal_connect(ctx->chk_json, "toggled",
                     G_CALLBACK(on_chk_json_toggled), ctx);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->chk_json, FALSE, FALSE, 0);

    ctx->box_json = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lbl_jp = gtk_label_new("    Salvar em:");
    ctx->entry_json = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ctx->entry_json),
                                   "/home/user/metadados.json");
    gtk_box_pack_start(GTK_BOX(ctx->box_json), lbl_jp, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctx->box_json), ctx->entry_json, TRUE, TRUE, 0);
    gtk_widget_set_no_show_all(ctx->box_json, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->box_json, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox),
        gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

    /* Botão aplicar */
    ctx->btn_apply = gtk_button_new_with_label("❄  Aplicar Freeze");
    g_signal_connect(ctx->btn_apply, "clicked",
                     G_CALLBACK(on_btn_apply_clicked), ctx);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->btn_apply, FALSE, FALSE, 0);

    /* Status */
    ctx->lbl_status = gtk_label_new("");
    gtk_widget_set_halign(ctx->lbl_status, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->lbl_status, FALSE, FALSE, 4);

    gtk_widget_show_all(ctx->window);
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    /* Aceita path como argumento (para integração com Explorer) */
    const char *initial_path = (argc > 1) ? argv[1] : "";

    UICtx ctx = {0};
    build_ui(&ctx, initial_path);

    gtk_main();
    return 0;
}
