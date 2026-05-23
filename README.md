SleepSize: Ferramenta de Congelamento de Diretórios
<img width="216" height="182" alt="image" src="https://github.com/user-attachments/assets/791e0420-e88b-4624-b68c-917524953dc1" />

Visão Geral

SleepSize é uma ferramenta desenvolvida em C com interface gráfica GTK3 que permite "congelar" diretórios, controlando rigorosamente as permissões de acesso. Uma vez congelado, um diretório e seus arquivos se tornam somente leitura, impedindo modificações indesejadas. A ferramenta também oferece funcionalidades adicionais, como proteção por senha para descongelamento e exportação de metadados dos arquivos para o formato JSON.

Funcionalidades

•
Congelamento de Diretórios: Altera as permissões de um diretório e seus conteúdos para somente leitura (0555 para diretórios, 0444 para arquivos), impedindo qualquer escrita ou modificação.

•
Descongelamento Seguro: Restaura as permissões originais (0755 para diretórios, 0644 para arquivos). Pode ser protegido por senha para maior segurança.

•
Proteção por Senha: Opcionalmente, um diretório pode ser protegido por senha. O descongelamento e a adição de novos arquivos exigirão a senha correta (hash SHA-256).

•
Exportação de Metadados JSON: Gera um arquivo JSON contendo informações detalhadas sobre os arquivos dentro do diretório (nome, tamanho, data de modificação).

•
Monitoramento com Fanotify: Utiliza a API fanotify do Linux para monitorar eventos de modificação e criação de arquivos no diretório, embora a funcionalidade completa possa requerer privilégios elevados (CAP_SYS_ADMIN).

•
Interface Gráfica (GTK3): Oferece uma interface de usuário intuitiva para configurar e aplicar as operações de congelamento e descongelamento.

Requisitos

Para compilar e executar o SleepSize, você precisará de:

•
Um compilador C (GCC recomendado).

•
Bibliotecas de desenvolvimento GTK3 (libgtk-3-dev).

•
pkg-config para gerenciar as dependências do GTK3.

•
build-essential (no Debian/Ubuntu) ou equivalente para ferramentas de desenvolvimento.

•
Sistema operacional Linux (devido ao uso de fanotify).

Compilação

Siga os passos abaixo para compilar o SleepSize:

1.
Instalar Dependências:

Bash


sudo apt-get update
sudo apt-get install -y libgtk-3-dev pkg-config build-essential





2.
Navegar até o Diretório do Projeto:

Bash


cd /caminho/para/o/diretorio/sleepsize





3.
Compilar:

Bash


make



Isso criará o executável sleepsize no diretório atual.



Uso

Para iniciar a interface gráfica do SleepSize, execute o binário gerado:

Bash


./sleepsize



Você pode, opcionalmente, passar um caminho de diretório como argumento para pré-preencher o campo de caminho na interface:

Bash


./sleepsize /home/usuario/meus_documentos



Interface Gráfica

A interface do usuário permite:

•
Caminho do Diretório: Insira o caminho completo do diretório que deseja congelar/descongelar.

•
Ativar Fanotify: (Obrigatório, desabilitado para edição) Indica o monitoramento de eventos.

•
Freeze: (Obrigatório, desabilitado para edição) Indica o estado de congelamento.

•
Desativar pasta com senha: Ativa/desativa a proteção por senha. Se ativada, um campo para a senha será exibido.

•
Exportar metadados para JSON: Ativa/desativa a exportação de metadados. Se ativada, um campo para o caminho do arquivo JSON será exibido.

•
Botão "Aplicar Freeze": Inicia o processo de congelamento ou descongelamento, dependendo do estado atual e das opções selecionadas.

•
Status: Exibe mensagens de sucesso ou erro.

Detalhes Técnicos

O SleepSize é modularizado em vários arquivos C, cada um com responsabilidades específicas:

•
main.c: Implementa a interface gráfica GTK3 e gerencia as interações do usuário.

•
sleep_core.c: Contém a lógica principal de inicialização, congelamento, descongelamento e adição de arquivos, manipulando permissões e estados.

•
sleep_fan.c: Gerencia a integração com a API fanotify do Linux para monitoramento de eventos do sistema de arquivos.

•
sleep_json.c: Responsável pela exportação dos metadados dos arquivos para um formato JSON personalizado.

•
sleep_util.c: Fornece funções utilitárias, incluindo um algoritmo SHA-256 puro em C para hashing de senhas, validação de caminhos seguros e uma função de cópia de arquivos.

•
sleepsize.h: Define estruturas de dados, constantes e protótipos de funções compartilhadas entre os módulos.

Estrutura de Dados Chave

A estrutura SleepDir (sleepsize.h) armazena o estado de um diretório gerenciado:

Plain Text


typedef struct {
    char  path[PATH_MAX];       /* caminho canonicalizado do diretório       */
    int   fan_fd;               /* fd do fanotify (-1 se não iniciado)       */
    bool  frozen;               /* true = congelado                          */
    char  passwd_hash[65];      /* SHA-256 hex da senha (vazio = sem senha)  */
    bool  has_password;         /* proteção por senha ativa?                 */
} SleepDir;



Contribuição

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues para bugs ou sugestões, ou enviar pull requests com melhorias.

Licença

Este projeto está licenciado sob a licença MIT.

