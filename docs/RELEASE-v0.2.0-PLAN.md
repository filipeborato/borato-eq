# BORATO EQ — Plano do release v0.2.0

**Tema:** sistema de presets, comparação A/B funcional e presets do usuário.

Hoje o header já tem os elementos visuais (`< Default >`, `A`, `B`, `A > B`, engrenagem),
mas eles são placeholders. Este release transforma isso em funcionalidade real.

> Boa parte da lógica de A/B e snapshots já existe, pronta para portar, no projeto
> irmão **borato-224** (`Source/PluginProcessor.cpp`): `storeSnapshot()`, `recallSnapshot()`,
> `swapAB()`, `captureEditableParameters()`, `applyParameterMap()` e o wrapper de estado
> versionado `BORATO224_STATE`. A ideia é reaproveitar esse padrão.

---

## 1. Sistema de presets de fábrica

**Modelo de dados**
- Um preset = nome + snapshot de todos os parâmetros editáveis da APVTS.
- Tabela estática de presets nomeados (como `programPresets` no borato-224), em
  `Source/PresetManager.h`. Cada entrada: `{ const char* name; std::map<String,float> values; }`
  ou um struct tipado por banda.
- Presets iniciais sugeridos: `Default`, `Vocal Air`, `Bus Glue`, `Low-End Weight`,
  `Vintage Dark`, `Modern Open`, `Master Polish`.

**Aplicação**
- Aplicar via `parameter->beginChangeGesture(); setValueNotifyingHost(convertTo0to1(v)); endChangeGesture();`
  (mesmo helper `setParameterValue` do borato-224), para que o host registre as mudanças.
- O áudio pega os novos coeficientes pelo caminho já existente da flag `paramsDirty`.

**Exposição ao host**
- Implementar `getNumPrograms / getProgramName / getCurrentProgram / setCurrentProgram`
  apontando para a tabela (hoje retornam 1/"Default").

---

## 2. Comparação A/B

**Modelo**
- Dois slots, `A` e `B`, cada um um snapshot completo de parâmetros. Slot ativo rastreado.
- `A` / `B`: ao trocar de slot, guarda o estado atual no slot que estava ativo e
  recarrega o do slot destino (`swapAB()` do borato-224 faz exatamente isso).
- `A > B`: copia o conteúdo de A para B.
- Botões `A`/`B` em radio group (já estão), `A > B` momentâneo.

**Persistência**
- Salvar slots A/B + slot ativo no `getStateInformation` envolvendo o estado da APVTS
  num elemento raiz versionado (ex.: `BORATOEQ_STATE`, espelhando o padrão do 224),
  e restaurar no `setStateInformation` com fallback para o formato simples atual.

---

## 3. Presets do usuário (salvar/carregar em disco)

**Localização**
```
<userApplicationDataDirectory>/Borato Company/BORATO EQ/Presets/<nome>.boratoeq
```
(`juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)`).

**Formato**
- `apvts.copyState().createXml()` dentro de um elemento raiz com atributos
  `name` e `pluginVersion`, para compatibilidade futura.

**Operações (no dropdown do seletor de presets)**
- **Save Preset…** → `juce::AlertWindow` pedindo o nome, grava o `.boratoeq`.
- **Save As / Overwrite** → regrava preset existente.
- **Delete** → remove o arquivo.
- **Open presets folder** → `presetDir.revealToUser()`.
- Ao carregar, fazer scan do diretório e listar junto dos presets de fábrica.

---

## Menu de configurações (engrenagem ⚙)

A engrenagem abre um `juce::PopupMenu` (ou um pequeno painel overlay) com
preferências **globais do plugin** — coisas que não fazem parte do som de um
preset específico e que o usuário ajusta uma vez. Persistir em
`juce::ApplicationProperties` (arquivo de settings do usuário), **não** no estado
da sessão, para valerem em todas as instâncias.

Sugestões, por ordem de utilidade para um EQ paralelo valvulado:

**Qualidade / processamento**
- **Oversampling** — `Off / 2x / 4x` e um modo **HQ no render/bounce** (mais
  oversampling offline do que em tempo real). Já temos a infraestrutura de
  `juce::dsp::Oversampling`; falta expor o fator. Lembrar de re-reportar
  `setLatencySamples()` ao trocar.
- **Auto gain / makeup** — compensação automática de nível ao empurrar COLOR
  (liga/desliga), para A/B "loudness-matched".
- **Safety limiter na saída** — limiter suave opcional pós-output para evitar
  picos absurdos com drive alto (liga/desliga).

**Interface**
- **Tamanho da GUI** — presets de escala (100% / 125% / 150% / 200%), além do
  resize livre que já existe.
- **Tooltips** — liga/desliga dicas ao passar o mouse.
- **Brilho dos LEDs** — atenuar o vermelho para sessões em ambiente escuro.
- **Tema** — variações sutis (ex.: "Graphite" / "Vintage"), se quisermos no futuro.

**Comportamento dos knobs**
- **Modo de arraste** — rotary vs. vertical/horizontal linear.
- **Sensibilidade do mouse** e **modificador de ajuste fino** (Shift/Ctrl).
- **Duplo-clique reseta** ao valor padrão (on/off).

**Estado / padrões**
- **Salvar estado atual como padrão** — o que carrega ao abrir uma nova instância.
- **Restaurar padrões de fábrica** — limpa as preferências do usuário.

**Sobre**
- Versão, fabricante (BORATO COMPANY), link/site e atalho **Open presets folder**.

> Mínimo viável para o v0.2.0: **Oversampling**, **Tamanho da GUI**, **Tooltips**
> e **Sobre**. O resto entra incrementalmente conforme a necessidade.

---

## Integração de UI (`HeaderBarComponent`)

- O display `Default` vira clicável → `juce::PopupMenu` com duas seções:
  **Factory** e **User** (com marcação do preset atual).
- `<` / `>` navegam pela lista combinada (fábrica + usuário).
- Engrenagem → `PopupMenu` de **configurações globais** (ver seção abaixo).
- `A` / `B` / `A > B` conectados aos novos métodos do processor.
- Indicar estado "modificado" (ex.: asterisco ou cor) quando o usuário mexe após carregar.

---

## Arquivos a tocar

- **Novo:** `Source/PresetManager.{h,cpp}` — tabela de fábrica, IO de presets de
  usuário, índice atual, scan de diretório.
- `Source/PluginParameters.{h,cpp}` — helper com a lista de todos os IDs editáveis
  para snapshot (ou gerar a partir de `BoratoEq::bands` + globais).
- `Source/PluginProcessor.{h,cpp}` — A/B + snapshots + estado versionado; expor
  presets via `getNumPrograms` etc. (portar do borato-224).
- `Source/ui/HeaderBarComponent.{h,cpp}` — wiring de prev/next/display/A/B/A>B/gear,
  `PopupMenu`, indicador de "modificado".
- **Novo:** `Source/Settings.{h,cpp}` — preferências globais via
  `juce::ApplicationProperties` (oversampling, escala da GUI, tooltips, etc.),
  separadas do estado da sessão.

---

## Critérios de aceitação

- Trocar preset de fábrica muda audível e visualmente todos os controles.
- A/B: ajustar, ir pro B, ajustar diferente, alternar A/B para comparar; `A > B` copia.
- Salvar um preset de usuário, reabrir o plugin/sessão e ele aparece e recarrega certo.
- Sem cliques/glitches; mudanças de preset envolvidas em gesture (host grava automação).
- RT-safe: aplicação de preset acontece no message thread; o áudio pega pela flag
  `paramsDirty` já existente — nada de IO ou alocação no audio thread.

---

## Fora de escopo (depois)

- Categorias/tags de preset e navegador dedicado.
- Mapeamento de Program Change (MIDI) para presets.
- Morphing/interpolação entre presets.
- Importar/exportar pacotes de presets.
