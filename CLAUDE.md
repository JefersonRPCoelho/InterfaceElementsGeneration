# CLAUDE.md

Contexto para as sessões do Claude Code neste repositório. O projeto é longo (2D → testes → GeMA → 3D); leia isto
antes de mexer no código ou no relatório.

## Objetivo

Inserir automaticamente **elementos de interface** (quadriláteros de espessura nula, 4 nós em pares coincidentes) em
malhas de elementos finitos, a partir de um conjunto de arestas (faces em 3D), de forma **incremental/adaptativa** e
sobre uma estrutura de dados compacta baseada em índices (**IBHM**, Lage et al. 2017, `doc/papers/`). Autor do
algoritmo: Jeferson Coelho (Tecgraf/PUC-Rio).

## O que é base e o que é legado

- **Base a ser evoluída:** `core/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.{h,cpp}` sobre
  `core/TopologyDataStructures/IBHM.{h,cpp}`. Não pode ter defeitos: toda mudança deve passar pelos testes abaixo.
- **Legado (experimento inicial, não evoluir, ignorar em relatórios):** `CHE.{h,cpp}` e
  `CHEInterfaceElementBuilder.{h,cpp}`, e os arquivos `malha.txt` e `malha2.txt`. A *teoria* da CHE continua válida
  como base conceitual da IBHM.
- `doc/ppt/Operators_Template.{pptx,pdf}` (mesmo conteúdo) foi feita **antes** da implementação, cumpriu seu papel e
  **não é mais mantida**: não apontar nem corrigir imprecisões dela. A documentação de referência é o relatório em
  `doc/report/` (e, em caso de conflito, o código).

## Como o algoritmo funciona (resumo)

- Para cada half-edge `a` (A→B, elemento U) com oposta `b` (B→A, elemento L), cria um elemento `[F,G,H,E]` com
  half-edges `h0..h3 = nextAvailableHE()..+3`. `h0 ↔ b` e `h2 ↔ a` ("arestas do meio", sempre ligadas no fim);
  `h3` (lado curto em A) e `h1` (lado curto em B) são responsabilidade dos operadores, chamados com `(a, h3)` e `(b, h1)`.
- Convenção: a origem de `elementHE` fica do lado do elemento que contém `edgeHE`; `next(elementHE)` fica do outro lado.
- Sentinelas em `O[]`: `BORDER = -1`, `COLLAPSED = -2` (lado curto de comprimento zero, vértice ainda não duplicado).
- Estado do builder: `_inInterfaceElement[v]`, `_isInterfaceElement[e]`, `_collapsed` (vértice → half-edge colapsada).
  Um único builder por malha durante toda a edição.
- Escolha do operador (só pelo estado do vértice de `he`):
  - vértice fora de EI → **Canônico** (interno: aresta colapsada, 0 vértices; borda: vértice novo + rotação até a borda);
  - colapsado e EI vizinho de um elemento da aresta nova (hipóteses x1/x3) → **Split Element** (O(1));
  - colapsado, sem elemento comum → **Expand Edge** (rotação anti-horária de `c` até `he`);
  - em EI sem colapsada → **Open Hole** (abre/aumenta o buraco). Em vértice de borda (externa ou de furo), um dos
    dois laços pode chegar à borda antes de um EI: o buraco se abre para o exterior e esse lado age como o Canônico
    de borda (correção do I1; decisão do autor: tratar dentro do Open Hole, sem quinto operador).
- Split Element é um caso particular (otimização) do Expand Edge: trocar um pelo outro dá a mesma topologia.
- **Princípio:** a arquitetura deve garantir que o operador escolhido sempre é aplicável; se não for, é bug. Uma
  implementação com verificações prévias e `assert` foi testada e revertida por custo. Decisão provisória do autor
  (B10): os operadores devolvem `true` e `insertInterfaceElements` faz `assert` do resultado (e do operador
  `UNDEFINED`), para o erro aparecer cedo; o relato de falhas fica para depois. Evitar verificações extras no caminho
  crítico sem combinar com o autor.
- Invariante: cada vértice termina com tantas cópias quantos são os componentes da sua estrela cortada pelas arestas
  selecionadas (interno com 1 aresta: 1 cópia, colapsado; borda com k arestas: k+1). Por isso o resultado não depende
  da ordem de inserção nem de inserir em lote.
- Na IBHM, `commitElement(4)` é chamado **antes** dos operadores (next/prev dependem de `_offset`), e posições
  reservadas de `O[]` começam com `BORDER` (antes do I3, com 0). Entrada válida: `h < numberOfHalfEdges()` lido no
  início da chamada (`= Of[n_c]`); `numberReservedElements()` inclui o espaço reservado.

## Defeitos conhecidos da IBHM (relatório v0.4, seção 7.4: tabela de abertos e de corrigidos)

