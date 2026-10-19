# Histórico do relatório

Relatório técnico "Inserção adaptativa de elementos de interface em malhas 2D". A versão atual fica em
`doc/report/relatorio.pdf` (fontes em `doc/report/`), e o PDF de cada versão publicada fica nesta pasta como
`relatorio_vX.Y.pdf`.

Para publicar uma nova versão:
1. incrementar a macro `\versao` em `doc/report/relatorio.tex`;
2. acrescentar uma linha na tabela de histórico logo após o resumo (`relatorio.tex`) e uma entrada abaixo;
3. regenerar as figuras (`tools/gen.py`, ver Apêndice A do relatório) e compilar;
4. copiar o PDF compilado para `historico/relatorio_vX.Y.pdf`, sem apagar as versões anteriores.

## Pendências para a próxima versão (v0.4)

Itens pedidos pelo autor para entrar quando o relatório for gerado novamente. **Não gerar o relatório antes de o autor
pedir.**

- [ ] Refletir no relatório as modificações de código feitas desde a v0.3 (lista abaixo): seção 7.4 (I3, I4 e I5
  resolvidos; texto de I2 atualizado), seção 2 (`numberOfHalfEdges()` e `O[]` reservado começando com `BORDER`),
  seções 2.4/2.5 e pseudocódigos da seção 4 (`addPoint()` → `duplicatePoint(v)`), verificação V7 na seção 6 e no
  apêndice A, teste de coordenadas com 13 verificações e o novo `test_vertex_count.cpp` (11 casos).

### Modificações de código desde a v0.3

- I4 corrigido (`IBHM.{h,cpp}`, `IBHMInterfaceElementBuilder.cpp`): novo método `IBHM::duplicatePoint(vertex)`, que
  acrescenta um ponto com as coordenadas de um vértice existente. Lê a origem por índice e copia cada valor para uma
  variável local antes do `push_back`, então não precisa de vetor temporário e continua correto se o vetor for
  realocado. Os quatro operadores (Canônico de borda, Split, Expand, Open Hole) usam `duplicatePoint(v)` em vez de
  `addPoint()`: as cópias passam a nascer na posição do vértice original. Escolhido em vez de montar um
  `std::vector` com as coordenadas a cada cópia, para não alocar memória temporária no caminho crítico.
  - Ferramentas: o `driver` ganhou a verificação **V7** (toda cópia tem as coordenadas do vértice original);
    `test_coordinates.cpp` ganhou 3 verificações de `duplicatePoint` (dentro da reserva, com realocação durante a cópia
    e em 3D), total de 13.
  - Verificação: teste de coordenadas 13/13; exemplo dos slides com a mesma topologia (incremental e lote) e V7 ok;
    rodadas do estresse com os mesmos números (259/141 e 400/400), agora também com V7 em todos os casos válidos.
    Controle negativo: com o construtor antigo, a V7 falha já na primeira aresta do exemplo.
- I8 corrigido (pelo autor, revisado): as mensagens de operador indefinido em `insertInterfaceElements` usam `%u`, e
  a do vértice B imprime `b` em vez de `a`. Compila sem avisos; exemplo e 100 casos do estresse sem alteração.
- I10, parte dos includes, corrigida: `IBHM.cpp` inclui `<cassert>`, `<cstdio>` (`printf` em `print()`), `<cstring>`
  (`memset`) e `<algorithm>` (`std::upper_bound`); `IBHMInterfaceElementBuilder.cpp` inclui `<cassert>` e `<cstdio>`
  (`printf`). Saíram os includes sem uso (`<map>` em `IBHM.cpp` e `<iostream>` no construtor). Compila sem avisos;
  exemplo, teste de coordenadas e 100 casos do estresse sem alteração. A parte do `CMakeLists.txt` (exige Qt) continua
  em aberto.
- Convenção definida pelo autor: funções e tipos herdados de C (`printf`, `memset`, `size_t`) são usados sem `std::`,
  com `<cstdio>` e `<cstring>`. (Uma versão intermediária com `std::` foi descartada.) A inspeção do CLion pode marcar
  `<cstdio>`/`<cstring>` como desnecessários por inclusão indireta no libc++; o `misc-include-cleaner` do clang-tidy os
  considera necessários, e eles são mantidos.
