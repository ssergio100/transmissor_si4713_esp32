# Como alterar os menus do TFT

O encoder gira para selecionar ou ajustar. O clique abre um item; no ajuste,
confirma e retorna ao submenu. Nas opções binárias (transmissão, mono/estéreo,
pré-ênfase, entrada de áudio e RDS), o clique alterna e aplica diretamente
na lista, mantendo o item selecionado. A pressão longa descarta a edição em andamento
e retorna à tela principal. Valores já confirmados permanecem aplicados.
As listas são circulares: depois do último item vem o primeiro; antes do
primeiro vem o último. Assim, um passo para trás no início chega a VOLTAR.
Os ajustes numéricos mantêm seus limites mínimo e máximo.
Não há instruções de operação, legenda nem rodapé dentro das janelas.

## Onde mexer

| Quero alterar | Arquivo e local |
| --- | --- |
| Nome ou ordem das opções | Listas no início de `menu.cpp` |
| Cor da janela, seleção ou texto | Cores da janela em `tema_tft.h` |
| Posição e tamanho da janela | `X`/`Y` em `janela_menu_tft.h`; tamanho no início do `.cpp` |
| Espaçamento e aparência das linhas | Função `linha` em `janela_menu_tft.cpp` |
| Valor mostrado ao abrir um ajuste | Função `iniciarEdicao` em `menu.cpp` |
| Efeito do giro e limites | Função `girar` em `menu.cpp` |
| Campo alterado ao confirmar | Função `copiarCampoConfirmado` em `menu.cpp` |
| Alternância de ligado/desligado e outras opções binárias | Função `selecionar` em `menu.cpp` |
| Aplicação no hardware | Função `executarAcao` no arquivo `.ino` |
| Tela principal | `tela_principal_tft.cpp` |

## Alterar uma opção existente

Uma linha da lista tem dois campos:

```cpp
{RF_POTENCIA, "POTENCIA"},
```

`RF_POTENCIA` identifica a função. `"POTENCIA"` é o texto visível.
Para mudar apenas o nome, edite o texto entre aspas. Para mudar a ordem,
mova a linha inteira dentro da mesma lista. A navegação e o desenho leem as
mesmas listas; não é necessário atualizar uma contagem manual.

As listas principais são `menuInicial`, `menuRf`, `menuAudio` e `menuRds`.
A lista `menuResultado` contém as ações depois de encontrar um canal.
Mantenha `VOLTAR` como primeira opção do resultado: sem resultado válido,
somente essa opção fica disponível. Não mova itens entre categorias sem
revisar seu comportamento em `selecionar`.

A raiz comporta seis linhas e os submenus comportam cinco. Listas maiores
rolam para manter a seleção visível. O resultado da busca comporta duas ações.
Nomes longos devem ser conferidos na imagem gerada para não encostar no valor.

## Alterar uma cor

```cpp
constexpr uint16_t FUNDO_SELECAO = AZUL_PETROLEO;
```

Escolha um nome da paleta, como `PRETO`, `AZUL_ARDOSIA` ou `AZUL_PETROLEO`.
Para definir uma nova cor, acrescente seu nome na seção 1 de `tema_tft.h`
usando `RGB(vermelho, verde, azul)`, com cada componente de 0 a 255. O helper
converte a cor para o formato do TFT. As cores ficam no tema, separadas dos
parâmetros de RF; códigos hexadecimais de registradores e do PI continuam
hexadecimais porque representam dados do rádio, não cores.

## Acrescentar um ajuste numérico

1. Dê um identificador ao item em `ItemPainel`, no arquivo `estado_painel.h`.
2. Acrescente sua linha à lista do submenu.
3. Em `iniciarEdicao`, copie o valor atual para `valorEditado_`.
4. Em `girar`, defina mínimo, máximo e passo com `ajustarNumero`.
5. Em `copiarCampoConfirmado`, copie o rascunho para o campo correspondente.
6. Em `valorAtual` e `mostrarEdicao`, formate o número e sua unidade.

Se o parâmetro ainda não existir no transmissor, também será necessário
incluí-lo no modelo e no driver. Adicionar uma linha ao menu não cria uma
função de hardware automaticamente.

Os limites usados nesta mudança foram preservados do projeto. Em particular,
`frequenciaKhz` é um nome legado: armazena passos de **10 kHz**. O valor `9950`
representa **99,50 MHz**. Não use esse número como kHz literais.

## Por que o ajuste fica separado