- **Em aberto (deixados para depois pelo autor): I7 (relato de inseridas/rejeitadas; hoje `printf`), I9 (desempenho)**,
  mais o pico de memória da construção (`buildOppositesLinear`) e pendências menores. As sugestões combinadas estão em
  `doc/report/historico/HISTORICO.md`, seção "Sugestões registradas para implementação futura": para o I7, devolver uma
  entrada por aresta de entrada (elemento criado ou código de rejeição), não a contagem nem a lista (os elementos novos
  já são a faixa contígua no fim); a biblioteca deixa de imprimir e quem chama registra. Não implementar sem o autor
  pedir. Um defeito em aberto pode ganhar um teste desativado (prefixo `DISABLED_`) que descreve o comportamento
  correto; ao corrigir, o teste é ativado. Hoje não há nenhum.
- **Já corrigidos:** I11 (crescimento exato: `reserve` antes de cada `resize` em `reserveSpaceForElements` e nos vetores
  de flags do construtor; reserva só o que falta; `reserveCapacityForElements` público), I2 (`isBorder`: o passo pela
  aresta colapsada passa pela condição de parada como os demais; testes `IBHMIsBorder` em `test_ibhm.cpp`), I6
  (sentinelas: `heNext`/`hePrevious`/`heElement` têm `assert(halfEdge < numberOfHalfEdges())`; `assert(!= COLLAPSED)` em
  x1/x3, `reindexElementsCCW` e laços do Open Hole; só em debug), I1 (Open Hole em vértice de borda: os dois laços param
  na borda, `assert` de que ao menos um lado encontra EI; testes em `test_open_hole_border.cpp`), I3 (índices fora de
  `[0, numberOfHalfEdges())` rejeitados na inserção e na contagem; `O[]` reservado começa com `BORDER`), I4 (cópias
  nascem com as coordenadas do original, via `IBHM::duplicatePoint`), I5 (contagem exata de vértices novos), I8 e I10
  (includes; o CMake não depende mais de Qt). Correções feitas desde a última versão do relatório:
  `doc/report/historico/HISTORICO.md`.
- **Memória é prioridade:** reservas devem ser exatas (sem crescimento geométrico: `reserve` exato antes de todo
  `resize` que cresce um vetor); quem usa a IBHM pode reservar mais espaço de uma vez, com
  `reserveCapacityForElements(posições, elementos)` e `reserveSpaceForNodes(nós)` (só capacidade). Sem essa reserva,
  inserir uma aresta por chamada realoca e copia a malha a cada chamada; com ela, nenhuma realocação.
  `reserveSpaceForElements(p, e)` garante `p` posições e `e` entradas livres depois dos elementos válidos e reaproveita
  as que sobraram (de arestas rejeitadas). Não assumir número máximo fixo de coordenadas: `addPoint` recebe `const
  std::vector<float>&` (vazio = origem), nunca ponteiro para o vetor interno. Para copiar um vértice, usar
  `duplicatePoint(vertex)` (sem vetor temporário).

## Build e testes

- CMake sem dependências externas no núcleo (Qt saiu; volta quando houver interface). Alvos: `InterfaceElementsLib`
  (núcleo, sem CHE), `InterfaceElementsPrototype` (`main.cpp` + CHE legado), e em `tests/`: `InterfaceElementsTests`
  (Google Test), `driver` e as bibliotecas de teste. O `cmake`/`ninja` do CLion ficam em
  `/Applications/CLion.app/Contents/bin/{cmake,ninja}/mac/aarch64/` (não estão no PATH). Build fora do repositório:
  ```
  cmake -S . -B $W/build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build $W/build && (cd $W/build && ctest -j8)
  ```
  `ctest -L unit` (suítes Google Test, ~0,2 s) e `ctest -L stress` (`stress_supported`, seed 22/400, ~7 s).
- **Testes (`tests/`)**: Google Test (pacote instalado, ou v1.17.0 baixado pelo `FetchContent` com hash fixo), cada
  `TEST` roda em processo próprio no CTest (timeout 60 s). O núcleo é recompilado sem `NDEBUG` para os testes
  (`InterfaceElementsLibChecked`), em qualquer tipo de build: os `assert` precisam estar ligados.
  - `test_ibhm.cpp` (pontos, reserva, `numberOfHalfEdges`, testes de morte, `isBorder` em toda half-edge do
    exemplo), `test_vertex_count.cpp`,
    `test_base_mesh.cpp` (exemplo dos slides: 24 operadores, vértices por passo, V1–V7, ordem/lote/várias chamadas),
    `test_invalid_input.cpp` (I3 e rejeições), `test_open_hole_border.cpp` (I1: vértice de borda com 2+ arestas,
    nas duas ordens, em lote, na malha com furo e com 3 arestas).
  - `tests/support/`: `TestMeshes` (malha base = `malha2Hyb.txt` com o vértice 18 em (4,3), leques com 3 e 4
    quadriláteros, `meshIBHM.txt` com furo, dois triângulos),
    `MeshValidation` (snapshot, V1–V5, origem das cópias e V7, partição dos cantos), `BuilderInspector` (acesso de
    leitura ao estado privado do construtor, que o declara `friend`; não usar `#define private public`, que quebra a
    ligação no MSVC).
  - `stress_all` (seed 21/400, todas as configurações) e `stress_supported` (seed 22/400, sem vértice de borda com
    2+ arestas) precisam passar.