- I5 corrigido (`IBHMInterfaceElementBuilder.{h,cpp}`): `computeNumberOfNewVertices` agora devolve o número **exato**
  de vértices novos. Em duas etapas: (1) seleciona só as arestas que `insertInterfaceElements` vai abrir, com os mesmos
  testes (pula arestas de borda, de elementos de interface e repetidas, identificadas por `min(a, O[a])`) e conta, por
  vértice, quantas o tocam; (2) aplica a regra uma vez por vértice: em elemento de interface → n; fora, na borda → n;
  fora, interno → n − 1. O teste de borda roda uma vez por vértice distinto (antes: duas vezes por aresta). Código e
  cabeçalho comentados em detalhe.
  - Efeito colateral: como as arestas de elementos de interface são descartadas antes de `isBorder`, a entrada que
    travava o programa (I2, ex.: `34` e depois `48`) agora é rejeitada normalmente. O defeito latente de `isBorder` (o
    desvio da aresta colapsada pula a parada) continua, mas o construtor não chega mais a esse caminho.
  - Novo teste `doc/report/tools/test_vertex_count.cpp` (8 casos: exemplo em lote, aresta repetida inclusive pela outra
    meia-aresta, aresta de borda, combinação, vértice já em elemento de interface, aresta de elemento de interface e
    vértice de borda com duas arestas). Código novo: 8/8. Código antigo: 5 contagens erradas e trava no caso da aresta
    de elemento de interface.
  - Estresse: mesmos números (259/141 e 400/400) e nenhuma contagem diferente da criada, inclusive no lote.
- I3 corrigido (`IBHM.{h,cpp}`, `IBHMInterfaceElementBuilder.{h,cpp}`), como sugerido na seção 7.4:
  - Novo `IBHM::numberOfHalfEdges()` (= `Of[n_c]`, o número de half-edges dos elementos válidos; ao contrário de
    `numberReservedElements()`, não conta o espaço reservado).
  - `reserveSpaceForElements` inicializa as posições reservadas de `O[]` com `BORDER` (antes, 0, que é uma half-edge
    válida). Os operadores sempre definem a oposta das quatro half-edges novas, então a inserção não muda.
  - `insertInterfaceElements` lê o limite `numberOfHalfEdges()` **no início da chamada** e rejeita, com mensagem no
    console como as outras rejeições, todo índice `>=` limite (fora do vetor ou no espaço reservado). É o primeiro
    teste, antes de `heOpposite`. `computeNumberOfNewVertices` aplica o mesmo teste (teste 0), então a contagem
    continua exata. Um índice de elemento de interface criado na própria chamada também fica fora do limite e é
    rejeitado; criado em chamada anterior, continua rejeitado pelo teste de elemento de interface.
  - Custo: uma comparação por aresta da entrada, fora dos operadores.
  - O `@warning` de `insertInterfaceElements` (índices não validados e laço infinito, este já resolvido pelo I5) foi
    substituído pela lista de rejeições, que agora inclui os índices fora da malha.
  - Ferramentas: o `driver` não consulta mais índices fora da malha antes de passá-los ao construtor (imprime os
    vértices como `BORDER`); `test_vertex_count.cpp` ganhou 3 casos (índice fora do vetor, índice no espaço reservado
    com verificação de `O[]` = `BORDER`, e índices inválidos misturados a uma aresta válida no mesmo lote): 11 casos,
    15 verificações, todas ok.
  - Controle com o código antigo: `999` para na asserção de `heOpposite`; `48` no espaço reservado (após uma chamada
    só com aresta de borda) é contado como 2 vértices novos e a inserção para na asserção de `openHole`. Código novo:
    ambos rejeitados, nenhum vértice ou elemento criado.
  - Verificação: exemplo dos slides com 19 vértices novos, mesma partição incremental e em lote; estresse com os
    mesmos números (259/141, abortos todos I1, e 400/400) e `prediction_mismatch` 0.

## v0.3 — 18/10/2026

- Saiu o confronto com a apresentação de projeto (antiga seção 7.1, itens A1–A10) e as referências a ele; a
  apresentação não é mais mantida e continua citada só como origem das ideias e do exemplo. Com isso, a seção de
  defeitos passou de 7.5 para **7.4**.
- Nova figura dedicada às hipóteses x1 e x3 do Split Element (seção 4.2), com as consultas de
  `retrieveAvailableVertex` e o caso em que nenhuma vale (Expand Edge).
