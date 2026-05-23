# 🧊 SleepSize

**SleepSize** é uma ferramenta de proteção e congelamento de diretórios para Linux, desenvolvida em **C** com interface gráfica **GTK3**.

Seu objetivo é impedir alterações não autorizadas em diretórios sensíveis, tornando arquivos e pastas efetivamente somente leitura através de controle de permissões, monitoramento de eventos do sistema de arquivos e mecanismos opcionais de autenticação.

---

## ✨ Principais Recursos

### 🔒 Congelamento de Diretórios

Converte recursivamente um diretório protegido para modo somente leitura:

| Tipo | Permissão |
|--------|--------|
| Diretórios | `0555` |
| Arquivos | `0444` |

Após o congelamento, operações de escrita, modificação e exclusão ficam bloqueadas para usuários sem privilégios adequados.

---

### 🔓 Descongelamento Seguro

Restaura as permissões originais do diretório protegido:

| Tipo | Permissão |
|--------|--------|
| Diretórios | `0755` |
| Arquivos | `0644` |

O processo pode exigir autenticação por senha quando configurado pelo usuário.

---

### 🔑 Proteção por Senha

Opcionalmente, cada diretório protegido pode possuir uma senha de desbloqueio.

Características:

- Hash SHA-256 armazenado localmente
- Senha necessária para descongelar diretórios protegidos
- Senha necessária para adicionar novos arquivos em áreas protegidas
- Implementação em C puro sem dependências criptográficas externas

---

### 📊 Exportação de Metadados em JSON

Gera relatórios contendo informações dos arquivos armazenados:

- Nome
- Caminho
- Tamanho
- Data de modificação
- Tipo do arquivo

Exemplo:

```json
{
  "name": "documento.txt",
  "size": 1024,
  "modified": "2026-05-23T12:00:00"
}
```

---

### 👁️ Monitoramento com Fanotify

Integração com a API **fanotify** do Linux para observação de eventos do sistema de arquivos.

Eventos monitorados:

- Criação de arquivos
- Modificações
- Abertura de arquivos
- Operações de escrita

> Algumas funcionalidades podem exigir privilégios elevados (`CAP_SYS_ADMIN`) dependendo da configuração do sistema.

---

### 🖥️ Interface Gráfica GTK3

Interface simples e intuitiva para gerenciamento dos diretórios protegidos.

Permite:

- Seleção de diretórios
- Ativação de proteção por senha
- Exportação de relatórios JSON
- Congelamento e descongelamento
- Visualização de status e mensagens de erro

---

# 📦 Requisitos

### Dependências

- GCC
- GTK3 Development Libraries
- pkg-config
- build-essential
- Linux Kernel com suporte a fanotify

### Debian / Ubuntu

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    pkg-config \
    libgtk-3-dev
```

---

# 🔨 Compilação

Clone o repositório:

```bash
git clone https://github.com/seuusuario/SleepSize.git
cd SleepSize
```

Compile o projeto:

```bash
make
```

Será gerado o executável:

```bash
./sleepsize
```

---

# 🚀 Utilização

Executar normalmente:

```bash
./sleepsize
```

Ou iniciar com um diretório previamente selecionado:

```bash
./sleepsize /home/usuario/documentos
```

---

# 🖱️ Interface

## Caminho do Diretório

Define o diretório alvo para proteção.

---

## Ativar Fanotify

Habilita o monitoramento de eventos do sistema de arquivos.

---

## Proteção por Senha

Quando ativada:

- Exibe campo para definição de senha
- Exige autenticação para operações críticas

---

## Exportação JSON

Quando ativada:

- Solicita local de destino
- Exporta metadados dos arquivos protegidos

---

## Aplicar Freeze

Executa:

- Congelamento do diretório
- Descongelamento
- Atualização do estado interno
- Inicialização do monitoramento

---

## Status

Área destinada a:

- Mensagens de sucesso
- Alertas
- Erros operacionais

---

# 🏗️ Arquitetura Interna

O projeto é dividido em módulos independentes.

## `main.c`

Responsável pela interface gráfica GTK3 e interação com o usuário.

### Funções

- Inicialização da GUI
- Tratamento de eventos
- Atualização de status
- Integração com o núcleo da aplicação

---

## `sleep_core.c`

Núcleo principal do sistema.

### Responsabilidades

- Congelamento de diretórios
- Descongelamento
- Gerenciamento de permissões
- Controle de autenticação
- Manipulação de estados

---

## `sleep_fan.c`

Camada de monitoramento baseada em Fanotify.

### Responsabilidades

- Inicialização do fanotify
- Registro de eventos
- Monitoramento de alterações
- Tratamento de notificações

---

## `sleep_json.c`

Módulo de exportação.

### Responsabilidades

- Coleta de metadados
- Serialização JSON
- Geração de relatórios

---

## `sleep_util.c`

Funções auxiliares compartilhadas.

### Inclui

- SHA-256 em C puro
- Validação de caminhos
- Canonicalização (`realpath`)
- Cópia segura de arquivos
- Utilidades diversas

---

## `sleepsize.h`

Arquivo central de definições compartilhadas.

Contém:

- Estruturas
- Constantes
- Protótipos
- Macros globais

---

# 📐 Estrutura Principal

```c
typedef struct {
    char path[PATH_MAX];
    int fan_fd;
    bool frozen;
    char passwd_hash[65];
    bool has_password;
} SleepDir;
```

### Campos

| Campo | Descrição |
|---------|---------|
| `path` | Caminho canonicalizado |
| `fan_fd` | Descritor fanotify |
| `frozen` | Estado atual do diretório |
| `passwd_hash` | Hash SHA-256 da senha |
| `has_password` | Indica proteção por senha |

---

# 🔐 Considerações de Segurança

SleepSize implementa diversas medidas de proteção:

- Canonicalização de caminhos (`realpath`)
- Hash SHA-256 para autenticação
- Controle explícito de permissões
- Monitoramento por fanotify
- Operações restritas a diretórios válidos

---

# 🤝 Contribuições

Contribuições são bem-vindas.

Você pode colaborar através de:

- Issues
- Pull Requests
- Sugestões de funcionalidades
- Correções de bugs
- Revisões de segurança

---

# 📄 Licença

Distribuído sob a licença **GPL (GNU General Public License)**.

Consulte o arquivo `LICENSE` para mais informações.