- `doc/report/tools/driver.cpp` (alvo `driver`): insere arestas (`--batch` para lote), imprime o operador por vértice,
  valida com `tests/support/MeshValidation` (V1–V7) e grava JSON (usado por `gen.py`, `stress.py`, `compare.py`).
  Exemplo dos slides: `driver malha2Hyb.txt $W/ex.json 31 34 11 26 47 38 23 19 30 42 29 45` → 19 vértices novos,
  `TOTAL ERRORS: 0`. Sem CMake:
  ```
  c++ -std=c++17 -Icore/TopologyDataStructures -Icore/InterfaceElementsBuilder -Itests/support \
      doc/report/tools/driver.cpp tests/support/MeshValidation.cpp core/TopologyDataStructures/IBHM.cpp \
      core/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.cpp -o $W/driver
  ```
- `doc/report/tools/stress.py <driver> <seed> <casos> <dir> [--skip-border-multi]`: malhas aleatórias (quad, tri,
  híbrida, polígonos), 3 ordens + lote; código de saída 1 se algum caso falhar. Referência atual: seed 21/400 →
  400/400 (148 com vértice de borda com 2+ arestas); seed 22/400 com `--skip-border-multi` → 400/400. Até a v0.4
  (antes do I1) a seed 21 dava 259 ok e 141 abortos; os casos mudam porque um aborto pula sorteios.
- `doc/report/tools/compare.py`: compara topologias (partição dos cantos dos elementos originais). As ferramentas são
  explicadas no apêndice A do relatório.
- Formato de malha (`malha2Hyb.txt`, `meshIBHM.txt`): `np ne`, `np` linhas `x y`, `ne` linhas `n v0 ... v(n-1)`.
  Obs.: em `malha2Hyb.txt` o vértice 18 deveria estar em (4,3), não (3,4) (só afeta geometria).

## Relatório técnico (`doc/report/`)

- Em português; fontes em `relatorio.tex`, `styles.tex`, `secoes/*.tex`; figuras de malha em `figs/` são **geradas**
  por `tools/gen.py` a partir de execuções reais do driver (não editar à mão; regenerar).
- Compilar com `tectonic -X compile relatorio.tex` (instalado via Homebrew) ou xelatex.
- **Versionamento obrigatório:** nunca sobrescrever versões anteriores. A cada nova versão: incrementar `\versao` em
  `relatorio.tex`, acrescentar linha na tabela de histórico (após o resumo) e em `historico/HISTORICO.md`, compilar e
  copiar o PDF para `historico/relatorio_vX.Y.pdf`. Versão atual: **0.4**.
- **Só gerar uma nova versão quando o autor pedir.** Os itens pedidos para a próxima versão ficam na seção
  "Pendências para a próxima versão" de `historico/HISTORICO.md`; consulte-a ao gerar o relatório.

## Convenções de código

- C++17, comentários e identificadores em inglês, Doxygen (`/** ... */`, `@param`, `@return`), indentação de 4 espaços,
  chaves em linha própria, linhas de até 120 colunas, `[[nodiscard]]` em consultas.
- Funções e tipos herdados de C são usados **sem** `std::` (`printf`, `memset`, `size_t`), com os cabeçalhos `<cstdio>`
  e `<cstring>` (regra do autor). Incluir diretamente o cabeçalho de tudo o que o arquivo usa (`<cstdio>`, `<cstring>`,
  `<algorithm>`...) e não usar comentários para suprimir avisos da IDE. A inspeção de includes do CLion pode marcar
  esses cabeçalhos como desnecessários, porque no libc++ eles chegam por inclusão indireta; não removê-los (o projeto
  também mira Windows/MSVC).
- Qualquer correção deve valer para malhas híbridas (triângulos, quadriláteros, polígonos) e ser verificada com o
  `ctest` (suítes + estresse) antes de ser dada como pronta. Defeito corrigido ganha teste em `tests/`.

## Roteiro

1. Corrigir I1–I6 (feito) e transformar o driver/stress em testes automatizados (feito: Google Test + CTest).
2. Geometria das cópias (coordenada do vértice de origem) e mapa vértice novo → origem (transferência de campos).
3. Relato de erros/rejeições na API; manter `VH[]` da IBHM.
4. Integração com o GeMA.
5. Extensão 3D (faces selecionadas; duplicação também de arestas; operadores 2D reaparecem como operadores de aresta).