- Seção 7.1 (comentários) virou uma tabela com o estado de B1–B14: todos corrigidos, exceto o B10. Seção 7.4
  (defeitos) atualizada: I1 com as duas asserções observadas, I4 restrito às cópias na origem, novo I11 (política de
  memória em `reserveSpaceForElements` e nos vetores do construtor).
- Seção 2 atualizada com a reserva exata e o novo `addPoint`; verificação com o teste de coordenadas e a contagem das
  asserções da rodada 1.
- Ferramentas documentadas (`driver.cpp`, `gen.py`, `stress.py`, `compare.py`) e novo `test_coordinates.cpp`; o
  apêndice A explica cada uma, o formato do JSON e o desenho das figuras, com uma figura nova das malhas aleatórias.
- Verificação refeita com o código do commit `066cbde`: mesmos números da v0.2 (rodada 1: 259 válidas e 141 abortos,
  todos com vértice de borda com 2+ arestas; rodada 2: 400/400; Split → Expand: 791 ocorrências, topologia idêntica).

### Modificações de código entre a v0.2 e a v0.3

- B1 corrigido: em `IBHMInterfaceElementBuilder.h`, o `@param he` de `openHole` agora diz que o vértice já pertence a
  um elemento de interface e **não** tem aresta colapsada (não está em `_collapsed`). No relatório: retirar B1 da lista
  da seção 7.2 (ou marcá-lo como resolvido).
- B3 corrigido: em `IBHM.h`, o comentário de `_halfEdgeVertex` não cita mais `_numberVerticesByElement` (membro da CHE);
  descreve a organização por `_offset` (elemento i nas posições `_offset[i]` a `_offset[i+1]-1`) e o espaço reservado.
- B4 corrigido: na documentação da classe `IBHMInterfaceElementBuilder`, "There are only 4 operators to 3D" virou
  "only 4 operators in 2D ... topological only ... to ease the future extension to 3D".
- B5 corrigido: `splitElement` descreve as duas posições do elemento compartilhado (elemento de `he` ou de
  `opposite(he)`), correspondentes às hipóteses x1 e x3.
- B6 corrigido: `heOpposite`, `_oppositeHalfEdge` e o novo comentário do enum `OPPOSITE` documentam `BORDER (-1)` e
  `COLLAPSED (-2)`.
- B7 corrigido (só documentação): `computeNumberOfNewVertices` descreve as regras e diz que o resultado é um limite
  superior, explicando as duas fontes de excesso (arestas que serão rejeitadas e a regra de borda aplicada por aresta,
  que conta 2k - 1 em vez de k). O comportamento não mudou; a contagem em excesso continua sendo o defeito I5.
- B8 corrigido: `reindexElementsCCW` documenta a pré-condição de não chegar à half-edge que segue uma aresta colapsada.
- B9 corrigido: `insertInterfaceElements` documenta as arestas ignoradas (borda, já em elemento de interface,
  repetidas), que nada é informado a quem chama, e um `@warning` sobre índices não validados (I2/I3). Corrigido também
  "There user can select anyone of them".
- B11 corrigido (só documentação): `numberReservedElements` agora diz que devolve o número de posições de half-edge
  reservadas, não de elementos. O nome do método não mudou (não é usado em lugar nenhum).
