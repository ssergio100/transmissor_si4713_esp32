# Como alterar os menus do TFT

O encoder gira para selecionar ou ajustar. O clique abre um item; no ajuste,
confirma e retorna ao submenu. Nas opções binárias (transmissão, mono/estéreo,
pré-ênfase, entrada de áudio, RDS e passo de frequência), o clique alterna e aplica diretamente
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
A lista `menuSistema` contém o repouso do display, o passo de frequência e o
volume do receptor usado como monitor.
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
quando confirmada. Os outros ajustes locais são aplicados em funcionamento.
O tempo de repouso do display usa uma chave própria na NVS e é gravado ao ser
confirmado, sem gravar junto os demais ajustes que ainda não foram salvos.
O passo de frequência usa outra chave própria e também é gravado imediatamente.
O volume do monitor segue o mesmo modelo de persistência.

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

## Repouso do display

O menu SISTEMA oferece `NUNCA`, `13 S`, `30 S`, `1 MIN`, `5 MIN`, `15 MIN` e
`30 MIN`. Esses tempos ficam na lista `TEMPOS_REPOUSO_DISPLAY_SEGUNDOS`, em
`configuracao.h`; o padrão é 5 minutos. A escolha é gravada imediatamente.

Após o período sem atividade do encoder, o firmware envia Display Off e
Sleep In ao ST7789. O primeiro giro, clique ou pressionamento longo acorda o
display e é consumido: ele não navega nem altera uma opção. Depois de Sleep
Out, o firmware aguarda 120 ms e redesenha a tela atual.

## Passo de frequência

O item `PASSO FREQUENCIA`, no menu SISTEMA, alterna diretamente entre
`0.1 MHz` e `0.2 MHz BR`. O padrão brasileiro é 0,2 MHz: a canalização começa
em 76,1 MHz e segue por 76,3; 76,5; 76,7 MHz, mantendo o último décimo ímpar.
Essa escolha afeta o ajuste local pelo encoder; a API continua aceitando a
resolução mínima de 0,1 MHz oferecida pelo rádio.

Se uma frequência antiga estiver fora da grade selecionada, o primeiro giro
leva ao próximo canal válido na direção escolhida. Com o passo brasileiro, o
maior centro de canal abaixo do limite superior é 107,9 MHz.

O conector atualmente descrito no projeto não oferece um pino separado para
a iluminação. Por isso, o repouso para a varredura do painel, mas não controla
eletricamente o LED de fundo.

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

### Diagnostico do multiplex em AUDIO > MODO

Cada clique avanca e aplica imediatamente: MONO (`0x0000`), ESTEREO
(`0x0003`), ESTEREO + RDS (`0x0007`), SOMENTE PILOTO (`0x0001`),
SOMENTE L-R (`0x0002`), APENAS L e APENAS R, retornando a MONO. Os valores sao escritos em
`TX_COMPONENT_ENABLE` (`0x2100`) e conferidos por leitura do chip.
A selecao sincroniza o estado de RDS com o bit correspondente; nao altera
volume, ganho, pre-enfase, desvio de audio, frequencia ou potencia.

Use "Salvar no dispositivo" na interface web para persistir a configuracao,
como antes para Mono/Estereo. Preferencias antigas preservam a combinacao anterior de
Mono/Estereo e RDS. Alterar TRANSMISSAO RDS separadamente volta a combinacao
legada de Mono/Estereo e RDS; para diagnostico isolado, selecione novamente
um dos modos em AUDIO > MODO.

APENAS L e APENAS R mantem `TX_COMPONENT_ENABLE = 0x0003` e usam somente
`TX_LINE_INPUT_MUTE` (`0x2105`) para isolar as entradas: `0x0001` silencia R
(APENAS L), `0x0002` silencia L (APENAS R). Ao selecionar esses modos, o mute
global e liberado para permitir ouvir o canal escolhido; ao voltar a qualquer
modo normal, ambas as entradas ficam ativas (`0x0000`). A preferencia e os dados
RDS sao preservados ao entrar no teste L/R, mas o bit RDS nao e transmitido
nesses modos, conforme a mascara obrigatoria `0x0003`. O Serial informa o nome
do modo e os valores das duas propriedades.

Referencia dos bits: [AN332 Rev. 1.2](https://manuals.plus/m/8efcb83e9cd60b226feb5233ccd6a6a90a6b69dd4a16f946a1624049018d338d),
propriedade TX_LINE_INPUT_MUTE, pagina 39 (LIMUTE = bit 1; RIMUTE = bit 0).

Antes das escritas de cada aplicacao de configuracao, o Serial registra leituras
reais via GET_PROPERTY de `0x2100 TX_COMPONENT_ENABLE`, `0x2102
TX_PILOT_DEVIATION`, `0x2107 TX_PILOT_FREQUENCY` e `0x2105 TX_LINE_INPUT_MUTE`.
Mostra hexadecimal e decimal, acrescentando Hz para desvio (unidade de 10 Hz)
e frequencia do piloto (unidade de 1 Hz). Ao entrar em APENAS L/R, le novamente
o mute depois da aplicacao. Leituras malsucedidas aparecem como FALHA, sem
substituicao por valores esperados. Esse diagnostico nao escreve propriedades
nem corrige valores do piloto.