`valorEditado_` e `textoEditado_` são rascunhos. Girar não modifica o rádio.
No clique de confirmação, o menu recebe uma cópia atual da configuração e
altera somente o campo selecionado. Assim, uma alteração feita pela interface
web em outro campo durante a edição não é perdida.

O `.ino` executa a operação e chama `concluirAplicacao`. Em caso de falha,
a janela apresenta a mensagem; um clique a dispensa, conservando o editor
para nova tentativa. A pressão longa fecha tudo.

A persistência existente foi mantida: a frequência é gravada pelo transmissor
quando confirmada. Os outros ajustes locais são aplicados em funcionamento,
sem acrescentar gravação automática nesta mudança. O menu SISTEMA foi removido;
essas funções continuam disponíveis nas APIs existentes quando suportadas.

## Texto RDS e código PI

Girar escolhe uma posição. Clicar nessa posição entra na edição do caractere:
o quadrado preenchido mostra qual caractere será alterado. Um novo clique
aceita o caractere e retorna à escolha das posições. A opção `CONCLUIR` fica
logo após a última posição na navegação e aplica o texto inteiro.

O nome tem oito posições, o RadioText tem 32 e o PI tem quatro dígitos
hexadecimais. Não é necessário clicar em todos os caracteres para concluir:
é possível girar até `CONCLUIR`. A pressão longa descarta o texto inteiro ainda
não confirmado. A escolha da fonte dinâmica do RadioText existente na interface
web é preservada; o editor local altera o campo de texto manual.

## Monitor e canal livre

MONITOR abre as leituras de áudio já existentes; um clique retorna à raiz.
CANAL LIVRE solicita a busca imediatamente. Durante a busca, o giro não faz
nada, o clique cancela e retorna à raiz, e a pressão longa cancela e retorna
à tela principal. Ao concluir, aparecem a frequência, VOLTAR e APLICAR.
O resultado de uma busca anterior não é oferecido se a nova tentativa falhar.

## Desenho e memória

A janela ocupa `288 × 176` pixels, a partir de `x=16, y=60`, deixando a faixa
NO AR/RDS descoberta. O restante do painel fica congelado sob a janela.
Ao fechar, ele é redesenhado com o estado atual, inclusive os campos que
não mudaram enquanto estavam cobertos.

`GFXcanvas16` monta a janela em RAM antes do envio. Seu buffer ocupa
`288 × 176 × 2 = 101.376 bytes`, alocados uma vez. Esse consumo dinâmico não
aparece no total de variáveis globais informado pelo compilador Arduino.
A inicialização verifica se a alocação funcionou. A janela é montada e enviada somente quando muda algum conteúdo visível.
`conteudoMudou` compara os campos usados por cada menu; ao adicionar um valor
visível, inclua sua comparação nessa função. Ao reabrir, `forcar=true` garante
a restauração mesmo se os valores forem iguais aos da última abertura.

O envio fica em `DisplayTft::renderizar`, chamando `drawRGBBitmap` diretamente
em `tft_`. Não passe esse envio por uma referência `Adafruit_GFX&`: nessa
biblioteca a função não é virtual, e a versão genérica configura o destino
pixel por pixel. A versão do driver SPI configura a área e envia linhas em
bloco. Quando há mudança, ainda enviamos a janela inteira; atualizações
parciais ficam para uma etapa posterior, se a medição na placa justificar.
Fluidez e memória disponível com Wi-Fi devem ser verificadas na placa.

## Verificar

Teste de comportamento, sem placa:

```sh
sh scripts/testar_menus.sh
```

Para também executar os renderizadores reais e gerar imagens PPM, indique
onde a biblioteca Adafruit_GFX do projeto está instalada:

```sh
MENU_GFX_DIR='/caminho/para/Adafruit GFX Library' sh scripts/testar_menus.sh
```

As imagens ficam em `build/previas-menu`. Esse teste verifica que a janela não
escreve fora da sua área, que o cabeçalho pode atualizar sem apagá-la e que o
retorno restaura o painel. Os stubs em `tests/gfx_stubs` substituem somente o
suporte Arduino do computador; as operações gráficas são da Adafruit_GFX real.

Compilação para o perfil ESP32-S3 existente:

```sh
sh scripts/compilar.sh
```

Na placa, conferir giro/clique, confirmação e saída longa em cada ajuste,
busca concluída/cancelada, falha do rádio, legibilidade, resposta do encoder e
abertura/fechamento com a rede ativa. Os testes de computador não comprovam
resposta elétrica do encoder, tempo de SPI ou operação RF real.