- B12 corrigido: "See @reindexElements" virou "See reindexElementsCCW()".
- B13 corrigido: `_collapsed` diz que o valor é a própria half-edge colapsada (h3 ou h1) e quando a entrada é removida.
- B14 corrigido: cabeçalho de `IBHM.h` ("File: IBHM.h"), `elementsList` (sem citar a CHE), destrutor ("Default
  destructor"), parâmetro inexistente `numberOfVertices` no construtor, descrição de `buildOppositesLinear`, "the
  bother" → "the border" e "the same build" → "the same builder".
- B10 **continua em aberto**. Uma solução foi implementada e depois revertida a pedido do autor, por acrescentar
  verificações ao caminho crítico e deixar o algoritmo mais complexo e possivelmente mais lento. Para retomar no futuro,
  a ideia era: cada operador verifica, antes de alterar a malha, se pode ser aplicado e devolve `false` sem mudar nada
  (Canônico: vértice já em elemento de interface; Split: sem aresta colapsada ou meias-arestas fora das hipóteses x1/x3;
  Expand: rotação a partir de `c` que não alcança `he`; Open Hole: uma das rotações chega à borda antes de achar um
  elemento de interface); um método `applyOperator(he, elementHE)` substitui os dois `switch`, e `insertInterfaceElements`
  trata o retorno com `assert`. Com ela, os 141 abortos da rodada 1 passavam a vir de `assert(appliedA/appliedB)` e, em
  release, não havia mais escrita fora do vetor. Princípio definido pelo autor: a arquitetura deve garantir que o
  operador escolhido sempre seja aplicável; caso contrário, é bug.
- B2 corrigido e I4 corrigido em parte (código, `IBHM.{h,cpp}`):
  - `reserveSpaceForNodes` usa `reserve` **exato** (só a capacidade cresce, exatamente o pedido); o tamanho de
    `_coordinates` passa a ser sempre `numberPoints() * dim`. Decisão do autor: eficiência de memória é prioridade, sem
    crescimento geométrico; se forem esperadas muitas inserções, o usuário reserva mais espaço de uma vez.
  - `addPoint` recebe `const std::vector<float> &coordinates = {}` em vez de `const float*`. Não há limite fixo para o
    número de coordenadas (decisão do autor: preferir uma estrutura flexível). O vetor deve ter exatamente
    `numberCoordinatesByVertex()` valores, ou ser vazio para usar a origem (padrão). Como é lido pelos próprios índices,
    e não por um ponteiro para o buffer interno, é seguro montá-lo a partir das coordenadas de um ponto da malha. O
    construtor verifica `numberCoordinates > 0`. (Uma versão intermediária com `std::array` de tamanho fixo foi
    descartada.)
  - Comentários de `addPoint`, `reserveSpaceForNodes` e `_numberOfValidPoints` atualizados.
  - Verificação: teste de coordenadas (reserva exata, índice correto, cópia de vértice, sem realocação dentro da reserva,
    crescimento além da reserva, malha 3D com 3 coordenadas), exemplo dos slides (incremental e lote: mesma topologia da
    v0.2) e as duas rodadas do estresse com números idênticos aos da v0.2 (259/141 e 400/400).
  - Falta em I4: as cópias criadas pelos operadores ainda nascem na origem (o construtor chama `addPoint()` sem
    coordenadas).

### Observações registradas para o texto da v0.3

- I1 (antes da correção do B10) aparecia em modo debug por duas asserções: `currentHE != IBHM::BORDER` em `openHole`
  (72 das 141 malhas da rodada 1) e `position < _offset.size() - 1` em `heNext`, chamada pelo segundo laço do
  `openHole` com `BORDER` (69). (Com a solução do B10, revertida, passava a aparecer como `assert(appliedA/appliedB)`.)
- `reserveSpaceForElements` continua usando `resize` (necessário, pois as posições são gravadas por índice), mas o
  `resize` cresce geometricamente e pode alocar até o dobro do necessário em `_halfEdgeVertex`, `_oppositeHalfEdge` e
  `_offset` (o mesmo vale para os vetores `_isInterfaceElement`/`_inInterfaceElement` do construtor). Se a política de
  memória exata valer também aqui, basta chamar `reserve` exato antes do `resize`. Ainda não decidido pelo autor.


## v0.2 — 18/10/2026

- A análise passou a ser feita sobre `IBHMInterfaceElementBuilder`, a implementação que será evoluída. A implementação
  baseada na CHE (`CHE`, `CHEInterfaceElementBuilder`) foi um experimento inicial e saiu do relatório; a teoria da CHE
  foi mantida.
- Nova seção 2.5 com os vetores da IBHM (V, O, Of) antes e depois de uma inserção.
- Algoritmo de travessia descrito com a validação de entrada e o `commitElement(4)` antes dos operadores.
- Verificação refeita só com a IBHM: 800 malhas aleatórias (quadriláteros, triângulos, híbridas e com hexágonos),
  três ordens mais uma inserção em lote por malha; experimento Split → Expand com 791 ocorrências.
- Novos casos-limite: índices de half-edge inválidos ou no espaço reservado, malha com furo interno.
- Comentários do código revisados em `IBHMInterfaceElementBuilder.h` e `IBHM.h` (B1–B14) e defeitos restritos à IBHM
  (I1–I10).
- Ferramentas (`tools/driver.cpp`, `gen.py`, `stress.py`) passaram a usar só a IBHM; o driver ganhou `--batch`.

## v0.1 — 18/10/2026

- Primeira versão, baseada em `CHEInterfaceElementBuilder`: introdução ao problema, IBHM, arquitetura, operadores,
  travessia, verificação (600 casos) e confronto documentação × implementação.
